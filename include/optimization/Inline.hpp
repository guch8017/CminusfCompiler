#pragma once
#include "Module.h"
#include "Function.h"
#include "IRBuilder.h"
#include "BasicBlock.h"
#include "Instruction.h"
#include "PassManager.hpp"

class Inline : public Pass
{
private:
    static int opID;
    std::set<Function *> recurFunction;
public:
    Inline(Module *m) : Pass(m){}
    ~Inline(){};
    void run() override;
private:

    // 抽取所有CallInst类型的指令，同时分析Function价值（并没有利用
    void callAnalysis();
    /**
     * 拆分基本块
     * 
     * @note inst必须为targetBB内的指令，否则将返回 ::nullptr
     * 
     * @param targetBB 待拆分的基本块
     * @param inst 以该指令为分界进行拆分
     * 
     * @return 拆分后的新块地址
     */
    BasicBlock* splitBasicBlock(BasicBlock* targetBB, CallInst* inst);
    /**
     * 函数内联化
     * 
     * @param func 待处理的函数
     * @param target 被插入函数
     * @param entry 调用者所在的基本块拆分后的入口块
     * @param exit_ 调用者所在的基本块拆分后的出口块
     * @param caller 调用该函数的指令
     * 
     * @note 执行本指令后函数结构将变为 entry -> | inlineBasicBlocks | -> exit_
     * 
     * @return 内联化函数入口点
     */
    BasicBlock* transplant(Function* func, Function* target,  BasicBlock* entry, BasicBlock* exit_, CallInst* caller);

    

    std::map<Function*, int> instrCountMap;
    std::vector<std::tuple<Function*, BasicBlock*, CallInst*, int>> callInstrHotSpot;
    std::string getName(){ return "inline_" + std::to_string(++opID); }
};

