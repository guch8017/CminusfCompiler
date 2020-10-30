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
    // === Begin of function main ===
    Function* mainFunction = Function::create(FunctionType::get(i32_t, {}), "main", module);
    BasicBlock* main_entry_block = BasicBlock::create(module, "main_entry", mainFunction);
    BasicBlock* main_while_if_block = BasicBlock::create(module, "main_while_if", mainFunction);
    BasicBlock* main_while_main_block = BasicBlock::create(module, "main_while_main", mainFunction);
    BasicBlock* main_finally_block = BasicBlock::create(module, "main_finally", mainFunction);
    builder->set_insert_point(main_entry_block);
    auto a_main_ptr = builder->create_alloca(i32_t);
    auto i_main_ptr = builder->create_alloca(i32_t);
    builder->create_store(CONST_INT(10), a_main_ptr);
    builder->create_store(CONST_INT(0), i_main_ptr);
    builder->create_br(main_while_if_block);
    builder->set_insert_point(main_while_if_block);
    auto i_main_val = builder->create_load(i_main_ptr);
    auto cond_val = builder->create_icmp_lt(i_main_val, CONST_INT(10));
    builder->create_cond_br(cond_val, main_while_main_block, main_finally_block);
    builder->set_insert_point(main_while_main_block);
    auto i1_main_val = builder->create_load(i_main_ptr);
    auto i2_main_val = builder->create_iadd(i1_main_val, CONST_INT(1));
    builder->create_store(i2_main_val, i_main_ptr);
    auto a1_main_val = builder->create_load(a_main_ptr);
    auto a2_main_val = builder->create_iadd(a1_main_val, i2_main_val);
    builder->create_store(a2_main_val, a_main_ptr);
    builder->create_br(main_while_if_block);
    builder->set_insert_point(main_finally_block);
    auto ret_main_val = builder->create_load(a_main_ptr); 
    builder->create_ret(ret_main_val);
    // === End of function main ==
    std::cout << module->print();
    delete module;
    return 0;
}