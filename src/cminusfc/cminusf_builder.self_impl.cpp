#include "cminusf_builder.hpp"
#include "logging.hpp"
#include <stack>


// use these macros to get constant value
#define CONST_FP(num) \
    ConstantFP::get((float)num, module.get())
#define CONST_ZERO(type) \
    ConstantZero::get(var_type, module.get())

/// 更详细的类型分类
/// 修改注释：添加Const标志，用于固定值的预计算。
/// 涉及到的功能：优化输出结构（固定的结果编译时进行计算）、编译时下标越界检查、
typedef enum CM_TYPE{
    CM_EMPTY = 0,
    CM_INT = 0x1,
    CM_FLOAT = 0x2,
    CM_BOOL = 0x4,
    CM_ERR = 0x8,
    CM_VOID = 0x10,
    CM_ARRAY = 0x20,
    CM_CONST = 0x40,
    CM_PARAM = 0x80
} CM_TYPE;

int block_counter = 0;
/// 函数参数类型
/// Type不从Value派生，故无法使用bottom_up_stack向上传递，改用一个vector来传递函数变量类型
std::vector<Type*> param_list;

/// 当前正在处理的函数入
Function* function = nullptr;

/// 这两个栈中的元素保持对应，即类型栈栈顶代表的类型即为值的类型
/// 注意：由于CminusType中没有i1的枚举，故此实现中i1采用TYPE_VOID进行存储
std::stack<Value*> bottom_up_stack;  // 值传递栈
std::stack<int> type_stack;   // 类型栈

/// 黑洞，用于吸收ret后的所有不可达指令，当builder指向此处时，所插入的代码将不会体现在输出的.ll上
/// 当程序遇到ret语句时，在执行完builder->create_(void_)ret后，builder的插入点将会被设置到__bb黑洞处
/// 此操作可以减少部分冗余代码，也可防止不可达代码带来的lli编译问题
Module* blackholeModule = new Module("MOD");
FunctionType* __funcType = FunctionType::get(Type::get_void_type(blackholeModule), {});
Function* __blackholeFunc = Function::create(__funcType, "NONE", blackholeModule);
BasicBlock* __bb = BasicBlock::create(blackholeModule, "bb", __blackholeFunc);

Type* GetDeclType(CminusType _type, Module* module){
    /**
     * @desc: 将ASTNode中的Type转换为LightLR中Type*类型
     * @param _type: CMinusType表示的类型 
     * @param module: 模块指针
     */
    switch (_type)
        {
        case TYPE_INT:
            return Type::get_int32_type(module);      
        case TYPE_FLOAT:
            return Type::get_float_type(module);
        case TYPE_VOID:
            return Type::get_void_type(module);
        default:
            throw "Unknown declaration type: " + std::to_string(_type);
        }
}

inline std::string GetNewBlockName(){
    /**
     * 获取一个新的BasicBlock名 
     **/
    return std::to_string(++block_counter);
}

int Val2CM_TYPE(Type* t){
    int ret = CM_EMPTY;
    Type* pt;
    if(t->is_function_type()){
        pt = ((FunctionType*)t)->get_return_type();
    }else{
        // 注：由于编译器实现，调用此函数传入的参数一定为AllocaInst所返回的指针类型，直接转型
        pt = t->get_pointer_element_type();
    }
    if(pt->is_pointer_type()){
        // ArrayType，仅出现在数组参数传递过程中
        ret |= CM_PARAM | CM_ARRAY;
        pt = pt->get_pointer_element_type();
    }
    if(pt->is_array_type()){
        ret |= CM_ARRAY;
        pt = ((ArrayType*)pt)->get_element_type();
    }
    if(pt->is_integer_type()){
        ret |= CM_INT;
    }else if(pt->is_float_type()){
        ret |= CM_FLOAT;
    }else if(pt->is_void_type()){
        ret |= CM_VOID;
    }
    return ret;
}

int CminusType2CM_TYPE(CminusType cm){
    switch (cm)
    {
    case TYPE_INT:
        return CM_INT;
    case TYPE_FLOAT:
        return CM_FLOAT;
    case TYPE_VOID:
        return CM_VOID;
    default:
        throw "unknown CminusType";
    }
}

void LoadFromPointerIfNeeded(IRBuilder* builder){
    Value* val = bottom_up_stack.top();
    if( dynamic_cast<GlobalVariable*>(val) ||
        dynamic_cast<AllocaInst*>(val) ||
        dynamic_cast<GetElementPtrInst*>(val)){
        // 传入为指针，需要读取值
        bottom_up_stack.pop();
        bottom_up_stack.push(builder->create_load(val));
    }
}

void CminusfBuilder::visit(ASTProgram &node) {
    for (auto n : node.declarations)
    {
        n->accept(*this);
    }
}

void CminusfBuilder::visit(ASTNum &node) { 
    switch (node.type)
    {
    case TYPE_INT:
        bottom_up_stack.push(ConstantInt::get(node.i_val, module.get()));
        break;
    case TYPE_FLOAT:
        bottom_up_stack.push(ConstantFP::get(node.f_val, module.get()));
        break;
    default:
        throw "unknown CminusType";
    }
    type_stack.push(CminusType2CM_TYPE(node.type) | CM_CONST);
}

void CminusfBuilder::visit(ASTVarDeclaration &node) { 
    Type* decl_type = GetDeclType(node.type, module.get());
    Type* var_type = nullptr;
    Value* var = nullptr;
    if(node.type == TYPE_VOID){
        throw "variable has incomplete type 'void'";
    }
    if(node.num != nullptr){
        // Array Type
        node.num->accept(*this);
        // 获取Num的值
        ConstantInt* num = dynamic_cast<ConstantInt*>(bottom_up_stack.top());
        bottom_up_stack.pop();
        type_stack.pop();
        // 数组大小小于等于0，报错
        if(!num || num->get_value() <= 0){
            throw "Array length should be a positive integer";
        }
        // 获得ArrayType类型
        ArrayType* arr_type = ArrayType::get(decl_type, num->get_value());
        var_type = arr_type;
    }else{
        // Not Array Type
        var_type = decl_type;
    }
    if(scope.in_global()){
        // 全局变量
        ConstantZero* initializer = ConstantZero::get(decl_type, module.get());
        var = GlobalVariable::create(node.id, module.get(), var_type, false, initializer);
    }else{
        // 局部变量
        var = builder->create_alloca(var_type);
    }
    if(!scope.push(node.id, var)){
        throw "Redefinition of '" + node.id + '\'';
    }
}

void CminusfBuilder::visit(ASTFunDeclaration &node) {
    Type* ret_type = GetDeclType(node.type, module.get());
    param_list.clear();
    for(auto param: node.params){
        param->accept(*this);
    }
    FunctionType* f_type = FunctionType::get(ret_type, param_list);
    Function* func = Function::create(f_type, node.id, module.get());
    scope.push(node.id, func);
    function = func;
    scope.enter();
    int i = 0;
    builder->set_insert_point(BasicBlock::create(module.get(), GetNewBlockName(), function));
    for(auto arg: function->get_args()){
        if(node.params[i]->type == TYPE_VOID){
            throw "void type only allowed for function results";
        }
        AllocaInst* val = nullptr;
        if(node.params[i]->isarray){
            if(node.params[i]->type == TYPE_INT){
                val = builder->create_alloca(Type::get_int32_ptr_type(module.get()));
            }else if(node.params[i]->type == TYPE_FLOAT){
                val = builder->create_alloca(Type::get_float_ptr_type(module.get()));
            }
        }
        else{
            if(node.params[i]->type == TYPE_INT){
                val = builder->create_alloca(Type::get_int32_type(module.get()));
            }else if(node.params[i]->type == TYPE_FLOAT){
                val = builder->create_alloca(Type::get_float_type(module.get()));
            }
        }
        builder->create_store(arg, val);
        if(!scope.push(node.params[i]->id, val))
            throw "redefinition of '" + node.params[i]->id + '\'';
        ++i;
    }
    node.compound_stmt->accept(*this);
    switch (node.type)
    {
    case TYPE_VOID:
        builder->create_void_ret();
        break;
    case TYPE_INT:
        builder->create_ret(ConstantInt::get(0, module.get()));
        break;
    case TYPE_FLOAT:
        builder->create_ret(ConstantFP::get(0, module.get()));
        break;
    default:
        break;
    }
    scope.exit();
}

void CminusfBuilder::visit(ASTParam &node) { 
    if(node.type == TYPE_INT){
        if(node.isarray)
            param_list.push_back(Type::get_int32_ptr_type(module.get()));
        else
            param_list.push_back(Type::get_int32_type(module.get()));
    }else if(node.type == TYPE_FLOAT){
        if(node.isarray)
            param_list.push_back(Type::get_float_ptr_type(module.get()));
        else
            param_list.push_back(Type::get_float_type(module.get()));
    }else{
        throw "void type only allowed for function results";
    }
}

void CminusfBuilder::visit(ASTCompoundStmt &node) {
    scope.enter();
    for(auto var_decl: node.local_declarations){
        var_decl->accept(*this);
    }
    for(auto statement: node.statement_list){
        statement->accept(*this);
    }
    scope.exit();
}

void CminusfBuilder::visit(ASTExpressionStmt &node) { 
    node.expression->accept(*this);
    // 未用到表的达式返回值，也将其弹出栈
    type_stack.pop();
    bottom_up_stack.pop();
}

void CminusfBuilder::visit(ASTSelectionStmt &node) {
    node.expression->accept(*this);
    // 运算结果需要压入栈中
    LoadFromPointerIfNeeded(builder);
    Value* expression_val = bottom_up_stack.top();
    int expression_type = type_stack.top();
    bottom_up_stack.pop();
    type_stack.pop();
    // Array/Void类型检查
    if(expression_type & CM_VOID || expression_type & CM_VOID){
        throw "void/array type can is not compareable";;
    }
    // 恒成立/不成立情况判断
    if(expression_type & CM_CONST){
        bool is_true;
        if(expression_type & CM_INT){
            is_true = dynamic_cast<ConstantInt*>(expression_val)->get_value() != 0;
        }else if(expression_type & CM_FLOAT){
            is_true = dynamic_cast<ConstantFP*>(expression_val)->get_value() != 0;
        }
        if(is_true){
            node.if_statement->accept(*this);
        }else{
            if(node.else_statement != nullptr){
                node.else_statement->accept(*this);
            }
        }
        return;
    }
    if(!(expression_type & CM_BOOL)){
        // 不是i1类型需要进行转换
        if(expression_type & CM_INT)
            expression_val = builder->create_icmp_ne(expression_val, ConstantInt::get(0, module.get()));
        else
            expression_val = builder->create_fcmp_ne(expression_val, ConstantFP::get(0, module.get()));
    }

    BasicBlock* ifBlock = BasicBlock::create(module.get(), GetNewBlockName(), function);
    BasicBlock* elseBlock = nullptr;
    // 调整了block创建顺序，使生成的代码更好看
    // 预创建一个if语句块后的入口
    if(node.else_statement != nullptr){
        elseBlock = BasicBlock::create(module.get(), GetNewBlockName(), function);
    }
    BasicBlock* finallyBlock = BasicBlock::create(module.get(), GetNewBlockName(), function);
    if(node.else_statement != nullptr){
        builder->create_cond_br(expression_val, ifBlock, elseBlock);
        builder->set_insert_point(elseBlock);
        node.else_statement->accept(*this);
        builder->create_br(finallyBlock);
    }else{
        builder->create_cond_br(expression_val, ifBlock, finallyBlock);
    }
    builder->set_insert_point(ifBlock);
    node.if_statement->accept(*this);
    builder->create_br(finallyBlock);
    builder->set_insert_point(finallyBlock);
}

void CminusfBuilder::visit(ASTIterationStmt &node) {
    BasicBlock* condBlock = BasicBlock::create(module.get(), GetNewBlockName(), function);
    builder->create_br(condBlock);
    builder->set_insert_point(condBlock);
    node.expression->accept(*this);
    LoadFromPointerIfNeeded(builder);
    Value* cond = bottom_up_stack.top();
    int condType = type_stack.top();
    bottom_up_stack.pop();
    type_stack.pop();
    if(condType & CM_VOID || condType & CM_ARRAY){
        throw "void/array type can is not compareable";
    }
    // 恒成立/不成立情况判断
    if(condType & CM_CONST){
        bool is_true;
        if(condType & CM_INT){
            is_true = dynamic_cast<ConstantInt*>(cond)->get_value() != 0;
        }else if(condType & CM_FLOAT){
            is_true = dynamic_cast<ConstantFP*>(cond)->get_value() != 0;
        }
        if(is_true){
            // 无限循环
            // ** 注意：若条件恒成立则判断块必为空，直接利用判断块进行循环即可
            node.statement->accept(*this);
            builder->create_br(condBlock);
            // 后面均是不可达代码，直接丢弃
            builder->set_insert_point(__bb);
        }else{
            // 恒假，当作循环体不存在
        }
        return;
    }
    BasicBlock* whileBlock = BasicBlock::create(module.get(), GetNewBlockName(), function);
    BasicBlock* finallyBlock = BasicBlock::create(module.get(), GetNewBlockName(), function);
    if(!(condType & CM_BOOL)){
        if(condType & CM_INT)
            cond = builder->create_icmp_ne(cond, ConstantInt::get(0, module.get()));
        else
            cond = builder->create_fcmp_ne(cond, ConstantFP::get(0, module.get()));
        
    }
    builder->create_cond_br(cond, whileBlock, finallyBlock);
    builder->set_insert_point(whileBlock);
    node.statement->accept(*this);
    builder->create_br(condBlock);
    builder->set_insert_point(finallyBlock);
}

void CminusfBuilder::visit(ASTReturnStmt &node) { 
    if(function->get_return_type()->is_void_type() && node.expression != nullptr){
        throw "void function '" + function->get_name() + "' should not return a value";
    }else if(!function->get_return_type()->is_void_type() && node.expression == nullptr){
        throw "non-void function '" + function->get_name() +"' should return a value";
    }
    if(node.expression){
        node.expression->accept(*this);
        LoadFromPointerIfNeeded(builder);
        Value* expr_val = bottom_up_stack.top();
        int expr_type = type_stack.top();
        if(expr_type & CM_ARRAY || expr_type & CM_VOID){
            throw "function should not return void/array type";
        }
        if (expr_type & CM_BOOL){
            expr_val = builder->create_zext(expr_val, Type::get_int32_type(module.get()));
            expr_type = (expr_type & (~CM_BOOL)) | CM_INT;
        }
        if(function->get_return_type()->is_integer_type() && (expr_type & CM_FLOAT)){
            expr_val = builder->create_fptosi(expr_val, Type::get_int32_type(module.get()));
        }else if(function->get_return_type()->is_float_type() && (expr_type & CM_INT)){
            expr_val = builder->create_sitofp(expr_val, Type::get_float_type(module.get()));
        }
        builder->create_ret(expr_val);
        bottom_up_stack.pop();
        type_stack.pop();
        builder->set_insert_point(__bb);
    }else{
        builder->create_void_ret();
        builder->set_insert_point(__bb);
    }
    
}

void CminusfBuilder::visit(ASTVar &node) { 
    Value* var = scope.find(node.id);
    if(!var){
        throw "use of undeclared identifier '" + node.id + '\'';
    }
    if(dynamic_cast<Function*>(var)){
        throw "non-object type is not assignable";
    }
    int var_t = Val2CM_TYPE(var->get_type());
    if(!(var_t & CM_ARRAY) && node.expression != nullptr){
        throw "subscripted value is not an array";
    }
    if(node.expression){
        node.expression->accept(*this);
        LoadFromPointerIfNeeded(builder);
        Value* subscrip = bottom_up_stack.top();
        int subscripType = type_stack.top();
        bottom_up_stack.pop();
        type_stack.pop();
        // INT 类型检查
        // 修正: 改为自动类型转换，转换不了的才报错
        if(subscripType & (CM_ARRAY | CM_VOID)){
            throw "index of array should be an integer";
        }
        if(subscripType & CM_CONST){
            // 小于0判断（编译时）
            // 注：若编译时能判断，即代表数组下标为常数，则跳过运行时判断，减少冗余代码
            float sub_val;
            if(subscripType & CM_INT){
                sub_val = dynamic_cast<ConstantInt*>(subscrip)->get_value();
            }else{
                sub_val = dynamic_cast<ConstantFP*>(subscrip)->get_value();
            }
            subscrip = ConstantInt::get((int)sub_val, module.get());
            if(sub_val < 0){
                LOG(WARNING) << "Negtive array index detected!";
                builder->create_call(scope.find("neg_idx_except"), {});
                // 该语句块在该语句后的所有代码不可达，直接ret后导入黑洞中
                if(function->get_return_type()->is_void_type()){
                    builder->create_void_ret();
                }else if(function->get_return_type()->is_integer_type()){
                    builder->create_ret(ConstantInt::get(0, module.get()));
                }else{
                    builder->create_ret(ConstantFP::get(0, module.get()));
                }
                builder->set_insert_point(__bb);
            }
        }else{
            // 小于0判断（运行时）
            BasicBlock *err_cond = BasicBlock::create(module.get(), GetNewBlockName(), function);
            BasicBlock *normal_cond = BasicBlock::create(module.get(), GetNewBlockName(), function);
            if(subscripType & CM_FLOAT){
                subscrip = builder->create_fptosi(subscrip, Type::get_int32_type(module.get()));
            }
            CmpInst *cond = builder->create_icmp_lt(subscrip, ConstantInt::get(0, module.get()));
            builder->create_cond_br(cond, err_cond, normal_cond);
            builder->set_insert_point(err_cond);
            builder->create_call(scope.find("neg_idx_except"), {});
            // 本来应该是输出一个unreachable，但是LightIR里面未实现这个，换成无条件跳转，实际上此时程序已经退出了
            builder->create_br(normal_cond);
            builder->set_insert_point(normal_cond);
        }
        if(var_t & CM_PARAM){
            auto ptr = builder->create_load(var);
            bottom_up_stack.push(builder->create_gep(ptr, {subscrip,}));
        }else{
            bottom_up_stack.push(builder->create_gep(var, {ConstantInt::get(0, module.get()), subscrip}));
        }
        type_stack.push((var_t & (~CM_ARRAY)));
    }else
    {
        bottom_up_stack.push(var);
        type_stack.push(var_t);
    }
    
}

void CminusfBuilder::visit(ASTAssignExpression &node) { 
    node.var->accept(*this);
    Value* store_ptr = bottom_up_stack.top();
    bottom_up_stack.pop();
    int var_type = type_stack.top();
    type_stack.pop();
    node.expression->accept(*this);
    LoadFromPointerIfNeeded(builder);
    Value* store_val = bottom_up_stack.top();
    int val_type = type_stack.top();
    bottom_up_stack.pop();
    type_stack.pop();
    if(val_type & CM_VOID || val_type & CM_ARRAY){
        throw "void/array type can not be assigned to other variable";
    }
    // Start expression 常数优化
    if(val_type & CM_CONST){
        if(val_type & CM_INT && var_type & CM_FLOAT){
            val_type = (val_type & (~CM_INT)) | CM_FLOAT;
            ConstantInt* value = dynamic_cast<ConstantInt*>(store_val);
            int numeric = value->get_value();
            store_val = ConstantFP::get((float)numeric, module.get());
        }else if(val_type & CM_FLOAT && var_type & CM_INT){
            val_type = (val_type & (~CM_FLOAT)) | CM_INT;
            ConstantFP* value = dynamic_cast<ConstantFP*>(store_val);
            float numeric = value->get_value();
            store_val = ConstantInt::get((int)numeric, module.get());
        }
    }
    // End expression常数优化
    else if(val_type != var_type){
        
        if(val_type & CM_BOOL){
            val_type = (val_type & (~CM_BOOL)) | CM_INT;
            store_val = builder->create_zext(store_val, Type::get_int32_type(module.get()));
        }
        if((val_type & CM_INT) && (var_type & CM_FLOAT)){
            val_type = (val_type & (~CM_INT)) | CM_FLOAT;
            store_val = builder->create_sitofp(store_val, Type::get_float_type(module.get()));
        }else if((val_type & CM_FLOAT) && (var_type & CM_INT)){
            val_type = (val_type & (~CM_INT)) | CM_FLOAT;
            store_val = builder->create_fptosi(store_val, Type::get_int32_type(module.get()));
        }
        
    }
    bottom_up_stack.push(store_val);
    type_stack.push(val_type);
    builder->create_store(store_val, store_ptr);
}

void CminusfBuilder::visit(ASTSimpleExpression &node) { 
    node.additive_expression_l->accept(*this);
    if(node.additive_expression_r == nullptr){
        return;
    }else{
        LoadFromPointerIfNeeded(builder);
        Value* l_val = bottom_up_stack.top();
        int l_type = type_stack.top();
        bottom_up_stack.pop();
        type_stack.pop();
        node.additive_expression_r->accept(*this);
        LoadFromPointerIfNeeded(builder);
        Value* r_val = bottom_up_stack.top();
        int r_type = type_stack.top();
        if(l_type & CM_ARRAY || l_type & CM_VOID || r_type & CM_ARRAY || r_type & CM_VOID){
            throw "void/array type is not compareable";
        }
        bottom_up_stack.pop();
        type_stack.pop();
        if(l_type & CM_CONST && r_type & CM_CONST){
            float l_v, r_v;
            if(l_type & CM_INT){
                l_v = dynamic_cast<ConstantInt*>(l_val)->get_value();
            }else{
                l_v = dynamic_cast<ConstantFP*>(l_val)->get_value();
            }
            if(r_type & CM_INT){
                r_v = dynamic_cast<ConstantInt*>(r_val)->get_value();
            }else{
                r_v = dynamic_cast<ConstantFP*>(r_val)->get_value();
            }
            switch (node.op)
            {
            case OP_LE:
                bottom_up_stack.push(ConstantInt::get(l_v <= r_v, module.get()));
                break;
            case OP_LT:
                bottom_up_stack.push(ConstantInt::get(l_v < r_v, module.get()));
                break;
            case OP_GT:
                bottom_up_stack.push(ConstantInt::get(l_v > r_v, module.get()));
                break;
            case OP_GE:
                bottom_up_stack.push(ConstantInt::get(l_v >= r_v, module.get()));
                break;
            case OP_EQ:
                bottom_up_stack.push(ConstantInt::get(l_v == r_v, module.get()));
                break;
            case OP_NEQ:
                bottom_up_stack.push(ConstantInt::get(l_v != r_v, module.get()));
                break;
            default:
                throw "Unknown relop type";
            }
            type_stack.push(CM_INT | CM_CONST);
            return;
        }
        
        if(l_type & (CM_INT | CM_BOOL) && r_type & (CM_INT | CM_BOOL)){
            if(l_type & CM_BOOL){
                l_val = builder->create_zext(l_val, Type::get_int32_type(module.get()));
            }
            if(r_type & CM_BOOL){
                r_val = builder->create_zext(r_val, Type::get_int32_type(module.get()));
            }
            switch (node.op)
            {
            case OP_LE:
                bottom_up_stack.push(builder->create_icmp_le(l_val, r_val));
                break;
            case OP_LT:
                bottom_up_stack.push(builder->create_icmp_lt(l_val, r_val));
                break;
            case OP_GT:
                bottom_up_stack.push(builder->create_icmp_gt(l_val, r_val));
                break;
            case OP_GE:
                bottom_up_stack.push(builder->create_icmp_ge(l_val, r_val));
                break;
            case OP_EQ:
                bottom_up_stack.push(builder->create_icmp_eq(l_val, r_val));
                break;
            case OP_NEQ:
                bottom_up_stack.push(builder->create_icmp_ne(l_val, r_val));
                break;
            default:
                throw "Unknown relop type";
            }
        }else{
            if(l_type & (CM_INT | CM_BOOL)){
                if(l_type & CM_BOOL){
                    l_val = builder->create_zext(l_val, Type::get_int32_type(module.get()));
                }
                l_val = builder->create_sitofp(l_val, Type::get_float_type(module.get()));
            }
            if(r_type & (CM_INT | CM_BOOL)){
                if(r_type & CM_BOOL){
                    r_val = builder->create_zext(r_val, Type::get_int32_type(module.get()));
                }
                r_val = builder->create_sitofp(r_val, Type::get_float_type(module.get()));
            }
            switch (node.op)
            {
            case OP_LE:
                bottom_up_stack.push(builder->create_fcmp_le(l_val, r_val));
                break;
            case OP_LT:
                bottom_up_stack.push(builder->create_fcmp_lt(l_val, r_val));
                break;
            case OP_GT:
                bottom_up_stack.push(builder->create_fcmp_gt(l_val, r_val));
                break;
            case OP_GE:
                bottom_up_stack.push(builder->create_fcmp_ge(l_val, r_val));
                break;
            case OP_EQ:
                bottom_up_stack.push(builder->create_fcmp_eq(l_val, r_val));
                break;
            case OP_NEQ:
                bottom_up_stack.push(builder->create_fcmp_ne(l_val, r_val));
                break;
            default:
                throw "Unknown relop type";
            }
        }
        type_stack.push(CM_BOOL);
    }
}

void CminusfBuilder::visit(ASTAdditiveExpression &node) { 
    node.term->accept(*this);
    if(node.additive_expression == nullptr){
        return;
    }else{
        LoadFromPointerIfNeeded(builder);
        Value* term_val = bottom_up_stack.top();
        int term_ty = type_stack.top();
        bottom_up_stack.pop();
        type_stack.pop();
        node.additive_expression->accept(*this);
        LoadFromPointerIfNeeded(builder);
        Value* addi_val = bottom_up_stack.top();
        int addi_ty = type_stack.top();
        bottom_up_stack.pop();
        type_stack.pop();
        if(addi_ty & CM_VOID || term_ty & CM_VOID || addi_ty & CM_ARRAY || term_ty & CM_ARRAY){
            throw "void/array type is not caculatable";
        }
        // expression 常数优化，若为比较常量则会自动存储为ConstantInt类型，不考虑i1情况
        if(term_ty & CM_CONST && addi_ty & CM_CONST){
            if(term_ty & CM_INT && addi_ty & CM_INT){
                int t_val = dynamic_cast<ConstantInt*>(term_val)->get_value();
                int a_val = dynamic_cast<ConstantInt*>(addi_val)->get_value();
                if(node.op == OP_PLUS){
                    bottom_up_stack.push(ConstantInt::get(t_val + a_val, module.get()));
                }else{
                    bottom_up_stack.push(ConstantInt::get(a_val - t_val, module.get()));
                }
                type_stack.push(CM_INT | CM_CONST);
            }else{
                float t_val, a_val;
                if(term_ty & CM_INT){
                    t_val = dynamic_cast<ConstantInt*>(term_val)->get_value();
                }else{
                    t_val = dynamic_cast<ConstantFP*>(term_val)->get_value();
                }
                if(addi_ty & CM_INT){
                    a_val = dynamic_cast<ConstantInt*>(addi_val)->get_value();
                }else{
                    a_val = dynamic_cast<ConstantFP*>(addi_val)->get_value();
                }
                if(node.op == OP_PLUS){
                    bottom_up_stack.push(ConstantFP::get(t_val + a_val, module.get()));
                }else{
                    bottom_up_stack.push(ConstantFP::get(a_val - t_val, module.get()));
                }
                type_stack.push(CM_FLOAT | CM_CONST);
            }
            return;
        }
        // End 常数化优化
        int finalType;
        if(term_ty & CM_BOOL){
            term_val = builder->create_zext(term_val, Type::get_int32_type(module.get()));
            term_ty = (term_ty & (~CM_BOOL)) | CM_INT;
        }
        if(addi_ty == CM_BOOL){
            addi_val = builder->create_zext(addi_val, Type::get_int32_type(module.get()));
            addi_ty = (addi_ty & (~CM_BOOL)) | CM_INT;
        }
        if(term_ty & CM_FLOAT || addi_ty & CM_FLOAT){
            finalType = CM_FLOAT;
            if(term_ty & CM_INT){
                term_val = builder->create_sitofp(term_val, Type::get_float_type(module.get()));
            }
            if(addi_ty & CM_INT){
                addi_val = builder->create_sitofp(addi_val, Type::get_float_type(module.get()));
            }
            if(node.op == OP_PLUS){
                bottom_up_stack.push(builder->create_fadd(addi_val, term_val));
            }else{
                bottom_up_stack.push(builder->create_fsub(addi_val, term_val));
            }
        }else{
            finalType = CM_INT;
            if(node.op == OP_PLUS){
                bottom_up_stack.push(builder->create_iadd(addi_val, term_val));
            }else{
                bottom_up_stack.push(builder->create_isub(addi_val, term_val));
            }
        }
        type_stack.push(finalType);
    }
}

void CminusfBuilder::visit(ASTTerm &node) { 
    node.factor->accept(*this);
    if(node.term == nullptr){
        return;
    }else{
        LoadFromPointerIfNeeded(builder);
        Value* factor_val = bottom_up_stack.top();
        int factor_ty = type_stack.top();
        bottom_up_stack.pop();
        type_stack.pop();
        node.term->accept(*this);
        LoadFromPointerIfNeeded(builder);
        Value* term_val = bottom_up_stack.top();
        int term_ty = type_stack.top();
        bottom_up_stack.pop();
        type_stack.pop();
        if(factor_ty & CM_VOID || term_ty & CM_VOID || factor_ty & CM_ARRAY || term_ty & CM_ARRAY){
            throw "void/array type is not caculatable";
        }
        // expression 常数优化，若为比较常量则会自动存储为ConstantInt类型，不考虑i1情况
        if(term_ty & CM_CONST && factor_ty & CM_CONST){
            if(term_ty & CM_INT && factor_ty & CM_INT){
                int f_val = dynamic_cast<ConstantInt*>(factor_val)->get_value();
                int t_val = dynamic_cast<ConstantInt*>(term_val)->get_value();
                
                if(node.op == OP_MUL){
                    bottom_up_stack.push(ConstantInt::get(t_val * f_val, module.get()));
                    type_stack.push(CM_INT | CM_CONST);
                }else{
                    if(f_val == 0){
                        LOG_WARNING << "division by zero error (compile time)";
                        bottom_up_stack.push(builder->create_isdiv(term_val, factor_val));
                        type_stack.push(CM_INT);
                    }else{
                        bottom_up_stack.push(ConstantInt::get(t_val / f_val, module.get()));
                        type_stack.push(CM_INT | CM_CONST);
                    }
                }
            }else{
                float t_val, f_val;
                if(term_ty & CM_INT){
                    t_val = dynamic_cast<ConstantInt*>(term_val)->get_value();
                }else{
                    t_val = dynamic_cast<ConstantFP*>(term_val)->get_value();
                }
                if(factor_ty & CM_INT){
                    f_val = dynamic_cast<ConstantInt*>(factor_val)->get_value();
                }else{
                    f_val = dynamic_cast<ConstantFP*>(factor_val)->get_value();
                }
                if(node.op == OP_MUL){
                    bottom_up_stack.push(ConstantFP::get(t_val * f_val, module.get()));
                    type_stack.push(CM_FLOAT | CM_CONST);
                }else{
                    if(f_val == 0){
                        LOG_WARNING << "division by zero error (compile time)";
                        bottom_up_stack.push(builder->create_fdiv(ConstantFP::get(t_val, module.get()), ConstantFP::get(f_val, module.get())));
                        type_stack.push(CM_FLOAT);
                    }else{
                        bottom_up_stack.push(ConstantFP::get(t_val / f_val, module.get()));
                        type_stack.push(CM_FLOAT | CM_CONST);
                    }
                }
                
            }
            return;
        }
        // End 常数化优化

        int finalType;
        if(term_ty & CM_BOOL){
            term_val = builder->create_zext(term_val, Type::get_int32_type(module.get()));
            term_ty = (term_ty & (~CM_BOOL)) | CM_INT;
        }
        if(factor_ty & CM_BOOL){
            factor_val = builder->create_zext(factor_val, Type::get_int32_type(module.get()));
            factor_ty = (factor_ty & (~CM_BOOL)) | CM_INT;
        }
        if(term_ty & CM_FLOAT || factor_ty & CM_FLOAT){
            finalType = CM_FLOAT;
            if(term_ty & CM_INT){
                term_val = builder->create_sitofp(term_val, Type::get_float_type(module.get()));
            }
            if(factor_ty & CM_INT){
                factor_val = builder->create_sitofp(factor_val, Type::get_float_type(module.get()));
            }
            if(node.op == OP_MUL){
                bottom_up_stack.push(builder->create_fmul(term_val, factor_val));
            }else{
                bottom_up_stack.push(builder->create_fdiv(term_val, factor_val));
            }
        }else{
            finalType = CM_INT;
            if(node.op == OP_MUL){
                bottom_up_stack.push(builder->create_imul(term_val, factor_val));
            }else{
                bottom_up_stack.push(builder->create_isdiv(term_val, factor_val));
            }
        }
        type_stack.push(finalType);
    }
}

void CminusfBuilder::visit(ASTCall &node) { 
    Value* func = scope.find(node.id);
    if(func == nullptr){
        throw "Use of undeclared identifier '" + node.id + '\'';;
    }
    Function* c_func = dynamic_cast<Function*>(func);
    if(c_func == nullptr){
        throw "called object type is not a function";
    }
    int ret_t = Val2CM_TYPE(func->get_type());
    std::vector<Value*> params;
    if(node.args.size() != c_func->get_args().size()){
        throw "function missing required positional argument";
    }
    int i = 0;
    for(auto paramType: c_func->get_args()){
        auto expr = node.args[i++];
        expr->accept(*this);
        Value* exprVal = bottom_up_stack.top();
        int exprType = type_stack.top();
        if(paramType->get_type()->is_pointer_type()){
            if(!(exprType & CM_ARRAY)){
                throw "array type is required by param";
            }
            if(exprType & CM_PARAM){
                LoadInst *ptr = builder->create_load(exprVal);
                exprVal = ptr;
            }else{
                GetElementPtrInst *ptr = builder->create_gep(exprVal, {ConstantInt::get(0, module.get()), ConstantInt::get(0, module.get())});
                exprVal = ptr;
            }
        }
        else{
            LoadFromPointerIfNeeded(builder);
            exprVal = bottom_up_stack.top();
            if(exprType & CM_BOOL){
                exprVal = builder->create_zext(exprVal, Type::get_int32_type(module.get()));
                exprType = (exprType & (~CM_BOOL)) | CM_INT;
            }
            if(exprType & CM_INT && paramType->get_type()->is_float_type()){
                exprVal = builder->create_sitofp(exprVal, Type::get_float_type(module.get()));
            }else if(exprType & CM_FLOAT && paramType->get_type()->is_integer_type()){
                exprVal = builder->create_fptosi(exprVal, Type::get_int32_type(module.get()));
            }
        }
        params.push_back(exprVal);
        bottom_up_stack.pop();
        type_stack.pop();
    }
    CallInst* ret_val = builder->create_call(c_func, params);
    bottom_up_stack.push(ret_val);
    type_stack.push(ret_t);
}
