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
    Module *module = new Module("if.c");
    IRBuilder *builder = new IRBuilder(nullptr, module);
    // 类型定义
    Type *i32_t = Type::get_int32_type(module);
    Type *f32_t = Type::get_float_type(module);
    // === Begin of function main ===
    Function* mainFunction = Function::create(FunctionType::get(i32_t, {}), "main", module);
    BasicBlock* main_entry_block = BasicBlock::create(module, "main_entry", mainFunction);
    BasicBlock* main_if_block = BasicBlock::create(module, "main_if", mainFunction);
    BasicBlock* main_finally_block = BasicBlock::create(module, "main_finally", mainFunction);
    builder->set_insert_point(main_entry_block);
    auto a_main_ptr = builder->create_alloca(f32_t);
    builder->create_store(CONST_FP(5.555), a_main_ptr);
    auto a_main_val = builder->create_load(a_main_ptr);
    auto if_cond_val = builder->create_fcmp_gt(a_main_val, CONST_FP(1));
    builder->create_cond_br(if_cond_val, main_if_block, main_finally_block);
    builder->set_insert_point(main_if_block);
    builder->create_ret(CONST_INT(233));
    builder->set_insert_point(main_finally_block);
    builder->create_ret(CONST_INT(0));
    // === End of function main ==
    std::cout << module->print();
    delete module;
    return 0;
}