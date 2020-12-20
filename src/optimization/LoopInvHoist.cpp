#include <algorithm>
#include "logging.hpp"
#include "LoopSearch.hpp"
#include "LoopInvHoist.hpp"
#include "IRBuilder.h"

#define CONTAIN(set, val) (set.find(val) != set.end())

void LoopInvHoist::run()
{
    IRBuilder* builder = new IRBuilder(nullptr, m_);
    // 先通过LoopSearch获取循环的相关信息
    LoopSearch loop_searcher(m_, false);
    loop_searcher.run();
    // 对所有循环进行处理
    for(Function* func: m_->get_functions()){
        for(auto bbsets: loop_searcher.get_loops_in_func(func)){
            // 若发生了改变需要进行迭代
            // 一次外提操作后可能带来新的不变量
            bool has_change = true;
            while (has_change)
            {
                has_change = false;
                // 保存循环内所有左值，其余变量则为外部来的
                std::unordered_set<Value*> exists;
                for(BasicBlock* bb: *bbsets){
                    for(Instruction* ins: bb->get_instructions()){
                        // 函数调用返回值、load指令(此处只涉及到全局变量及数组取值，其余均已被SSA化)不存入集合，因为它们的返回值可能受其他过程影响
                        if(ins->is_phi() || ins->is_store() || ins->is_load() ||  ins->is_br() || ins->is_call()) continue;
                        exists.insert(ins);
                    }
                }
                // 二次遍历，若发现不变式则外提至主入口处
                std::set<std::pair<BasicBlock*, Instruction*>>  tbr;
                for(BasicBlock* bb: *bbsets){
                    for(Instruction* ins: bb->get_instructions()){
                        // call/phi指令返回值无法确定，即使其未运用到循环体内变量也不外提
                        if(ins->is_phi() ||  ins->is_br() || ins->is_call() || ins->is_load() || ins->is_store()) continue;
                        // TODO: 全局变量处理。。
                        bool t = true;
                        // 遍历所有操作数，若包含内部变量则跳过
                        for(Value* val: ins->get_operands()){
                            if(CONTAIN(exists, val)){
                                t  =  false;
                                break;
                            }
                        }
                        // 全部操作数均来自循环外部，添加到待删除列表
                        if(t){
                            tbr.insert({bb, ins});
                            exists.erase(ins);
                            has_change = true;
                        }
                    }
                }
                // 替换位置
                BasicBlock* bb;
                Instruction* ins;
                for(auto pair: tbr){
                    std::tie(bb, ins) = pair;
                    // 此处取第1个前驱块，也就是进入循环的入口，后序的前驱都是循环内部回来的
                    BasicBlock* target = *loop_searcher.get_loop_base(bbsets)->get_pre_basic_blocks().begin();
                    if(target == nullptr) continue;
                    Instruction*  termIns = target->get_terminator();
                    target->delete_instr_only(termIns);
                    ins->set_parent(target);
                    target->add_instruction(ins);
                    target->add_instruction(termIns);
                    bb->delete_instr_only(ins);
                }
            }
            
        }
    }
}