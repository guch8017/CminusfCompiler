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
    Module *module = new Module("assign.c");
    IRBuilder *builder = new IRBuilder(nullptr, module);
    // 类型定义
    Type *i32_t = Type::get_int32_type(module);
    Type *i32_ptr_t = Type::get_int32_ptr_type(module);
    ArrayType *i32_arr10_t = ArrayType::get(i32_t, 10);
    // === Start of function main ===
    auto mainFunction = Function::create(FunctionType::get(i32_t, {}), "main", module);
    auto main_entry_block = BasicBlock::create(module, "main_entry", mainFunction);
    builder->set_insert_point(main_entry_block);
    // 为数组a分配内存
    auto a_main_alloca = builder->create_alloca(i32_arr10_t);
    // int* ptr_a0 = &a[0]
    auto a_0_main_ptr = builder->create_gep(a_main_alloca, {CONST_INT(0), CONST_INT(0)});
    // int* ptr_a1 = &a[1]
    auto a_1_main_ptr = builder->create_gep(a_main_alloca, {CONST_INT(0), CONST_INT(1)});
    // a[0] = 10
    builder->create_store(CONST_INT(10), a_0_main_ptr);
    // 取a[0]
    auto a_0_main_val = builder->create_load(a_0_main_ptr);
    // a1_tmp = 2 * a[0]
    auto a_1_main_val = builder->create_imul(a_0_main_val, CONST_INT(2));
    // a[1] = a1_tmp
    builder->create_store(a_1_main_val, a_1_main_ptr);
    // 取a[1]
    auto ret_main_val = builder->create_load(a_1_main_ptr);
    // 返回
    builder->create_ret(ret_main_val);
    std::cout << module->print();
    delete module;
    return 0;
}
