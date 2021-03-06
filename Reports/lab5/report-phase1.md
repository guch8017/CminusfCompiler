# Lab5 实验报告-阶段一


组长：顾超 PB18030825
组员：王正阳 PB18030836

## 实验要求

* 阅读代码，了解`LoopSearch Pass`构造CFG与强连通分量的方法，了解该Pass运行完成后得到的用于后序优化分析过程的数据的存储位置与存储方式，为后序实验做好准备。
* 阅读代码，了解`Mem2Reg Pass`中搜索支配节点及支配边界的算法（似乎是《A Simple, Fast Dominance Algorithm》 Deith D.Cooper）。了解如何利用支配边界进行phi节点构造，以及如何巧妙设计数据结构来实现对store/load指令进行替换。

## 思考题
### LoopSearch
1. 通过`find_loop_base`方法确定。
- 若给出的是最外层循环，则在`find_loop_base`中第一阶段可以找到前继来源于强连通分量基本块集合外部的基本块(`set->find(prev) == set->end()`，即在强连通分量中找不到基本块前继，代表基本块入口来源于外部)。
- 若给出的是内层循环，则有可能满足上一种情况。而若上述情况中未找到对应的BasicBlock，说明内层循环入口在被删去的外层循环入口处，遍历被删除的节点集合`reserved`，从中找出后继节点中包含DFA中节点的基本块，被包含的基本块即为要找的入口基本块。
- 若给出的为Cminus-f生成的LLVM IR，则循环体不应包含多个入口块，因为Cminus-f并没有类似goto的破坏块完整性的语句。
2. 每找到一层循环，就将其入口块的BasicBlock从DFG中移除，从而使外层循环的基本块不再与内层循环的基本块构成强连通分量。而内层循环仍为一个完整的循环，可以被算法检测到，且不会被外层循环的基本块所干扰，在下一次循环中内层循环的入口块也能被找到。
### Mem2reg
1. 一个基本块的支配边界指函数控制流程图中，该基本块在此边界后，不再直接支配其他基本块。即在此边界后的基本块，其中的变量的值可能来源于其他基本块。
2. Phi节点根据到达Phi节点的控制流路径，返回其中某个变元的值。该节点用于实现判断等指令，合并由不同情况到达的结果。
3. 
- Function gcd
    * arg0, arg1在函数执行过程中未发生改变，故去除了对arg0, arg1新分配内存及从内存中读取对应值的操作，改为直接使用两个参数的传入值。
- Function main
    * input的返回值均未被改变，故不为其值新分配内存进行存储，对其值的调用改为直接利用返回call返回的数值。
    * 利用phi指令代替`label_entry`和`label10`中的重复赋值语句，对`op20`,`op21`根据控制流进行赋值。
4. 放置phi节点时，利用支配树算法生成的支配边界信息，在支配边界以内的函数无需考虑phi函数，因为其中变量的值仅来自于直接支配它的基本块。而在支配边界上的基本块中的变量的值可能来源多个控制路径，此时需要Phi指令来控制变量的最终值。故利用支配边界信息，可以准确确定需要插入phi节点的基本块。
5. 
- 若变量为全局变量或数组取值，则跳过替换，不进行任何处理
- 若变量为局部变量store类型，且其目标变量已被phi指令处理，则将其压到对应的栈顶，便于调用该变量时取到本应stote的值。
- 若变量为局部变量load类型，且其目标已被phi指令替换，则从对应的栈顶取出值作为原本load的返回值。
- 该实现的巧妙之处是其存储的数据结构，利用一个Map将指针类型的左值映射到一个Stack，从而可以方便的判断是否需要进行指令替换。而store指令进行的压栈操作，保证栈顶元素所代表的Value始终是最新的，在load时直接取栈顶元素，即可取到当前执行语句所需的正确值，无需其他额外的判断。

### 代码阅读总结

学到了优化的思想，了解到了根据IR指令寻找循环路径的方法，了解到了构建支配树，寻找支配边界的具体实现。

### 实验反馈 （可选 不会评分）

NULL

### 组间交流 （可选）

NULL
