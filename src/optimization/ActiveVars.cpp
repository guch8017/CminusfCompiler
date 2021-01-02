#include "ActiveVars.hpp"
#include "LoopSearch.hpp"
#include "Instruction.h"
#include "algorithm"
#include "logging.hpp"

#define IS_CONST_INT(ptr) (dynamic_cast<ConstantInt*>(ptr) != nullptr)  // Bool常量也在这
#define IS_CONST_FP(ptr) (dynamic_cast<ConstantFP*>(ptr) != nullptr)
#define CONTAIN(set, val) (set.find(val) != set.end())

void ActiveVars::run()
{
    std::ofstream output_active_vars;
    output_active_vars.open("active_vars.json", std::ios::out);
    output_active_vars << "[";
    for (auto &func : this->m_->get_functions()) {
        if (func->get_basic_blocks().empty()) {
            continue;
        }
        else
        {
            func_ = func;  

            func_->set_instr_name();
            live_in.clear();
            live_out.clear();
            
            // 在此分析 func_ 的每个bb块的活跃变量，并存储在 live_in live_out 结构内
            // 建立流图：无需重新建立，给的api已经有相关操作了，只需要进行深度优先遍历即可

            LOG(DEBUG) << "Position 1:initial\n";
            std::map<BasicBlock *, std::set<Value *>> bb2ActiveUseVar,bb2ActiveDefVar;
            reverse_post_order_.clear();
            create_reverse_post_order(func_);   // 获得深度优先逆序的遍历顺序
            LOG(DEBUG) << "Position 2:successfully get DFS reverse\n";
            LOG(DEBUG) << "reverse cfg:";
            for (auto printbb : reverse_post_order_){
                LOG(DEBUG) << " " + printbb->get_name() + " ";
            }
            // 需要添加entry&exit块否？);
            // 计算每块的use & def, 对 live_in 初始化
            std::set<Value *> use_var, def_var, phi_deleted_var;
            for (auto bb : func_->get_basic_blocks())
            {
                LOG(DEBUG) << bb->print();
                // 可以这么初始化吗。。。？
                use_var.clear();
                def_var.clear();
                // 计算每条语句的use & def
                // 和语句的类型有关吗？A:有的
                for(auto ins: bb->get_instructions())
                {
                    //LOG(DEBUG) << ins->print();
                    if(ins->isBinary()){
                        // 普通的两操作数运算指令,操作数用get_operand取，左值就是指令本身
                        // 区分加一个立即数&加一个变量但变量本身为常数的区别，因此左值肯定是变量，常量传播不是我考虑的
                        Value* lhs = ins->get_operand(0);
                        Value* rhs = ins->get_operand(1);
                        if(!CONTAIN(def_var,lhs) && !IS_CONST_INT(lhs) && !IS_CONST_FP(lhs)){
                            use_var.insert(lhs);
                        }
                        if(!CONTAIN(def_var,rhs) && !IS_CONST_INT(rhs) && !IS_CONST_FP(rhs)){
                            use_var.insert(rhs);
                        }
                        if(!CONTAIN(use_var,ins)){
                            def_var.insert(ins);
                        }
                    }
                    else if(ins->isTerminator()){
                        if(ins->is_ret()){
                            // 返回指令，若有返回值，也算一次引用！
                            ReturnInst* retInstr = dynamic_cast<ReturnInst*>(ins);
                            if(!retInstr->is_void_ret()){
                                //有返回值！
                                Value* ret_var = ins->get_operand(0);
                                if(!CONTAIN(def_var,ret_var) && !IS_CONST_INT(ret_var) && !IS_CONST_FP(ret_var)){
                                    use_var.insert(ret_var);
                                }
                            }
                        }
                        else if(ins->is_br()){
                            // 跳转指令，只需考虑有条件跳转的条件
                            BranchInst* brInstr =  dynamic_cast<BranchInst*>(ins);
                            if(brInstr->is_cond_br()){
                                Value* cond = brInstr->get_operand(0);
                                if(!CONTAIN(def_var,cond) && !IS_CONST_INT(cond) && !IS_CONST_FP(cond)){
                                    use_var.insert(cond);
                                }
                            }
                        }
                    }
                    else if(ins->is_cmp() || ins->is_fcmp()){
                        // 比较指令，类似binary指令
                        Value* lhs = ins->get_operand(0);
                        Value* rhs = ins->get_operand(1);
                        if(!CONTAIN(def_var,lhs) && !IS_CONST_INT(lhs) && !IS_CONST_FP(lhs)){
                            use_var.insert(lhs);
                        }
                        if(!CONTAIN(def_var,rhs) && !IS_CONST_INT(rhs) && !IS_CONST_FP(rhs)){
                            use_var.insert(rhs);
                        }
                        if(!CONTAIN(use_var,ins)){
                            def_var.insert(ins);
                        }
                    }
                    else if(ins->is_call()){
                        // 调用指令，第一个参数为函数label，显然label不会成为活跃变量[块中顺序执行]
                        // 右值参数数量不定，但是处理方法类似算术运算，左值为指令本身
                        CallInst* callInstr =  dynamic_cast<CallInst*>(ins);
                        for(int i = 1; i < callInstr->get_num_operand(); i++){
                            Value* arg = callInstr->get_operand(i);
                            if(!CONTAIN(def_var,arg) && !IS_CONST_INT(arg) && !IS_CONST_FP(arg)){
                                use_var.insert(arg);
                            }
                        }
                        if(!callInstr->get_function_type()->is_void_type()){
                            //函数调用时，返回值不为空，要判断返回值类型
                            if(!CONTAIN(use_var,ins)){
                                def_var.insert(ins);
                            }
                        }
                    }
                    else if(ins->is_fp2si()){
                        // 指令只有一个操作数
                        Value* ope = ins->get_operand(0);
                        if(!CONTAIN(def_var,ope) && !IS_CONST_FP(ope)){
                            use_var.insert(ope);
                        }
                        if(!CONTAIN(use_var,ins)){
                            def_var.insert(ins);
                        }
                    }
                    else if(ins->is_si2fp()){
                        // 指令只有一个操作数
                        Value* ope = ins->get_operand(0);
                        if(!CONTAIN(def_var,ope) && !IS_CONST_INT(ope)){
                            use_var.insert(ope);
                        }
                        if(!CONTAIN(use_var,ins)){
                            def_var.insert(ins);
                        }
                    }
                    else if(ins->is_zext()){
                        // 指令只有一个操作数
                        Value* ope = ins->get_operand(0);
                        if(!CONTAIN(def_var,ope) && !IS_CONST_INT(ope) && !IS_CONST_FP(ope)){
                            use_var.insert(ope);
                        }
                        if(!CONTAIN(use_var,ins)){
                            def_var.insert(ins);
                        }
                    }
                    else if(ins->is_phi()){
                        //phi指令，pair(bb,var)的var为右值，指令本身为左值
                        PhiInst* phiInstr =  dynamic_cast<PhiInst*>(ins);
                        for(int i = 0; i < phiInstr->get_num_operand()/2; i++){
                            Value* ope = phiInstr->get_operand(2*i);
                            if(!CONTAIN(def_var,ope) && !IS_CONST_INT(ope) && !IS_CONST_FP(ope)){
                                use_var.insert(ope);
                            }
                        }
                        if(!CONTAIN(use_var,ins)){
                            def_var.insert(ins);
                        }
                    }
                    else if(ins->is_gep()){
                        // 数组类型，可能和load，store相关？
                        // 右值参数数量不定，但是处理方法类似算术运算，左值为指令本身
                        GetElementPtrInst* gepInstr =  dynamic_cast<GetElementPtrInst*>(ins);
                        for(int i = 0; i < gepInstr->get_num_operand(); i++){
                            Value* ope = gepInstr->get_operand(i);
                            if(!CONTAIN(def_var,ope) && !IS_CONST_INT(ope) && !IS_CONST_FP(ope)){
                                use_var.insert(ope);
                            }
                        }
                        if(!CONTAIN(use_var,ins)){
                            def_var.insert(ins);
                        }
                    }
                    else if(ins->is_alloca()){
                        // alloca类型，一般只有在给数组赋值时可能才会出现
                        // 指令没有右值，即无操作数；指令本身为左值
                        if(!CONTAIN(use_var,ins)){
                            def_var.insert(ins);
                        }
                    }
                    else if(ins->is_load()){
                        // load类型，一般与全局变量/数组联系
                        // 指令只有一个操作数
                        Value* ptr = ins->get_operand(0);
                        if(!CONTAIN(def_var,ptr) && !IS_CONST_INT(ptr) && !IS_CONST_FP(ptr)){
                            use_var.insert(ptr);
                        }
                        if(!CONTAIN(use_var,ins)){
                            def_var.insert(ins);
                        }
                    }
                    else if(ins->is_store()){
                        // store指令，似乎不会出现这个指令。两个操作数，第一个是val，第二个是ptr
                        // 该指令本身不会作为操作数被引用
                        Value* val = ins->get_operand(0);
                        Value* ptr = ins->get_operand(1);
                        if(!CONTAIN(def_var,val) && !IS_CONST_INT(val) && !IS_CONST_FP(val)){
                            use_var.insert(val);
                        }
                        if(!CONTAIN(def_var,ptr) && !IS_CONST_INT(ptr) && !IS_CONST_FP(ptr)){
                            use_var.insert(ptr);
                        }
                    }
                }
                bb2ActiveUseVar[bb].insert(use_var.begin(), use_var.end());
                bb2ActiveDefVar[bb].insert(def_var.begin(), def_var.end());
                LOG(DEBUG) << "Block " + bb->get_name() + " use_var ";
                for(auto printvar : use_var){
                    LOG(DEBUG) << " " + printvar->get_name() + " ";
                }
                LOG(DEBUG) << "Block " + bb->get_name() + " def_var ";
                for(auto printvar : def_var){
                    LOG(DEBUG) << " " + printvar->get_name() + " ";
                }
            }
            LOG(DEBUG) << "Position 3:successfully initialize use and def in each block\n";

            // 逆序计算每块的OUT & IN
            bool changed = true;
            std::set<Value *> out_ActiveVar, in_ActiveVar;
            while (changed) {
                changed = false;
                // 深度优先序的逆序
                for (auto bbs : reverse_post_order_)
                {
                    //暂存live_out的，所以要放在循环内初始化
                    out_ActiveVar.clear();
                    // 没有后继块，即最后一块
                    /* 不需要这个吧。。
                    if(bbs->get_succ_basic_blocks().size() < 1)
                        continue;
                    */
                    
                    // 取所有后继块的并集
                    for(auto bbsu : bbs->get_succ_basic_blocks())
                    {
                        // 可以这样插入多个值吗？注意函数的重载，若要区域插入需要两个参数：开头和结尾
                        out_ActiveVar.insert(live_in[bbsu].begin(), live_in[bbsu].end());
                        
                        // phi指令的处理
                        phi_deleted_var.clear();
                        for(auto phi_ins : bbsu->get_instructions())
                        {
                            // 如果不为块开头的phi指令，跳出判断
                            if(!phi_ins->is_phi())
                                break;
                            for(int i = 0; i < phi_ins->get_num_operand()/2; i++)
                            {
                                // 可以这样判断指令相等吗？
                                // 如果phi指令中某个变量来源不是bbs，则bbs出口处该变量不活跃
                                if(phi_ins->get_operand(2*i+1) != bbs)
                                    phi_deleted_var.insert(phi_ins->get_operand(2*i));
                            }
                            // 开删
                            for(auto PhiDeletedVar : phi_deleted_var)
                            {
                                auto iter = out_ActiveVar.find(PhiDeletedVar);
                                if(iter != out_ActiveVar.end())
                                    out_ActiveVar.erase(iter);
                            }
                        }
                    }
                    // 是否需要先清空？
                    live_out[bbs].clear();
                    live_out[bbs].insert(out_ActiveVar.begin(), out_ActiveVar.end());
                    
                    // 后续用于判断是否跳出循环
                    std::set<Value *> BeforeVar;
                    BeforeVar.clear();
                    BeforeVar.insert(live_in[bbs].begin(), live_in[bbs].end());

                    // 要在循环内clear。。。要不然就错了！！！
                    in_ActiveVar.clear();
                    // outB - defB
                    in_ActiveVar.insert(live_out[bbs].begin(), live_out[bbs].end());

                    for(auto DeletedVar : bb2ActiveDefVar[bbs])
                    {
                        auto iter = in_ActiveVar.find(DeletedVar);
                        if(iter != in_ActiveVar.end())
                            in_ActiveVar.erase(iter);
                    }

                    // useB
                    in_ActiveVar.insert(bb2ActiveUseVar[bbs].begin(), bb2ActiveUseVar[bbs].end());

                    live_in[bbs].clear();
                    live_in[bbs].insert(in_ActiveVar.begin(), in_ActiveVar.end());

                    // 发生了改变，要继续循环
                    if(BeforeVar != live_in[bbs])
                        changed = true;
                    
                    LOG(DEBUG) << "Block " + bbs->get_name() + " out ";
                    for(auto printvar : live_out[bbs]){
                        LOG(DEBUG) << " " + printvar->get_name() + " ";
                    }
                    LOG(DEBUG) << "Block " + bbs->get_name() + " in ";
                    for(auto printvar : live_in[bbs]){
                        LOG(DEBUG) << " " + printvar->get_name() + " ";
                    }
                }
            }
            LOG(DEBUG) << "Position 4:successfully get in and out\n";

            output_active_vars << print();
            output_active_vars << ",";
        }
    }
    output_active_vars << "]";
    output_active_vars.close();
    return ;
}

/**
 * 后序遍历
 * 已知流图，从开始点进行遍历，因此参数只需要点即可
 * 运行结束后reverse_post_order_被正确赋值
 */
void ActiveVars::create_reverse_post_order(Function *f)
{
    reverse_post_order_.clear();
    // 从函数入口进行遍历
    DFS_visit(f->get_entry_block());
    reverse_post_order_.reverse();
}

/**
 * 深度优先搜索
 * @param bb 基本块
 * @param visited 已访问的基本块集合
 */
void ActiveVars::DFS_visit(BasicBlock *bb)
{
    reverse_post_order_.push_back(bb);
    for (auto b : bb->get_succ_basic_blocks()) {
        // 如何在list中查找元素
        std::list<BasicBlock*>::iterator iter;
        iter = find(reverse_post_order_.begin(), reverse_post_order_.end(), b);
        if (iter == reverse_post_order_.end())   // 即b不在visited中，要访问b
            DFS_visit(b);
    }
}

std::string ActiveVars::print()
{
    std::string active_vars;
    active_vars +=  "{\n";
    active_vars +=  "\"function\": \"";
    active_vars +=  func_->get_name();
    active_vars +=  "\",\n";

    active_vars +=  "\"live_in\": {\n";
    for (auto &p : live_in) {
        if (p.second.size() == 0) {
            continue;
        } else {
            active_vars +=  "  \"";
            active_vars +=  p.first->get_name();
            active_vars +=  "\": [" ;
            for (auto &v : p.second) {
                active_vars +=  "\"%";
                active_vars +=  v->get_name();
                active_vars +=  "\",";
            }
            active_vars += "]" ;
            active_vars += ",\n";   
        }
    }
    active_vars += "\n";
    active_vars +=  "    },\n";
    
    active_vars +=  "\"live_out\": {\n";
    for (auto &p : live_out) {
        if (p.second.size() == 0) {
            continue;
        } else {
            active_vars +=  "  \"";
            active_vars +=  p.first->get_name();
            active_vars +=  "\": [" ;
            for (auto &v : p.second) {
                active_vars +=  "\"%";
                active_vars +=  v->get_name();
                active_vars +=  "\",";
            }
            active_vars += "]";
            active_vars += ",\n";
        }
    }
    active_vars += "\n";
    active_vars += "    }\n";

    active_vars += "}\n";
    active_vars += "\n";
    return active_vars;
}