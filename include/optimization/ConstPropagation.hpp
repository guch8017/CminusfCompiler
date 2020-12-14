#ifndef CONSTPROPAGATION_HPP
#define CONSTPROPAGATION_HPP
#include "PassManager.hpp"
#include "Constant.h"
#include "Instruction.h"
#include "Module.h"

#include "Value.h"
#include "IRBuilder.h"
#include <vector>
#include <stack>
#include <unordered_map>

// tips: ConstFloder类
class ConstFolder
{
public:
    ConstFolder(Module *m) : module_(m) {}
    ConstantInt *compute(
        Instruction::OpID op,
        ConstantInt *value1,
        ConstantInt *value2);
    // ...
private:
    Module *module_;
};

class ConstPropagation : public Pass
{
public:
    ConstPropagation(Module *m) : Pass(m) {}
    void run();
};

#endif