# Lab5 实验报告

小组成员 姓名 学号

## 实验要求

请按照自己的理解，写明本次实验需要干什么

+ 活跃变量分析：根据老师上课所讲述的活跃变量分析方法，得到流图的深度优先遍历逆序与各块的def，use集合，根据活跃变量分析表达式分析bb块的入口和出口的活跃变量，关注phi指令的处理

## 实验难点

实验中遇到哪些挑战

+ 活跃变量
  + phi指令的特殊处理：修改数据流方程
  + 不可达块的处理

## 实验设计

* 常量传播
    实现思路：
    相应代码：
    优化前后的IR对比（举一个例子）并辅以简单说明：
    


* 循环不变式外提
    实现思路：
    相应代码：
    优化前后的IR对比（举一个例子）并辅以简单说明：
    
* 活跃变量分析

    实现思路：

    1. 深度优先遍历流图，获得**流图**的结点深度优先序的**逆序**：利用reverse_post_order_存储

       + 注意basicblock的api已经构建好cfg了，直接使用即可
       + 无需手动添加entry、exit块
       + 不同深度遍历优先序得到的结果是等价的

    2. 计算各个基本块的use & def

       + 单个语句的迁移函数
         + 块内语句是顺序执行，逐条语句分析：先取操作数分析右值，看是否在该块的def中，若不在，加入use；再分析左值，看是否在该块的use中，若不在，加入def
       + 区分**左值右值**：右值是操作数，左值就是指令本身
       + 注意**常数**处理：常数不属于活跃变量
       + 以binary指令为例

       ```c++
       if(ins->isBinary()){
       	Value* lhs = ins->get_operand(0);
           Value* rhs = ins->get_operand(1);
           if(!CONTAIN(def_var,lhs) && !IS_CONST_INT(lhs) && !IS_CONST_FP(lhs)){
       		use_var.insert(lhs);
           }
           if(!CONTAIN(def_var,rhs) && !IS_CONST_INT(rhs) && !IS_CONST_FP(rhs)){
               use_var.insert(rhs);
           }
           if(!CONTAIN(use_var,ins)){
               def_var.insert(ins);
           }
       }
       ```

    3. 不同指令类型

       + binary指令：右值为两个操作数，左值即指令本身
       + cmp，fcmp指令：类似binary指令
       + 跳转br指令：只需要考虑**有条件判断**的 右值判断条件变量
       + 跳转ret指令：返回值**类型非空**时，考虑右值返回值
       + call指令：**第一个**操作数为函数**label**，显然label不会成为活跃变量[块中顺序执行]。后面为函数参数，需要考虑；当函数返回值不为空时，要考虑指令本身为左值。右值**参数数量不定**，但是处理方法类似算术运算
       + fp2si，si2fp，zext指令：一个操作数为右值，指令为左值
       + gep指令：右值**操作数数量不定**，但是处理方法类似算术运算，左值为指令本身
       + 内存相关指令
         + alloca指令：指令没有右值，即**无操作数**；指令本身为左值
         + load指令：指令只有一个操作数，一般与全局变量/数组联系；指令本身为左值
         + store指令：两个操作数，第一个是val，第二个是ptr；左值该指令本身**不会作为操作数被引用**
       + phi指令：块，变量对的**变量为右值**，指令本身为左值

    4. 逆序迭代求各块的out/in

       + 计算公式

         OUT[B] = ∪~S是B的后继~ IN [S]
         IN [B] = use~B~ ∪ (OUT [B] - def~B~ )  

       + 多次迭代直至所有块活跃变量不发生改变：while(changed)
       + 并集、补集的计算

       ```c++
       for(auto bbsu : bbs->get_succ_basic_blocks()){
       	out_ActiveVar.insert(live_in[bbsu].begin(), live_in[bbsu].end());
           //phi指令处理见下
       }
       live_out[bbs].insert(out_ActiveVar.begin(), out_ActiveVar.end());
       ```

       ```c++
       // outB - defB
       in_ActiveVar.insert(live_out[bbs].begin(), live_out[bbs].end());
       for(auto DeletedVar : bb2ActiveDefVar[bbs]){
       	auto iter = in_ActiveVar.find(DeletedVar);
           if(iter != in_ActiveVar.end())
           	in_ActiveVar.erase(iter);
       }
       // useB
       in_ActiveVar.insert(bb2ActiveUseVar[bbs].begin(), bb2ActiveUseVar[bbs].end());
       
       live_in[bbs].clear();
       live_in[bbs].insert(in_ActiveVar.begin(), in_ActiveVar.end());
       ```

    5. phi语句的处理：数据流方程的修改

       + phi语句形式：%0 = phi [%op1, %bb1], [%op2, %bb2]
       + S入口处活跃的变量并非在它所有前驱的出口处都是活跃的
       + 注意phi指令虽然**打印出来有**undef的部分，但是实际的指令中是**没有这个操作数**的

       判断：

       若块S的**开头**几个语句是phi语句，令set是out[B]中要删除的变量，即OUT[B] = ∪~S是B的后继~ ( IN[S] - set )

       下求set，伪代码如下：

       ```
       while(开头为phi语句) //phi语句逐条分析{
       	对phi语句中的每个[operand,bb]对
       		if bb != B：
       			set.insert(operand)，插入set
       }
       ```

       ```c++
       // phi指令的处理
       phi_deleted_var.clear();
       for(auto phi_ins : bbsu->get_instructions()){
       	// 如果不为块开头的phi指令，跳出判断
       	if(!phi_ins->is_phi())
           	break;
           for(int i = 0; i < phi_ins->get_num_operand()/2; i++){
           	// 如果phi指令中某个变量来源不是bbs，则bbs出口处该变量不活跃
               if(phi_ins->get_operand(2*i+1) != bbs)
               	phi_deleted_var.insert(phi_ins->get_operand(2*i));
           }
           // 开删
           for(auto PhiDeletedVar : phi_deleted_var){
           	auto iter = out_ActiveVar.find(PhiDeletedVar);
               if(iter != out_ActiveVar.end())
               	out_ActiveVar.erase(iter);
           }
       }
       ```

    6. 增加存储结构 map<bb,use> & map<bb,def>

       其中use，def是std::set<Value *> 方便计算

    7. 不可达块的处理

       cminus文件中的

       ```c
       int gcd (int u, int v) {
           if (v == 0) return u;
           else return gcd(v, u - u / v * v);
       }
       ```

       翻译为.ll文件会出现不可达块label20

       ```llvm
       define i32 @gcd(i32 %arg0, i32 %arg1) {
       label_entry:
         %op5 = icmp eq i32 %arg1, 0
         %op6 = zext i1 %op5 to i32
         %op7 = icmp ne i32 %op6, 0
         br i1 %op7, label %label8, label %label10
       label8:                                                ; preds = %label_entry
         ret i32 %arg0
       label10:                                                ; preds = %label_entry
         %op15 = sdiv i32 %arg0, %arg1
         %op17 = mul i32 %op15, %arg1
         %op18 = sub i32 %arg0, %op17
         %op19 = call i32 @gcd(i32 %arg1, i32 %op18)
         ret i32 %op19
       label20:
         ret i32 0
       }
       ```

       是因为生成的.ll文件自动补全了函数返回值0，一般来说这种自动补全的不可达块不会出现活跃变量，只会返回常数，但由此引发我思考：不可达块中的变量如何处理。如下程序，return之后的为不可达块，但确实出现了活跃变量

       ```c
       int gcd(){
           return 0;
           int a = 1;
           int b = 2;
       }
       ```

       问题本质在于该流图为一个非连通图，因而深度优先遍历的逆序处理时，不可达块不会被处理到，将深度优先遍历修改为对非连通图适用即可。

       询问过助教后得知样例不会出现这种情形，因此保留原先的版本。

### 实验总结

此次实验有什么收获

对于活跃变量的处理和块间控制流的转移有了更深刻的认识，对于c++应用，logging工具调试debug有了更深的掌握，锻炼了程序开发的大局观。

### 实验反馈 （可选 不会评分）

对本次实验的建议

### 组间交流 （可选）

本次实验和哪些组（记录组长学号）交流了哪一部分信息
