# lab3 实验报告
学号 PB18030825 姓名 顾超

## 问题1: cpp与.ll的对应
* CPP行注释中内容为CPP代码对应的LLVM IR指令
### assign_generator.cpp
```c++
Module *module = new Module("assign.c");
IRBuilder *builder = new IRBuilder(nullptr, module);
Type *i32_t = Type::get_int32_type(module);
Type *i32_ptr_t = Type::get_int32_ptr_type(module);
ArrayType *i32_arr10_t = ArrayType::get(i32_t, 10);

auto mainFunction = Function::create(FunctionType::get(i32_t, {}), "main", module);  // define i32 @main() { ... }
auto main_entry_block = BasicBlock::create(module, "main_entry", mainFunction);  // main_entry:
builder->set_insert_point(main_entry_block);
auto a_main_alloca = builder->create_alloca(i32_arr10_t);  // %0 = alloca [10 x i32]
auto a_0_main_ptr = builder->create_gep(a_main_alloca, {CONST_INT(0), CONST_INT(0)});  // %1 = getelementptr [10 x i32], [10 x i32]* %0, i32 0, i32 0
auto a_1_main_ptr = builder->create_gep(a_main_alloca, {CONST_INT(0), CONST_INT(1)});  // %2 = getelementptr [10 x i32], [10 x i32]* %0, i32 0, i32 1
builder->create_store(CONST_INT(10), a_0_main_ptr);  // store i32 10, i32* %1
auto a_0_main_val = builder->create_load(a_0_main_ptr);  // %3 = load i32, i32* %1
auto a_1_main_val = builder->create_imul(a_0_main_val, CONST_INT(2));  // %4 = mul i32 %3, 2
builder->create_store(a_1_main_val, a_1_main_ptr);  // store i32 %4, i32* %2
auto ret_main_val = builder->create_load(a_1_main_ptr);  // %5 = load i32, i32* %2
builder->create_ret(ret_main_val);  // ret i32 %5
```
### fun_generator.cpp
```c++
Module *module = new Module("fun.c");
IRBuilder *builder = new IRBuilder(nullptr, module);
Type *i32_t = Type::get_int32_type(module);
// === Begin of function callee ===
Function* callee_function = Function::create(FunctionType::get(i32_t, std::vector<Type *>{i32_t}), "callee", module);  // define i32 @callee(i32 %0){ ... }
BasicBlock* callee_entry_block = BasicBlock::create(module, "callee_entry", callee_function);  // callee_entry:
builder->set_insert_point(callee_entry_block);
auto ret_callee_val = builder->create_imul(*callee_function->arg_begin(), CONST_INT(2));  // // %1 = mul i32 %0, 2
builder->create_ret(ret_callee_val);  // ret i32 %1
// === End of function callee ===

// === Begin of function main ===
auto mainFunction = Function::create(FunctionType::get(i32_t, {}), "main", module);  // define i32 @main() { ... }
auto main_entry_block = BasicBlock::create(module, "main_entry", mainFunction);  // main_entry:
builder->set_insert_point(main_entry_block);
auto ret_main_val = builder->create_call(callee_function, {CONST_INT(110)});  // %0 = call i32 @callee(i32 110)
builder->create_ret(ret_main_val);  // ret i32 %0
// === End of function main ===
```
### if_generator.cpp
```c++
Module *module = new Module("if.c");
IRBuilder *builder = new IRBuilder(nullptr, module);
// 类型定义
Type *i32_t = Type::get_int32_type(module);
Type *f32_t = Type::get_float_type(module);
// === Begin of function main ===
Function* mainFunction = Function::create(FunctionType::get(i32_t, {}), "main", module);  // define i32 @main() { ... }
BasicBlock* main_entry_block = BasicBlock::create(module, "main_entry", mainFunction);  // main_entry:
BasicBlock* main_if_block = BasicBlock::create(module, "main_if", mainFunction);  // main_if:
BasicBlock* main_finally_block = BasicBlock::create(module, "main_finally", mainFunction);  // main_finally:
builder->set_insert_point(main_entry_block);
auto a_main_ptr = builder->create_alloca(f32_t);  // %0 = alloca float
builder->create_store(CONST_FP(5.555), a_main_ptr);  // store float 0x40163851e0000000, float* %0
auto a_main_val = builder->create_load(a_main_ptr);  // %1 = load float, float* %0
auto if_cond_val = builder->create_fcmp_gt(a_main_val, CONST_FP(1));  // %2 = fcmp ugt float %1,0x3ff0000000000000
builder->create_cond_br(if_cond_val, main_if_block, main_finally_block);  // br i1 %2, label %main_if, label %main_finally
builder->set_insert_point(main_if_block);
builder->create_ret(CONST_INT(233));  // ret i32 233
builder->set_insert_point(main_finally_block);
builder->create_ret(CONST_INT(0));  // ret i32 0
// === End of function main ==
```
### while_generator.cpp
```c++
Module *module = new Module("if.c");
IRBuilder *builder = new IRBuilder(nullptr, module);
// 类型定义
Type *i32_t = Type::get_int32_type(module);
// === Begin of function main ===
Function* mainFunction = Function::create(FunctionType::get(i32_t, {}), "main", module);  // define i32 @main() { ... }
BasicBlock* main_entry_block = BasicBlock::create(module, "main_entry", mainFunction);  // main_entry:
BasicBlock* main_while_if_block = BasicBlock::create(module, "main_while_if", mainFunction);  // main_while_if:
BasicBlock* main_while_main_block = BasicBlock::create(module, "main_while_main", mainFunction);  // main_while_main:
BasicBlock* main_finally_block = BasicBlock::create(module, "main_finally", mainFunction);  // main_finally:
builder->set_insert_point(main_entry_block);
auto a_main_ptr = builder->create_alloca(i32_t);  // %0 = alloca i32
auto i_main_ptr = builder->create_alloca(i32_t);  // %1 = alloca i32
builder->create_store(CONST_INT(10), a_main_ptr);  // store i32 10, i32* %0
builder->create_store(CONST_INT(0), i_main_ptr);  // store i32 0, i32* %1
builder->create_br(main_while_if_block);  // br label %main_while_if
builder->set_insert_point(main_while_if_block);
auto i_main_val = builder->create_load(i_main_ptr);  // %2 = load i32, i32* %1
auto cond_val = builder->create_icmp_lt(i_main_val, CONST_INT(10));  // %3 = icmp slt i32 %2, 10
builder->create_cond_br(cond_val, main_while_main_block, main_finally_block);  // br i1 %3, label %main_while_main, label %main_finally
builder->set_insert_point(main_while_main_block);
auto i1_main_val = builder->create_load(i_main_ptr);  // %4 = load i32, i32* %1
auto i2_main_val = builder->create_iadd(i1_main_val, CONST_INT(1));  // %5 = add i32 %4, 1
builder->create_store(i2_main_val, i_main_ptr);  // store i32 %5, i32* %1
auto a1_main_val = builder->create_load(a_main_ptr);  // %6 = load i32, i32* %0
auto a2_main_val = builder->create_iadd(a1_main_val, i2_main_val);  // %7 = add i32 %6, %5
builder->create_store(a2_main_val, a_main_ptr);  // store i32 %7, i32* %0
builder->create_br(main_while_if_block);  // br label %main_while_if
builder->set_insert_point(main_finally_block);
auto ret_main_val = builder->create_load(a_main_ptr);   // %8 = load i32, i32* %0
builder->create_ret(ret_main_val);  // ret i32 %8
// === End of function main ==
```
## 问题2: Visitor Pattern
请指出visitor.cpp中，`treeVisitor.visit(exprRoot)`执行时，以下几个Node的遍历序列:numberA、numberB、exprC、exprD、exprE、numberF、exprRoot。  
```
        exprRoot
        /       \
       exprE     numberF
      /      \ 
  exprC        exprD
  /   \        /    \
numA   numB   numB   numA
```
`exprRoot->numberF->exprE->exprD->numberB->numberA->exprC->numberA->numberB`


## 问题3: getelementptr
请给出`IR.md`中提到的两种getelementptr用法的区别,并稍加解释:
  - `%2 = getelementptr [10 x i32], [10 x i32]* %1, i32 0, i32 %0` 
  - `%2 = getelementptr i32, i32* %1 i32 %0` 

`[10 x i32]*` 为指向数组指针的指针\
`i32*` 为指向32位整型的指针\
语句1含义为`%1`指向数组，其中数组元素类型为`[10 x i32]`，取出数组中下标为0的元素(类型为`[10 x i32]`)，再取刚刚获得的数组中下标为`%0`的元素，类型为`i32`\
语句2含义为`%1`指向数组，其中元素类型为`i32`，取出其中下标为`%0`的元素，类型为`i32`

## 实验难点
无，对着文档说明可以较快的完成任务（除了用了macOS没用docker/Linux镜像踩坑）

## 实验反馈
希望文档给出更为详细的API说明。