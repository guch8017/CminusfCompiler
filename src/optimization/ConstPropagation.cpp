#include "ConstPropagation.hpp"
#include "logging.hpp"

#define IS_BINARY(ptr) (dynamic_cast<BinaryInst*>(ptr) != nullptr)
#define IS_CONST_INT(ptr) (dynamic_cast<ConstantInt*>(ptr) != nullptr)  // Bool常量也在这
#define IS_CONST_FP(ptr) (dynamic_cast<ConstantFP*>(ptr) != nullptr)
#define CONTAIN(set, val) (set.find(val) != set.end())


/**
 * 处理整数四则运算
 * 
 */
ConstantInt *compute_binary(
    Instruction::OpID op,
    Value *value1,
    Value *value2,
    Module *module_)
{

    int c_value1 = dynamic_cast<ConstantInt*>(value1)->get_value();
    int c_value2 = dynamic_cast<ConstantInt*>(value2)->get_value();
    switch (op)
    {
    case Instruction::add:
        return ConstantInt::get(c_value1 + c_value2, module_);

        break;
    case Instruction::sub:
        return ConstantInt::get(c_value1 - c_value2, module_);
        break;
    case Instruction::mul:
        return ConstantInt::get(c_value1 * c_value2, module_);
        break;
    case Instruction::sdiv:
        return ConstantInt::get((int)(c_value1 / c_value2), module_);
        break;
    default:
        return nullptr;
        break;
    }
}

/**
 * 处理浮点数四则运算
 */
ConstantFP *compute_f_binary(
    Instruction::OpID op,
    Value *value1,
    Value *value2,
    Module *module_)
{

    float c_value1 = dynamic_cast<ConstantFP*>(value1)->get_value();
    float c_value2 = dynamic_cast<ConstantFP*>(value2)->get_value();
    switch (op)
    {
    case Instruction::fadd:
        return ConstantFP::get(c_value1 + c_value2, module_);

        break;
    case Instruction::fsub:
        return ConstantFP::get(c_value1 - c_value2, module_);
        break;
    case Instruction::fmul:
        return ConstantFP::get(c_value1 * c_value2, module_);
        break;
    case Instruction::fdiv:
        return ConstantFP::get(c_value1 / c_value2, module_);
        break;
    default:
        return nullptr;
        break;
    }
}

/**
 * 处理整型比较
 */
ConstantInt *compute_compare(CmpInst::CmpOp op, Value* lhs, Value* rhs,  Module* module_){
    int lhs_v = dynamic_cast<ConstantInt*>(lhs)->get_value();
    int rhs_v = dynamic_cast<ConstantInt*>(rhs)->get_value();
    bool result;
    switch (op)
    {
    case CmpInst::EQ:
        result = lhs_v == rhs_v;
        break;
    case CmpInst::NE:
        result = lhs_v != rhs_v;
        break;
    case CmpInst::LE:
        result = lhs_v <= rhs_v;
        break;
    case CmpInst::LT:
        result = lhs_v < rhs_v;
        break;
    case CmpInst::GE:
        result = lhs_v >= rhs_v;
        break;
    case CmpInst::GT:
        result = lhs_v > rhs_v;
        break;
    default:
        return nullptr;
    }
    return ConstantInt::get(result, module_);
}

/**
 * 处理浮点型比较
 */
ConstantInt *compute_f_compare(FCmpInst::CmpOp op, Value* lhs, Value* rhs,  Module* module_){
    float lhs_v = dynamic_cast<ConstantFP*>(lhs)->get_value();
    float rhs_v = dynamic_cast<ConstantFP*>(rhs)->get_value();
    bool result;
    switch (op)
    {
    case FCmpInst::EQ:
        result = lhs_v == rhs_v;
        break;
    case FCmpInst::NE:
        result = lhs_v != rhs_v;
        break;
    case FCmpInst::LE:
        result = lhs_v <= rhs_v;
        break;
    case FCmpInst::LT:
        result = lhs_v < rhs_v;
        break;
    case FCmpInst::GE:
        result = lhs_v >= rhs_v;
        break;
    case FCmpInst::GT:
        result = lhs_v > rhs_v;
        break;
    default:
        return nullptr;
    }
    return ConstantInt::get(result, module_);
}

void ConstPropagation::run()
{
    for(Function* func: m_->get_functions()){
        if(func->get_num_basic_blocks() == 0) continue;
        std::set<BasicBlock*> visitedBB;
        std::queue<BasicBlock*> bbQueue;
        bbQueue.push(func->get_entry_block());
        visitedBB.insert(func->get_entry_block());
        // 常量折叠
        while (!bbQueue.empty())
        {
            BasicBlock* bb = bbQueue.front();
            bbQueue.pop();
            if(bb->empty()) continue;
            std::set<Instruction*> instrToBeDelete;
            std::unordered_map<Value*, std::vector<Value*>> pointerConst;
            for(Instruction* instr: bb->get_instructions()){
                if(IS_BINARY(instr)){
                    // 整数/浮点数四则运算
                    BinaryInst* inst = dynamic_cast<BinaryInst*>(instr);
                    Instruction::OpID op = instr->get_instr_type();
                    Value* lhs = inst->get_operand(0);
                    Value* rhs = inst->get_operand(1);
                    if(IS_CONST_INT(lhs) && IS_CONST_INT(rhs)){
                        inst->replace_all_use_with(compute_binary(op, lhs, rhs, m_));
                        instrToBeDelete.insert(inst);
                    }else if(IS_CONST_FP(lhs) && IS_CONST_FP(rhs)){
                        inst->replace_all_use_with(compute_f_binary(op, lhs, rhs, m_));
                        instrToBeDelete.insert(inst);
                    }
                }else if(instr->is_zext()){
                    // Zext扩展指令
                    if(IS_CONST_INT(instr->get_operand(0))){
                        instr->replace_all_use_with(ConstantInt::get(dynamic_cast<ConstantInt*>(instr->get_operand(0))->get_value(), m_));
                        instrToBeDelete.insert(instr);
                    }
                }else if(instr->is_cmp()){
                    // 整型比较
                    Value* lhs = instr->get_operand(0);
                    Value* rhs = instr->get_operand(1);
                    if(IS_CONST_INT(lhs) &&  IS_CONST_INT(rhs)){
                        instr->replace_all_use_with(compute_compare(dynamic_cast<CmpInst*>(instr)->get_cmp_op(),  lhs, rhs, m_));
                        instrToBeDelete.insert(instr);
                    }
                }else if(instr->is_fcmp()){
                    // 浮点比较
                    Value* lhs = instr->get_operand(0);
                    Value* rhs = instr->get_operand(1);
                    if(IS_CONST_FP(lhs) &&  IS_CONST_FP(rhs)){
                        instr->replace_all_use_with(compute_f_compare(dynamic_cast<FCmpInst*>(instr)->get_cmp_op(),  lhs, rhs, m_));
                        instrToBeDelete.insert(instr);
                    }
                }else if(instr->is_si2fp()){
                    if(IS_CONST_INT(instr->get_operand(0))){
                        instr->replace_all_use_with(ConstantFP::get(dynamic_cast<ConstantInt*>(instr->get_operand(0))->get_value(), m_));
                        instrToBeDelete.insert(instr);
                    }
                }else if(instr->is_fp2si()){
                    if(IS_CONST_FP(instr->get_operand(0))){
                        instr->replace_all_use_with(ConstantInt::get((int)dynamic_cast<ConstantFP*>(instr->get_operand(0))->get_value(), m_));
                        instrToBeDelete.insert(instr);
                    }
                }else if(instr->is_store()){
                    pointerConst[instr->get_operand(1)].push_back(instr->get_operand(0));
                }else if(instr->is_load()){
                    // 若存在，则说明值为刚刚存进去的，直接替换指令
                    if(CONTAIN(pointerConst, instr->get_operand(0))){
                        instr->replace_all_use_with(pointerConst[instr->get_operand(0)].back());
                    }
                    // 否则，将值存入栈，待后序使用
                    else{
                        pointerConst[instr->get_operand(0)].push_back(instr);
                    }
                }
                
                else if(instr->is_br()){
                    // 跳转指令，附带消除常量跳转
                    BranchInst* brInstr =  dynamic_cast<BranchInst*>(instr);
                    if(brInstr->is_cond_br()){
                        Value* cond = brInstr->get_operand(0);
                        BasicBlock* trueBB = dynamic_cast<BasicBlock*>(brInstr->get_operand(1));
                        BasicBlock* falseBB = dynamic_cast<BasicBlock*>(brInstr->get_operand(2));
                        if(IS_CONST_INT(cond) || IS_CONST_FP(cond)){
                            bool _tar;
                            if(IS_CONST_INT(cond)){
                                _tar  = dynamic_cast<ConstantInt*>(cond)->get_value() != 0;
                            }else{
                                _tar  = dynamic_cast<ConstantFP*>(cond)->get_value() != 0;
                            }
                            if(_tar){
                                // 恒成立跳转
                                brInstr->replace_all_use_with(BranchInst::create_br(trueBB,  bb));
                                if(!CONTAIN(visitedBB, trueBB)){
                                    visitedBB.insert(trueBB);
                                    bbQueue.push(trueBB);
                                }
                            }else{
                                // 恒不成立跳转
                                brInstr->replace_all_use_with(BranchInst::create_br(falseBB,  bb));
                                if(!CONTAIN(visitedBB, falseBB)){
                                    visitedBB.insert(falseBB);
                                    bbQueue.push(falseBB);
                                }
                            }
                            instrToBeDelete.insert(instr);
                        }else{
                            // Condition不是常量
                            if(!CONTAIN(visitedBB, trueBB)){
                                visitedBB.insert(trueBB);
                                bbQueue.push(trueBB);
                            }
                            if(!CONTAIN(visitedBB, falseBB)){
                                visitedBB.insert(falseBB);
                                bbQueue.push(falseBB);
                            }
                        }
                    }else{
                        // 无条件跳转
                        BasicBlock* trueBB = dynamic_cast<BasicBlock*>(brInstr->get_operand(0));
                        if(!CONTAIN(visitedBB, trueBB)){
                              visitedBB.insert(trueBB);
                              bbQueue.push(trueBB);
                          }
                    }

                }
            }
            // 删除多余的指令
            for(Instruction* instr: instrToBeDelete){
                bb->delete_instr(instr);
            }
        }
        // 删除死代码
        // 完全没有到达该代码块的路径，删掉
        std::set<BasicBlock*> tbdBB;
        for(BasicBlock* bb: func->get_basic_blocks()){
            if(!CONTAIN(visitedBB, bb)){
                tbdBB.insert(bb);
            }
        }
        for(BasicBlock* bb: tbdBB){
            // 删除Phi指令中的对应项，修复Phi指令的正确性
            // 此处需要更新Phi函数引用的路径。对于被删除的路径后的所有Path均需要更新arg no，否则会导致使用者链异常
            // TODO：入口为空时的处理？单个Phi指令替换成常量？
            for(auto instr: bb->get_instructions()){
                bb->delete_instr(instr);
            }
            auto l = bb->get_use_list();
            for(Use use: l){
                use.val_->remove_use(bb);
                PhiInst *pinstr = dynamic_cast<PhiInst*>(use.val_);
                if(pinstr != nullptr){
                    int indTBR = -1;
                    for(int i = 0; i < pinstr->get_operands().size(); ++i){
                        if(pinstr->get_operands()[i] == bb){
                            indTBR = i;
                            for(int j = i + 2; j < pinstr->get_operands().size(); j += 1){
                                pinstr->get_operands()[j]->remove_use(pinstr);
                                pinstr->get_operands()[j]->add_use(pinstr, j - 2);
                            }
                            break;
                        }
                    }
                    if(indTBR > 0){
                        pinstr->remove_operands(indTBR - 1, indTBR);
                    }
                    
                }
            }
            func->remove(bb);
        }
        
        // 只有一条无条件跳转指令的代码块，合并
        bool changed = true;
        while (changed)
        {
            changed = false;
            std::set<BasicBlock*> tbdBB;
            // 如果入口块是单跳转指令，删掉一以后顺序会乱，故进行调整
            BasicBlock* newEntry = nullptr;
            for(BasicBlock* bb: func->get_basic_blocks()){
                if(bb == func->get_entry_block()) continue;  // TODO: 当前版本无法修改顺序，先跳过基本块进行处理
                if(bb->get_instructions().size() == 1){
                    BranchInst* instr = dynamic_cast<BranchInst*>(bb->get_instructions().front());
                    if(instr != nullptr && !instr->is_cond_br()){
                        BasicBlock* targetBB = dynamic_cast<BasicBlock*>(instr->get_operand(0));
                        bb->replace_all_use_with(targetBB);
                        tbdBB.insert(bb);
                        changed = true;
                        if(func->get_entry_block() == bb || newEntry == bb){
                            newEntry = targetBB;
                        }
                    }
                }
            }
            for(BasicBlock* dBB: tbdBB){
                func->remove(dBB);
            }
            // 调整顺序，不调用Function::remove防止前驱后继关系被破坏
            // TODO: 调整顺序。当前API无法满足该需求
            if(newEntry != nullptr){
                func->get_basic_blocks().remove(newEntry);
                func->get_basic_blocks().push_front(newEntry);
            }
        }
        
    }
}