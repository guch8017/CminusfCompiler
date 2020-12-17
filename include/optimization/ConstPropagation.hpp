#ifndef CONSTPROPAGATION_HPP
#define CONSTPROPAGATION_HPP
#include "PassManager.hpp"
#include "Constant.h"
#include "Instruction.h"
#include "Module.h"

#include "Value.h"
#include "IRBuilder.h"
#include <vector>
#include <queue>
#include <stack>
#include <unordered_map>

class ConstPropagation : public Pass
{
private:
    void process_bb(BasicBlock* bb);
public:
    ConstPropagation(Module *m) : Pass(m) {}
    void run();
};

#endif