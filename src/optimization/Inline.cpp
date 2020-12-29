#include "Inline.hpp"

int Inline::opID = 0;

void Inline::callAnalysis(){
    for(Function* _func: m_->get_functions()){
        if(_func->is_declaration()) continue;
        int count = 0;
        for(BasicBlock* _bb: _func->get_basic_blocks()){
            count += _bb->get_instructions().size();
            for(Instruction* instr: _bb->get_instructions()){
                if(instr->is_call() && !dynamic_cast<Function*>(instr->get_operand(0))->is_declaration())
                    callInstrHotSpot.push_back({_func, _bb, dynamic_cast<CallInst*>(instr), 1});
            }
        }
        
    }
}

BasicBlock* Inline::splitBasicBlock(BasicBlock* targetBB, CallInst* inst){
    // 抽取函数指针
    Function*  func = targetBB->get_parent();
    // 新的跳转块
    BasicBlock* newBB = BasicBlock::create(func->get_parent(), getName(), func);
    // 获取插入点
    auto it = targetBB->get_instructions().begin();
    auto itt = targetBB->get_instructions().begin();
    auto ed = targetBB->get_instructions().end();
    for(;it != ed && *it != inst;it++,itt++);
    // 无法找到插入点
    if(it == ed) return nullptr;
    itt++;
    // 剪切之后的所有指令到新的BB
    while(itt != ed){
        newBB->get_instructions().push_back(*itt);
        itt++;
    }
    while (it != ed)
    {
        targetBB->get_instructions().pop_back();
        it++;
    }
    // 修正Parent关系
    for(Instruction* inst: newBB->get_instructions()){
        inst->set_parent(newBB);
    }
    // 获取插入块
    BasicBlock* inlineEntryBlock = transplant(dynamic_cast<Function*>(inst->get_operand(0)), func, targetBB, newBB, inst);
    // 插入跳转
    BranchInst::create_br(inlineEntryBlock, targetBB);
    return newBB;
}

BasicBlock* Inline::transplant(Function* func, Function* target, BasicBlock* entry, BasicBlock* exit_, CallInst* caller){
    std::map<Value*, Value*> pointerMap;
    // 建立新旧基本块映射
    printf("Step 1\n");
    for(BasicBlock* _oriBB: func->get_basic_blocks()){
        pointerMap[_oriBB] = BasicBlock::create(target->get_parent(), getName(), target);
    }
    // 建立新旧指令映射，先不考虑引用关系
    printf("Step 2\n");
    for(BasicBlock* _oriBB: func->get_basic_blocks()){
        BasicBlock* _newBB = dynamic_cast<BasicBlock*>(pointerMap[_oriBB]);
        for(Instruction* inst: _oriBB->get_instructions()){
            pointerMap[inst] = inst->deepcopy(_newBB);
        }
    }
    // 修复引用
    printf("Step 3\n");
    for(BasicBlock* _oriBB: func->get_basic_blocks()){
        BasicBlock* _newBB = dynamic_cast<BasicBlock*>(pointerMap[_oriBB]);
        for(Instruction* inst: _newBB->get_instructions()){
            printf("%s\n", inst->get_name().c_str());
            inst->transplant(pointerMap);
        }
    }
    // 若函数有返回值则创建Phi指令，否则执行空跳转
    printf("Step 4\n");
    if(func->get_return_type()->is_void_type()){
        for(BasicBlock* _oriBB: func->get_basic_blocks()){
            BasicBlock* _newBB = dynamic_cast<BasicBlock*>(pointerMap[_oriBB]);
            auto _instList = _newBB->get_instructions();
            if(_instList.back()->is_ret()){
                _newBB->get_instructions().pop_back();
                BranchInst::create_br(exit_, _newBB);
            }
        }
    }else{
        BasicBlock* finBB = BasicBlock::create(target->get_parent(), getName(), target);
        PhiInst* phi = PhiInst::create_phi(target->get_return_type(), finBB);
        finBB->add_instruction(phi);
        for(BasicBlock* _oriBB: func->get_basic_blocks()){
            BasicBlock* _newBB = dynamic_cast<BasicBlock*>(pointerMap[_oriBB]);
            auto _instList = _newBB->get_instructions();
            auto termInst = _instList.back();
            if(termInst->is_ret()){
                phi->add_phi_pair_operand(termInst->get_operand(0), _newBB);
                termInst->replace_all_use_with(phi);
                _newBB->get_instructions().pop_back();
                BranchInst::create_br(finBB, _newBB);
            }
        }
        BranchInst::create_br(exit_, finBB);
        
    }

    // 替换入参
    printf("Step 5\n");
    auto callParamList = caller->get_operands();
    auto pit = callParamList.begin();
    pit++; // 跳过function pointer
    auto callee = func->arg_begin();
    while (pit != callParamList.end())
    {
        // 替换所有调用者
        for(auto u: (*callee)->get_use_list()){
            User* uu = dynamic_cast<User*>(pointerMap[u.val_]);
            if(uu != nullptr)
                uu->set_operand(u.arg_no_, *pit);
        }
        pit++;
        callee++;
    }
    return dynamic_cast<BasicBlock*>(pointerMap[func->get_entry_block()]);
}


void Inline::run(){
    callAnalysis();
    

    Function* func;
    BasicBlock* bb;
    CallInst* instr;
    int value;
    for(auto st: callInstrHotSpot){
        std::tie(func, bb, instr, value) = st;
        Function* callTarget = dynamic_cast<Function*>(instr->get_operand(0));
        splitBasicBlock(bb, instr);
    }
    
}