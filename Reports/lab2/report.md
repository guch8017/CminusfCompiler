# lab2 实验报告
学号 PB18030825 姓名 顾超
## 实验要求

1. 了解 `bison` 基础知识和理解 Cminus-f 语法（重在了解如何将文法产生式转换为 `bison` 语句）
2. 阅读 `/src/common/SyntaxTree.c`，对应头文件 `/include/SyntaxTree.h`（重在理解分析树如何生成）
3. 了解 `bison` 与 `flex` 之间是如何协同工作，看懂pass_node函数并改写 Lab1 代码（提示：了解 `yylval` 是如何工作，在代码层面上如何将值传给`$1`、`$2`等）
4. 补全 `src/parser/syntax_analyzer.y` 文件和 `lexical_analyzer.l` 文件

## 实验难点

无

## 实验设计

1. 适当修改.l文件，去除无用的Token_node部分。
2. 根据cminus-f语法与助教提供的辅助函数构建产生式及并写出对应动作。

## 实验结果验证



## 实验反馈
文档指导较清晰。
需要写许多重复代码部分较为枯燥。
