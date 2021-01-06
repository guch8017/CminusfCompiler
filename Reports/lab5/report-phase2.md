# Lab5 实验报告

小组成员 姓名 学号
1. 顾超 PB18030825
2. 王正阳 PB18030836

## 实验要求

请按照自己的理解，写明本次实验需要干什么

+ 活跃变量分析：根据老师上课所讲述的活跃变量分析方法，得到流图的深度优先遍历逆序与各块的def，use集合，根据活跃变量分析表达式分析bb块的入口和出口的活跃变量，关注phi指令的处理

## 实验难点

* 常量传播
  * 对全局变量的处理

* 循环不变式外提
  * 对循环不变式的识别

+ 活跃变量
  + phi指令的特殊处理：修改数据流方程
  + 不可达块的处理

## 实验设计

* 常量传播
    实现思路：
      * 普通的四则运算：识别两操作数，若均为常量类型则将指令替换为两者的计算结果。此处不考虑除数为0的情况。注意区分整型与浮点型。
      * 字拓展指令：若为右值为常量则左值替换为常量，注意bool型变量需转换为整型传入ConstantInt::get中，否则将产生i1型常量。
      * 浮点/整型转换指令：思路同字拓展指令。
      * 比较指令：若所有右值均为常量则进行计算，结果存入i1型常量中替换指令。
      * 条件跳转指令：若条件取值为定值，则根据该值执行无条件跳转。
      * store/load指令：采用了TA的SSA化实现中处理全局变量的思路，利用Map+Stack存储指针对应变量的最新值，重复的load指令可被优化掉。若先store后load也可被优化处理。
    相应代码：
      * 四则运算/整型、浮点比较（以整型四则运算为例）
      ```c++
      if(IS_BINARY(instr)){
          // 整数/浮点数四则运算
          BinaryInst* inst = dynamic_cast<BinaryInst*>(instr);
          Instruction::OpID op = instr->get_instr_type();
          Value* lhs = inst->get_operand(0);
          Value* rhs = inst->get_operand(1);
          // 若两者均为常量，说明可以执行替换操作
          if(IS_CONST_INT(lhs) && IS_CONST_INT(rhs)){
              // 提前计算对应的值，并用该常量替换所有引用到该指令结果的位置
              inst->replace_all_use_with(compute_binary(op, lhs, rhs, m_));
              // 删除该指令
              instrToBeDelete.insert(inst);
          }
      }
      ```
      * 字拓展/浮点整型转换（以字拓展为例）
      ```c++
      if(instr->is_zext()){
        // Zext扩展指令
        //  操作数为常量，执行优化
        if(IS_CONST_INT(instr->get_operand(0))){
            // 直接替换为常量的值，然后删除该指令
            instr->replace_all_use_with(ConstantInt::get(dynamic_cast<ConstantInt*>(instr->get_operand(0))->get_value(), m_));
            instrToBeDelete.insert(instr);
        }
      }
      ```
      * store指令
      ```c++
      // 以下为变量定义，实际位于循环入口处
      std::unordered_map<Value*, std::vector<Value*>> pointerConst;
      // 由于map的特性，若不存在该项则会自动初始化一个空的vector，故不用判断指针对应的元素是否已经存在。
      // 无论map中原本有无该指针对应的值，store操作后栈顶的元素对应的值必然是刚刚存入的值，符合函数执行预期，用于后序load指令的处理
      pointerConst[instr->get_operand(1)].push_back(instr->get_operand(0));
      ```
      * load指令
      ```c++
      // 若map中存在对应指针的值，说明我们已经拥有指针指向的地址的最新的值，故直接将load指令替换为该值即可
      if(CONTAIN(pointerConst, instr->get_operand(0))){
          instr->replace_all_use_with(pointerConst[instr->get_operand(0)].back());
      }
      // 若不包含，则正常执行该load指令，若以后再次尝试读取该地址则可以省去一次load操作
      else{
          pointerConst[instr->get_operand(0)].push_back(instr);
      }
      ```
      * 条件跳转语句
      ```c++
      if(brInstr->is_cond_br()){
        // 处理条件跳转指令
        Value* cond = brInstr->get_operand(0);
        BasicBlock* trueBB = dynamic_cast<BasicBlock*>(brInstr->get_operand(1));
        BasicBlock* falseBB = dynamic_cast<BasicBlock*>(brInstr->get_operand(2));
        // 若条件是常量
        if(IS_CONST_INT(cond) || IS_CONST_FP(cond)){
            bool _tar;
            if(IS_CONST_INT(cond)){
                _tar  = dynamic_cast<ConstantInt*>(cond)->get_value() != 0;
            }else{
                _tar  = dynamic_cast<ConstantFP*>(cond)->get_value() != 0;
            }
            // 判断条件满足何种要求
            if(_tar){
                // 恒成立跳转
                brInstr->replace_all_use_with(BranchInst::create_br(trueBB,  bb));
                if(!CONTAIN(visitedBB, trueBB)){
                    visitedBB.insert(trueBB);
                    bbQueue.push(trueBB);
                }
            }else{
                // 恒不成立跳转
                brInstr->replace_all_use_with(BranchInst::create_br(falseBB,  bb));
                if(!CONTAIN(visitedBB, falseBB)){
                    visitedBB.insert(falseBB);
                    bbQueue.push(falseBB);
                }
            }
            instrToBeDelete.insert(instr);
        }
    }
    ```
    优化前后的IR对比（举一个例子）并辅以简单说明：
    cminusf语言原文
    ```c++
    void main(void){
      int a;
      int b;
      int c;
      float aa;
      float bb;
      float cc;
      a = 1;
      b = 2;
      aa = 1.0;
      bb = 2.0;
      /* Binary Demo  */
      c = a + b;
      cc = aa + bb;
      /* Branch Demo */
      if(a){
        b = 2;
      }
      else{
        b = 3;
      }
      /* Zext Demo */
      c = a == a;
      /* Fp2Si/Si2Fp Demo */
      aa = a;
      a = bb;
      /* Global Demo */
      gb = 1;
      gb = 2;
      output(gb);
    }
    ```
    优化前
    ```llvm
    define void @main() {
    label_entry:
      %op8 = add i32 1, 2
      %op11 = fadd float 0x3ff0000000000000, 0x4000000000000000
      %op13 = icmp ne i32 1, 0
      br i1 %op13, label %label14, label %label15
    label14:                                                ; preds = %label_entry
      br label %label16
    label15:                                                ; preds = %label_entry
      br label %label16
    label16:                                                ; preds = %label14, %label15
      %op26 = phi i32 [ 2, %label14 ], [ 3, %label15 ]
      %op19 = icmp eq i32 1, 1
      %op20 = zext i1 %op19 to i32
      %op22 = sitofp i32 1 to float
      %op24 = fptosi float 0x4000000000000000 to i32
      store i32 1, i32* @gb
      store i32 2, i32* @gb
      %op25 = load i32, i32* @gb
      call void @output(i32 %op25)
      ret void
    }
    ```
    优化后
    ```llvm
    define void @main() {
    label_entry:
      br label %label14 ; 此处整型加法、浮点加法已被优化。比较与跳转被提前计算，不可达分支被删除
    label14:                                                ; preds = %label_entry%label_entry
      br label %label16
    label16:                                                ; preds = %label14
      %op26 = phi i32 [ 2, %label14 ]
      store i32 1, i32* @gb
      store i32 2, i32* @gb
      call void @output(i32 2)  ; 此处对全局变量的读操作已被优化为已知值2
      ret void
    }
    ```

* 循环不变式外提
    实现思路：
    1. 遍历通过TA提供的循环搜索Pass获取到的所有循环。
    2. 对每个循环体内所有指令执行遍历，存储一个循环体内所有指令的左值构成集合。
    3. 再次遍历循环体内的所有指令，跳过br、call、load、store、phi等可能会导致外部环境变化的代码。剩余指令中若右值全部不在第2步存储的集合中，则说明该指令是循环不变的，因为其右值在循环中全部未改变。将该指令外提到循环入口处。
    4. 若2-3步中出现了指令外提，则转2，否则进入下一个循环遍历。因为外提的不变式可能导致新的不变式产生。
    相应代码：
    ```c++
    // 对所有循环进行处理
    for(Function* func: m_->get_functions()){
        for(auto bbsets: loop_searcher.get_loops_in_func(func)){
            // 若发生了改变需要进行迭代
            // 一次外提操作后可能带来新的不变量
            bool has_change = true;
            while (has_change)
            {
                has_change = false;
                // 保存循环内所有左值到一个集合中，其余变量则为外部来的
                std::unordered_set<Value*> exists;
                for(BasicBlock* bb: *bbsets){
                    for(Instruction* ins: bb->get_instructions()){
                        
                        exists.insert(ins);
                    }
                }
                // 二次遍历，若发现不变式则外提至主入口处
                std::set<std::pair<BasicBlock*, Instruction*>>  tbr;
                for(BasicBlock* bb: *bbsets){
                    for(Instruction* ins: bb->get_instructions()){
                        // call/phi指令返回值无法确定，即使其未运用到循环体内变量也不外提，load/store返回值不能保证一致，跳过
                        if(ins->is_phi() ||  ins->is_br() || ins->is_call() || ins->is_load() || ins->is_store()) continue;
                        bool t = true;
                        // 遍历所有操作数，若包含内部变量则跳过
                        for(Value* val: ins->get_operands()){
                            if(CONTAIN(exists, val)){
                                t  =  false;
                                break;
                            }
                        }
                        // 全部操作数均来自循环外部，说明可以外提，添加到待删除列表
                        if(t){
                            tbr.insert({bb, ins});
                            exists.erase(ins);
                            // 发生了变化，准备再次执行此过程
                            has_change = true;
                        }
                    }
                }
                // 替换位置
                BasicBlock* bb;
                Instruction* ins;
                for(auto pair: tbr){
                    std::tie(bb, ins) = pair;
                    // 此处取第1个前驱块，也就是进入循环的入口，后序的前驱都是循环内部回来的
                    BasicBlock* target = *loop_searcher.get_loop_base(bbsets)->get_pre_basic_blocks().begin();
                    if(target == nullptr) continue;
                    // 将外提的指令插入到入口块的终结指令（br）前
                    Instruction*  termIns = target->get_terminator();
                    target->get_instructions().remove(termIns);
                    ins->set_parent(target);
                    target->add_instruction(ins);
                    target->add_instruction(termIns);
                    bb->get_instructions().remove(ins);
                }
            }
            
        }
    }
    ```
    优化前后的IR对比（举一个例子）并辅以简单说明：
    测试cminus文件
    ```c++
    void main(void){
      int a;
      int b;
      int c;
      int d;
      a = input();
      b = input();
      while(a){
        c = a + b;
        d = input();
        while(d){
          a = input();
          c = b + d;
        }
      }
    }
    ```
    优化前:
    ```llvm
    define void @main() {
      label_entry:
        %op4 = call i32 @input()
        %op5 = call i32 @input()
        br label %label6
      label6:                                                ; preds = %label_entry, %label23
        %op24 = phi i32 [ %op13, %label23 ], [ undef, %label_entry ]
        %op25 = phi i32 [ %op27, %label23 ], [ undef, %label_entry ]
        %op26 = phi i32 [ %op4, %label_entry ], [ %op28, %label23 ]
        %op8 = icmp ne i32 %op26, 0
        br i1 %op8, label %label9, label %label14
      label9:                                                ; preds = %label6
        %op12 = add i32 %op26, %op5
        %op13 = call i32 @input()
        br label %label15
      label14:                                                ; preds = %label6
        ret void
      label15:                                                ; preds = %label9, %label18
        %op27 = phi i32 [ %op12, %label9 ], [ %op22, %label18 ]
        %op28 = phi i32 [ %op26, %label9 ], [ %op19, %label18 ]
        %op17 = icmp ne i32 %op13, 0
        br i1 %op17, label %label18, label %label23
      label18:                                                ; preds = %label15
        %op19 = call i32 @input()
        %op22 = add i32 %op5, %op13
        br label %label15
      label23:                                                ; preds = %label15
        br label %label6
      }
    ```
    优化后
    ```llvm
    define void @main() {
      label_entry:
        %op4 = call i32 @input()
        %op5 = call i32 @input()
        br label %label6
      label6:                                                ; preds = %label_entry, %label23
        %op24 = phi i32 [ %op13, %label23 ], [ undef, %label_entry ]
        %op25 = phi i32 [ %op27, %label23 ], [ undef, %label_entry ]
        %op26 = phi i32 [ %op4, %label_entry ], [ %op28, %label23 ]
        %op8 = icmp ne i32 %op26, 0
        br i1 %op8, label %label9, label %label14
      label9:                                                ; preds = %label6
        %op12 = add i32 %op26, %op5   ; c = a + b没有外提，因为循环中他的值可能发生改变
        %op13 = call i32 @input()
        %op17 = icmp ne i32 %op13, 0  ; 对 while(d) 中是否为0的判断被外提
        %op22 = add i32 %op5, %op13   ; c = b + d被外提
        br label %label15
      label14:                                                ; preds = %label6
        ret void
      label15:                                                ; preds = %label9, %label18
        %op27 = phi i32 [ %op12, %label9 ], [ %op22, %label18 ]
        %op28 = phi i32 [ %op26, %label9 ], [ %op19, %label18 ]
        br i1 %op17, label %label18, label %label23
      label18:                                                ; preds = %label15
        %op19 = call i32 @input()
        br label %label15
      label23:                                                ; preds = %label15
        br label %label6
    }
    ```
    
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

更加深刻的理解了常量传播与循环不变式外提的具体处理方法。学会了使用恰当的数据结构来处理关系。

对于活跃变量的处理和块间控制流的转移有了更深刻的认识，对于c++应用，logging工具调试debug有了更深的掌握，锻炼了程序开发的大局观。

### 实验反馈 （可选 不会评分）

对本次实验的建议

### 组间交流 （可选）

本次实验和哪些组（记录组长学号）交流了哪一部分信息
