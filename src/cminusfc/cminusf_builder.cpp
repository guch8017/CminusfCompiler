#include "cminusf_builder.hpp"
#include <stack>


// You can define global variables here
// to store state

class TypeScope {
    /**
     * 重写的Scope，用于进行类型计算
     **/
public:
    TypeScope(){
        enter();
        push("input", TYPE_INT, false);
        push("output", TYPE_VOID, false);
        push("outputFloat", TYPE_VOID, false);
        push("neg_idx_except", TYPE_VOID, false);
    }
    void enter() {inner.push_back({});}
    void exit() {inner.pop_back();}
    bool push(std::string name, CminusType ty, bool isArray){
        return push(name, std::make_tuple(ty, isArray));
    }
    bool push(std::string name, std::tuple<CminusType, bool> val) {
        auto result = inner[inner.size() - 1].insert({name, val});
        return result.second;
    }
    std::tuple<CminusType, bool> find(std::string name) {
        for (auto s = inner.rbegin(); s!= inner.rend();s++) {
            auto iter = s->find(name);
            if (iter != s->end()) {
                return iter->second;
            }
        }
        return std::make_tuple(TYPE_VOID, true);
    }
private:
    std::vector<std::map<std::string, std::tuple<CminusType, bool>>> inner;
};

int block_counter = 0;
/// 函数参数类型
/// Type不从Value派生，故无法使用bottom_up_stack向上传递，改用一个vector来传递函数变量类型
std::vector<Type*> param_list;

/// 当前正在处理的函数入
Function* function = nullptr;

/// 这两个栈中的元素保持对应，即类型栈栈顶代表的类型即为值的类型
/// 注意：由于CminusType中没有i1的枚举，故此实现中i1采用TYPE_VOID进行存储
std::stack<Value*> bottom_up_stack;  // 值传递栈
std::stack<CminusType> type_stack;   // 类型栈

/// 根据变量名获取变量的类型
TypeScope t_scope;


/// 黑洞，用于吸收ret后的所有不可达指令，当builder指向此处时，所插入的代码将不会体现在输出的.ll上
/// 当程序遇到ret语句时，在执行完builder->create_(void_)ret后，builder的插入点将会被设置到__bb黑洞处
/// 此操作可以减少部分冗余代码，也可防止不可达代码带来的lli编译问题
Module* blackholeModule = new Module("MOD");
FunctionType* __funcType = FunctionType::get(Type::get_void_type(blackholeModule), {});
Function* __blackholeFunc = Function::create(__funcType, "NONE", blackholeModule);
BasicBlock* __bb = BasicBlock::create(blackholeModule, "bb", __blackholeFunc);

/*
 * use CMinusfBuilder::Scope to construct scopes
 * scope.enter: enter a new scope
 * scope.exit: exit current scope
 * scope.push: add a new binding to current scope
 * scope.find: find and return the value bound to the name
 */

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
        type_stack.push(TYPE_INT);
        break;
    case TYPE_FLOAT:
        bottom_up_stack.push(ConstantFP::get(node.f_val, module.get()));
        type_stack.push(TYPE_FLOAT);
        break;
    default:
        throw "Unknown declaration type";
    }
}

void CminusfBuilder::visit(ASTVarDeclaration &node) { 
    Type* decl_type = GetDeclType(node.type, module.get());
    Type* var_type = nullptr;
    Value* var = nullptr;
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
    t_scope.push(node.id, node.type, node.num != nullptr);
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
    // 知道是函数，后面的isArray随便给个值
    t_scope.push(node.id, node.type, false);
    function = func;
    scope.enter();t_scope.enter();
    int i = 0;
    for(auto arg: function->get_args()){
        if(!scope.push(node.params[i]->id, arg)){
            throw "Redefinition of '" + node.params[i]->id + '\'';
        }
        t_scope.push(node.params[i]->id, node.params[i]->type, node.params[i]->isarray);
        ++i;
    }
    builder->set_insert_point(BasicBlock::create(module.get(), GetNewBlockName(), function));
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
    scope.exit();t_scope.exit();
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
    }
}

void CminusfBuilder::visit(ASTCompoundStmt &node) {
    scope.enter();t_scope.enter();
    for(auto var_decl: node.local_declarations){
        var_decl->accept(*this);
    }
    for(auto statement: node.statement_list){
        statement->accept(*this);
    }
    scope.exit();t_scope.exit();
}

void CminusfBuilder::visit(ASTExpressionStmt &node) { 
    node.expression->accept(*this);
}

void CminusfBuilder::visit(ASTSelectionStmt &node) {
    node.expression->accept(*this);
    // 运算结果需要压入栈中
    Value* expression_val = bottom_up_stack.top();
    CminusType expression_type = type_stack.top();
    if(!dynamic_cast<CmpInst*>(expression_val)){
        // 不是i1类型需要进行转换
        if(expression_type == TYPE_INT)
            expression_val = builder->create_icmp_ne(expression_val, ConstantInt::get(0, module.get()));
        else
            expression_val = builder->create_fcmp_ne(expression_val, ConstantFP::get(0, module.get()));
    }
    bottom_up_stack.pop();
    type_stack.pop();
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
    BasicBlock* whileBlock = BasicBlock::create(module.get(), GetNewBlockName(), function);
    BasicBlock* finallyBlock = BasicBlock::create(module.get(), GetNewBlockName(), function);
    builder->create_br(condBlock);
    builder->set_insert_point(condBlock);
    node.expression->accept(*this);
    Value* cond = bottom_up_stack.top();
    CminusType condType = type_stack.top();
    if(!dynamic_cast<CmpInst*>(cond)){
        if(condType == TYPE_INT)
            cond = builder->create_icmp_ne(cond, ConstantInt::get(0, module.get()));
        else
            cond = builder->create_fcmp_ne(cond, ConstantFP::get(0, module.get()));
        
    }
    bottom_up_stack.pop();
    type_stack.pop();
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
        Value* expr_val = bottom_up_stack.top();
        CminusType expr_type = type_stack.top();
        if(function->get_return_type()->is_integer_type() && expr_type == TYPE_FLOAT){
            expr_val = builder->create_fptosi(expr_val, Type::get_int32_type(module.get()));
        }else if(function->get_return_type()->is_float_type() && expr_type == TYPE_INT){
            expr_val = builder->create_sitofp(expr_val, Type::get_int32_type(module.get()));
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
        throw "Use of undeclared identifier '" + node.id + '\'';
    }
    if(dynamic_cast<Function*>(var)){
        throw "non-object type is not assignable";
    }
    CminusType var_t;
    bool var_is_arr;
    std::tie(var_t, var_is_arr) = t_scope.find(node.id);
    // Type* var_type = var->get_type();
    // printf("%s: %d\n", node.id.c_str(), var_type->get_type_id());
    // 1: 全部为指针类型，对是否指向数组需要额外判断，实验没要求先PASS
    // 2: 加了个TypeScope可以判断了～
    if(!var_is_arr && node.expression != nullptr){
        throw "Subscripted value is not an array";
    }
    if(var_is_arr && node.expression == nullptr){
        throw "Array type is not assignable";
    }
    
    if(node.expression){
        node.expression->accept(*this);
        Value* subscrip = bottom_up_stack.top();
        CminusType subscripType = type_stack.top();
        bottom_up_stack.pop();
        type_stack.pop();
        // INT 类型检查
        if(subscripType != TYPE_INT){
            throw "index of array should be an integer";
        }
        // 小于0判断（运行时）
        BasicBlock *err_cond = BasicBlock::create(module.get(), GetNewBlockName(), function);
        BasicBlock *normal_cond = BasicBlock::create(module.get(), GetNewBlockName(), function);
        CmpInst *cond = builder->create_icmp_lt(subscrip, ConstantInt::get(0, module.get()));
        builder->create_cond_br(cond, err_cond, normal_cond);
        builder->set_insert_point(err_cond);
        builder->create_call(scope.find("neg_idx_except"), {});
        builder->set_insert_point(normal_cond);
        bottom_up_stack.push(builder->create_gep(var, {ConstantInt::get(0, module.get()), subscrip}));
    }else
    {
        bottom_up_stack.push(var);
    }
    type_stack.push(var_t);
}

void CminusfBuilder::visit(ASTAssignExpression &node) { 
    node.var->accept(*this);
    Value* store_ptr = bottom_up_stack.top();
    bottom_up_stack.pop();
    CminusType var_type = type_stack.top();
    type_stack.pop();
    node.expression->accept(*this);
    Value* store_val = bottom_up_stack.top();
    CminusType val_type = type_stack.top();
    if(val_type != var_type){
        bottom_up_stack.pop();
        type_stack.pop();
        if(val_type == TYPE_VOID){
            val_type = TYPE_INT;
            store_val = builder->create_zext(store_val, Type::get_int32_type(module.get()));
        }
        if(val_type == TYPE_INT && var_type == TYPE_FLOAT){
            store_val = builder->create_sitofp(store_val, Type::get_float_type(module.get()));
        }else if(val_type == TYPE_FLOAT && var_type == TYPE_INT){
            store_val = builder->create_fptosi(store_val, Type::get_int32_type(module.get()));
        }
        bottom_up_stack.push(store_val);
        type_stack.push(var_type);
    }
    builder->create_store(store_val, store_ptr);
}

void CminusfBuilder::visit(ASTSimpleExpression &node) { 
    node.additive_expression_l->accept(*this);
    if(node.additive_expression_r == nullptr){
        return;
    }else{
        Value* l_val = bottom_up_stack.top();
        bottom_up_stack.pop();
        node.additive_expression_r->accept(*this);
        Value* r_val = bottom_up_stack.top();
        bottom_up_stack.pop();
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
        type_stack.push(TYPE_VOID);
    }
}

void CminusfBuilder::visit(ASTAdditiveExpression &node) { 
    node.term->accept(*this);
    if(node.additive_expression == nullptr){
        return;
    }else{
        Value* term_val = bottom_up_stack.top();
        CminusType term_ty = type_stack.top();
        bottom_up_stack.pop();
        type_stack.pop();
        node.additive_expression->accept(*this);
        Value* addi_val = bottom_up_stack.top();
        CminusType addi_ty = type_stack.top();
        bottom_up_stack.pop();
        type_stack.pop();
        CminusType finalType;
        if(term_ty == TYPE_VOID){
            term_val = builder->create_zext(term_val, Type::get_int32_type(module.get()));
            term_ty = TYPE_INT;
        }
        if(addi_ty == TYPE_VOID){
            addi_val = builder->create_zext(addi_val, Type::get_int32_type(module.get()));
            addi_ty = TYPE_INT;
        }
        if(term_ty == TYPE_FLOAT || addi_ty == TYPE_FLOAT){
            finalType = TYPE_FLOAT;
            if(term_ty == TYPE_INT){
                term_val = builder->create_sitofp(term_val, Type::get_float_type(module.get()));
            }
            if(addi_ty == TYPE_INT){
                addi_val = builder->create_sitofp(addi_val, Type::get_float_type(module.get()));
            }
            if(node.op == OP_PLUS){
                bottom_up_stack.push(builder->create_fadd(addi_val, term_val));
            }else{
                bottom_up_stack.push(builder->create_fsub(addi_val, term_val));
            }
        }else{
            finalType = TYPE_INT;
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
    Value* factor_val = bottom_up_stack.top();
    if( dynamic_cast<GlobalVariable*>(factor_val) ||
        dynamic_cast<AllocaInst*>(factor_val) ||
        dynamic_cast<GetElementPtrInst*>(factor_val)){
        // 传入为指针，需要读取值
        bottom_up_stack.pop();
        factor_val = builder->create_load(factor_val);
        bottom_up_stack.push(factor_val);
    }
    if(node.term == nullptr){
        return;
    }else{
        Value* factor_val = bottom_up_stack.top();
        CminusType factor_ty = type_stack.top();
        bottom_up_stack.pop();
        type_stack.pop();
        node.term->accept(*this);
        Value* term_val = bottom_up_stack.top();
        CminusType term_ty = type_stack.top();
        bottom_up_stack.pop();
        type_stack.pop();

        CminusType finalType;
        if(term_ty == TYPE_VOID){
            term_val = builder->create_zext(term_val, Type::get_int32_type(module.get()));
            term_ty = TYPE_INT;
        }
        if(factor_ty == TYPE_VOID){
            factor_val = builder->create_zext(factor_val, Type::get_int32_type(module.get()));
            factor_ty = TYPE_INT;
        }
        if(term_ty == TYPE_FLOAT || factor_ty == TYPE_FLOAT){
            finalType = TYPE_FLOAT;
            if(term_ty == TYPE_INT){
                term_val = builder->create_sitofp(term_val, Type::get_float_type(module.get()));
            }
            if(factor_ty == TYPE_INT){
                factor_val = builder->create_sitofp(factor_val, Type::get_float_type(module.get()));
            }
            if(node.op == OP_MUL){
                bottom_up_stack.push(builder->create_fmul(term_val, factor_val));
            }else{
                bottom_up_stack.push(builder->create_fdiv(term_val, factor_val));
            }
        }else{
            finalType = TYPE_INT;
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
    CminusType ret_t;
    bool t;
    std::tie(ret_t, t) = t_scope.find(node.id);
    std::vector<Value*> params;
    if(node.args.size() != c_func->get_args().size()){
        throw "function missing required positional argument";
    }
    int i = 0;
    for(auto paramType: c_func->get_args()){
        auto expr = node.args[i++];
        expr->accept(*this);
        Value* exprVal = bottom_up_stack.top();
        CminusType exprType = type_stack.top();
        if(exprType == TYPE_VOID){
            exprVal = builder->create_zext(exprVal, Type::get_int32_type(module.get()));
            exprType = TYPE_INT;
        }
        if(exprType == TYPE_INT && paramType->get_type()->is_float_type()){
            exprVal = builder->create_sitofp(exprVal, Type::get_float_type(module.get()));
        }else if(exprType == TYPE_FLOAT && paramType->get_type()->is_integer_type()){
            exprVal = builder->create_fptosi(exprVal, Type::get_int32_type(module.get()));
        }
        params.push_back(exprVal);
        bottom_up_stack.pop();
        type_stack.pop();
    }
    CallInst* ret_val = builder->create_call(c_func, params);
    bottom_up_stack.push(ret_val);
    type_stack.push(ret_t);
    
}
