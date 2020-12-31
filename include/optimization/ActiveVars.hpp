#ifndef ACTIVEVARS_HPP
#define ACTIVEVARS_HPP
#include "PassManager.hpp"
#include "Constant.h"
#include "Instruction.h"
#include "Module.h"

#include "Value.h"
#include "IRBuilder.h"
#include <vector>
#include <stack>
#include <unordered_map>
#include <list>
#include <map>
#include <queue>
#include <fstream>

class ActiveVars : public Pass
{
public:
    ActiveVars(Module *m) : Pass(m) {}
    void run();
    std::string print();
private:
    Function *func_;
    std::map<BasicBlock *, std::set<Value *>> live_in, live_out;

    std::list<BasicBlock *> reverse_post_order_;    // 记录深度优先遍历访问逆序
    void create_reverse_post_order(Function *f);    // 创建深度优先序的逆序
    void DFS_visit(BasicBlock *bb); //深度优先遍历
};

#endif