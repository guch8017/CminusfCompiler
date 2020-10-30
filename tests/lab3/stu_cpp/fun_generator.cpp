#include "BasicBlock.h"
#include "Constant.h"
#include "Function.h"
#include "IRBuilder.h"
#include "Module.h"
#include "Type.h"

#include <iostream>
#include <memory>

#ifdef DEBUG  // 用于调试信息,大家可以在编译过程中通过" -DDEBUG"来开启这一选项
#define DEBUG_OUTPUT std::cout << __LINE__ << std::endl;  // 输出行号的简单示例
#else
#define DEBUG_OUTPUT
#endif

#define CONST_INT(num) \
    ConstantInt::get(num, module)

#define CONST_FP(num) \
    ConstantFP::get(num, module) // 得到常数值的表示,方便后面多次用到

int main(){
    Module *module = new Module("fun.c");
    IRBuilder *builder = new IRBuilder(nullptr, module);
    // 类型定义
    Type *i32_t = Type::get_int32_type(module);
    // === Begin of function callee ===
    Function* callee_function = Function::create(FunctionType::get(i32_t, std::vector<Type *>{i32_t}), "callee", module);
    BasicBlock* callee_entry_block = BasicBlock::create(module, "callee_entry", callee_function);
    builder->set_insert_point(callee_entry_block);
    // 从参数列表中取第一个参数，即a的值，将其*2后赋值给ret
    auto ret_callee_val = builder->create_imul(*callee_function->arg_begin(), CONST_INT(2));
    // 返回调用函数
    builder->create_ret(ret_callee_val);
    // === End of function callee ===

    // === Begin of function main ===
    auto mainFunction = Function::create(FunctionType::get(i32_t, {}), "main", module);
    auto main_entry_block = BasicBlock::create(module, "main_entry", mainFunction);
    builder->set_insert_point(main_entry_block);
    // 调用callee函数，传入参数110
    auto ret_main_val = builder->create_call(callee_function, {CONST_INT(110)});
    builder->create_ret(ret_main_val);
    // === End of function main ==
    std::cout << module->print();
    delete module;
    return 0;
}