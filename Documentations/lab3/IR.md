# IR Reference
- [IR Reference](#ir-reference)
  - [IR Features](#ir-features)
  - [IR Format](#ir-format)
  - [Instruction](#instruction)
    - [Terminator Instructions](#terminator-instructions)
      - [Ret](#ret)
      - [Br](#br)
    - [Standard binary operators](#standard-binary-operators)
      - [Add FAdd](#add-fadd)
      - [Sub FSub](#sub-fsub)
      - [Mul FMul](#mul-fmul)
      - [SDiv FDiv](#sdiv-fdiv)
    - [Memory operators](#memory-operators)
      - [Alloca](#alloca)
      - [Load](#load)
      - [Store](#store)
    - [CastInst](#castinst)
      - [ZExt](#zext)
      - [FpToSi](#fptosi)
      - [SiToFp](#sitofp)
    - [Other operators](#other-operators)
      - [Cmp FCmp](#cmp-fcmp)
      - [Call](#call)
      - [GetElementPtr](#getelementptr)

----
## IR Features
- 采用 3 地址的方式 
  - 区别于 X86 汇编的目标和源寄存器共用的模式： ADD EAX, EBX 
  - %2 = add i32 %0, %1
- SSA 形式 + 无限寄存器
  - 每个变量都只被赋值一次 
  - 容易确定操作间的依赖关系，便于优化分析
- 强类型系统
  - 每个 Value 都具备自身的类型， 
  - IR类型系统：
    - `i1`：1位宽的整数类型
    - `i32`：32位宽的整数类型
    - `float`：单精度浮点数类型
    - `pointer`：指针类型
      - 例如：`i32*, [10 x i32*]`
    - `label` bb的标识符类型
    - `functiontype`函数类型，包括函数返回值类型与参数类型（下述文档未提及）

## IR Format
以下面的`easy.c`与`easy.ll`为例进行说明。  
通过命令`clang -S -emit-llvm easy.c`可以得到对应的`easy.ll`如下（助教增加了额外的注释）。`.ll`文件中注释以`;`开头。  

- `easy.c`: 
  ``` c
  int main(){
    int a;
    int b;
    a = 1;
    b = 2;
    if(a < b)
      b = 3;
    return a + b;
  }
  ```
- `easy.ll`:
  ``` c
  ; 注释: .ll文件中注释以';'开头
  ; ModuleID = 'easy.c'                                
  source_filename = "easy.c"  
  ; 注释: target的开始
  target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
  target triple = "x86_64-unknown-linux-gnu"
  ; 注释: target的结束
  
  ; 注释: 全局main函数的定义
  ; Function Attrs: noinline nounwind optnone uwtable
  define dso_local i32 @main() #0 {
  ; 注释: 第一个基本块的开始
    %1 = alloca i32, align 4
    %2 = alloca i32, align 4
    %3 = alloca i32, align 4
    store i32 0, i32* %1, align 4
    store i32 1, i32* %2, align 4
    store i32 2, i32* %3, align 4
    %4 = load i32, i32* %2, align 4
    %5 = load i32, i32* %3, align 4
    %6 = icmp slt i32 %4, %5
    br i1 %6, label %7, label %8
  ; 注释: 第一个基本块的结束

  ; 注释: 第二个基本块的开始
  7:                                                ; preds = %0
    store i32 3, i32* %3, align 4
    br label %8
  ; 注释: 第二个基本块的结束
  
  ; 注释: 第三个基本块的开始
  8:                                                ; preds = %7, %0
    %9 = load i32, i32* %2, align 4
    %10 = load i32, i32* %3, align 4
    %11 = add nsw i32 %9, %10
    ret i32 %11                                     ; 注释: 返回语句
  ; 注释: 第三个基本块的结束
  }
  
  attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
  
  !llvm.module.flags = !{!0}
  !llvm.ident = !{!1}
  
  !0 = !{i32 1, !"wchar_size", i32 4}
  !1 = !{!"clang version 10.0.1 "}
  ```
其中，每个program由1个或多个module组成，module之间由LLVM Linker合并。  
每个module组成如下：
- Target Information：
  ``` c
  target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
  target triple = "x86_64-unknown-linux-gnu"
  ```
- Global Symbols: main函数的定义
- Others:尾部其他信息  

每个函数的组成如下：
- 头部：函数返回值类型，函数名，函数参数
- 一个或多个基本块：
  - 每个基本块又有Label和Instruction组成。
    ``` c
    8:                                                ; preds = %7, %0
      %9 = load i32, i32* %2, align 4
      %10 = load i32, i32* %3, align 4
      %11 = add nsw i32 %9, %10
      ret i32 %11  
    ```
    这个例子中，`8`就是Label。  
    `%9 = load i32, i32* %2, align 4`中的`%9`是目的操作数，`load`是指令助记符，`i32`是`int32`的类型，`i32*`是指向`int32`的地址类型，`%2`是源操作数，`align 4`表示对齐。
## Instruction
### Terminator Instructions
#### Ret 
- 概念：` ret`指令用于将控制流（以及可选的值）从函数返回给调用者。`ret`指令有两种形式：一种返回值，然后导致控制流，另一种仅导致控制流发生。
- 格式
  - `ret <type> <value>`
  - `ret void`

- 例子：
  - `ret i32 %0`
  - `ret void`
  
#### Br 
- 概念：`br`指令用于使控制流转移到当前功能中的另一个基本块。 该指令有两种形式，分别对应于条件分支和无条件分支。
- 格式：
  - `br i1 <cond>, label <iftrue>, label <iffalse>`
  - `br label <dest>`
- 例子：
  - `br i1 %cond label %truebb label %falsebb`
  - `br label %bb`
    
### Standard binary operators
#### Add FAdd
- 概念：`add`指令返回其两个`i32`类型的操作数之和，返回值为`i32`类型，`fadd`指令返回其两个`float`类型的操作数之和，返回值为`float`类型
- 格式：
  - `<result> = add <type> <op1> <op2>`
  - `<result> = fadd <type> <op1> <op2>`
- 例子：
  - `%2 = add i32 %1, %0` 
  - `%2 = fadd float %1, %0` 
  
#### Sub FSub
- 概念：`sub`指令返回其两个`i32`类型的操作数之差，返回值为`i32`类型，`fsub`指令返回其两个`float`类型的操作数之差，返回值为`float`类型
- 格式与例子与`add`，`fadd`类似

#### Mul FMul
- 概念：`mul`指令返回其两个`i32`类型的操作数之积，返回值为`i32`类型，`fmul`指令返回其两个`float`类型的操作数之积，返回值为`float`类型
- 格式与例子与`add`，`fadd`类似

#### SDiv FDiv
- 概念：`sdiv`指令返回其两个`i32`类型的操作数之商，返回值为`i32`类型，`fdiv`指令返回其两个`float`类型的操作数之商，返回值为`float`类型
- 格式与例子与`add`，`fadd`类似

### Memory operators
#### Alloca
- 概念： `alloca`指令在当前执行函数的堆栈帧上分配内存，当该函数返回其调用者时将自动释放该内存。 始终在地址空间中为数据布局中指示的分配资源分配对象
- 格式：`<result> = alloca <type>`
- 例子：
  - `%ptr = alloca i32`	
  - `%ptr = alloca [10 x i32]`

#### Load
- 概念：`load`指令用于从内存中读取。
- 格式：`<result> = load <type>, <type>* <pointer>`
- 例子：`%val = load i32, i32* %ptr`

#### Store
- 概念：`store`指令用于写入内存
- 格式：`store <type> <value>, <type>* <pointer>`
- 例子：`store i32 3, i32* %ptr`

### CastInst
#### ZExt    
- 概念：`zext`指令将其操作数**零**扩展为`ty2`类型。
- 格式：`<result> = zext <type> <value> to <ty2>`
- 例子：`%1 = zext i1 %0 to i32`

#### FpToSi
- 概念：`fptosi`指令将浮点值转换为`ty2`（整数）类型。
- 格式：`<result> = fptosi <type> <value> to <ty2>`
- 例子：`%Y = fptosi float 1.0E-247 to i32`

#### SiToFp
- 概念：`sitofp`指令将有符号整数转换为`ty2`（浮点数）类型。
- 格式：`<result> = sitofp <type> <value> to <ty2>`
- 例子：`%X = sitofp i32 257 to float`

### Other operators
#### ICmp FCmp
- 概念：`icmp`指令根据两个整数的比较返回布尔值，`fcmp`指令根据两个浮点数的比较返回布尔值。
- 格式：
  - `<result> = icmp <cond> <type> <op1>, <op2>`
    - `<cond> = eq | ne | sgt | sge | slt | sle`
  - `<result> = fcmp <cond> <type> <op1>, <op2>`
    - `<cond> = eq | ne | ugt | uge | ult | ule`
- 例子：`i1 %2 = cmp sge i32 %0 i32 %1`

#### Call
- 概念：`call`指令用于使控制流转移到指定的函数，其传入参数绑定到指定的值。 在被调用函数中执行`ret`指令后，控制流程将在函数调用后继续执行该指令，并且该函数的返回值绑定到`result`参数。
- 格式：
  - `<result> = call <return ty> <func name>(<function args>) `
- 例子：
  - `%0 = call i32 @func( i32 %1, i32* %0)`
  - `call @func( i32 %arg)`

#### GetElementPtr
- 概念：`getelementptr`指令用于获取数组结构的元素的地址。 它仅执行地址计算，并且不访问内存。
- 格式：`<result> = getelementptr <type>, <type>* <ptrval> [, <type> <idx>]`
- 参数解释：第一个参数是计算基础类型，第二第三个参数表示索引开始的指针类型及指针，`[]`表示可重复参数，里面表示的数组索引的偏移类型及偏移值。（Question：思考指针类型为`[10 x i32]`指针和`i32`指针`getelementptr`用法的不同，并给出解释，实验结束后回答两者使用情况的区别）
- 例子：
  - `%2 = getelementptr [10 x i32], [10 x i32]* %1, i32 0, i32 %0` 
  - `%2 = getelementptr i32, i32* %1 i32 %0` 
