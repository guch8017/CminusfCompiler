#ifndef SYSYC_INSTRUCTION_H
#define SYSYC_INSTRUCTION_H

#include "User.h"
#include "Type.h"
#include "BasicBlock.h"
#include <map>

class BasicBlock;
class Function;

class Instruction : public User
{
public:
    enum OpID
    {
        // Terminator Instructions
        ret,
        br,
        // Standard binary operators
        add,
        sub,
        mul,
        sdiv,
        // float binary operators
        fadd,
        fsub,
        fmul,
        fdiv,
        // Memory operators
        alloca,
        load,
        store,
        // Other operators
        cmp,
        fcmp,
        phi,
        call,
        getelementptr, 
        zext, // zero extend
        fptosi,
        sitofp
        // float binary operators Logical operators

    };
    // create instruction, auto insert to bb
    // ty here is result type
    Instruction(Type *ty, OpID id, unsigned num_ops,
                BasicBlock *parent);
    Instruction(Type *ty, OpID id, unsigned num_ops);
    inline const BasicBlock *get_parent() const { return parent_; }
    inline BasicBlock *get_parent() { return parent_; }
    void set_parent(BasicBlock *parent) { this->parent_ = parent; }
    // Return the function this instruction belongs to.
    Function *get_function();
    Module *get_module();

    /// ============= INLINE OPTIMIZATION HELPER FUNCTIONS ==============

    // 创建一个指令的深拷贝
    virtual Instruction* deepcopy(BasicBlock* parent) = 0;
    // 利用map映射替换指令内部所有指针到新值
    virtual void transplant(std::map<Value*, Value*> ptMap){
        // 替换UseList
        auto it = use_list_.begin();
        auto ite = use_list_.end();
        while (it != ite)
        {
            auto u = *it;
            if(ptMap.find(u.val_) != ptMap.end()){
                *it = Use(ptMap[u.val_], u.arg_no_);
            }
            it++;
        }
        // 替换Operands
        auto it1 = operands_.begin();
        auto ite1 = operands_.end();
        while(it1 != ite1){
            auto o =  *it1;
            if(ptMap.find(o) != ptMap.end()){
                *it1 = ptMap[o];
            }
            it1++;
        }
        
    };

    /// ============= INLINE OPTIMIZATION HELPER FUNCTIONS ==============


    OpID get_instr_type() { return op_id_; }
    std::string get_instr_op_name() {
        switch (op_id_)
        {
            case ret: return "ret"; break;
            case br: return "br"; break;
            case add: return "add"; break;
            case sub: return "sub"; break;
            case mul: return "mul"; break;
            case sdiv: return "sdiv"; break;
            case fadd: return "fadd"; break;
            case fsub: return "fsub"; break;
            case fmul: return "fmul"; break;
            case fdiv: return "fdiv"; break;
            case alloca: return "alloca"; break;
            case load: return "load"; break;
            case store: return "store"; break;
            case cmp: return "cmp"; break;
            case fcmp: return "fcmp"; break;
            case phi: return "phi"; break;
            case call: return "call"; break;
            case getelementptr: return "getelementptr"; break;
            case zext: return "zext"; break;
            case fptosi: return "fptosi"; break;
            case sitofp: return "sitofp"; break;
        
        default: return ""; break;
        }
    }



    bool is_void() { return ((op_id_ == ret) || (op_id_ == br) || (op_id_ == store) || (op_id_ == call && this->get_type()->is_void_type())); }

    bool is_phi() { return op_id_ == phi; }
    bool is_store() { return op_id_ == store; }
    bool is_alloca() { return op_id_ == alloca; }
    bool is_ret() { return op_id_ == ret; }
    bool is_load() { return op_id_ == load; }
    bool is_br() { return op_id_ == br; }

    bool is_add() { return op_id_ == add; }
    bool is_sub() { return op_id_ == sub; }
    bool is_mul() { return op_id_ == mul; }
    bool is_div() { return op_id_ == sdiv; }
    
    
    bool is_fadd() { return op_id_ == fadd; }
    bool is_fsub() { return op_id_ == fsub; }
    bool is_fmul() { return op_id_ == fmul; }
    bool is_fdiv() { return op_id_ == fdiv; }
    bool is_fp2si() { return op_id_ == fptosi; }
    bool is_si2fp() { return op_id_ == sitofp; }

    bool is_cmp() { return op_id_ == cmp; }
    bool is_fcmp() { return op_id_ == fcmp; }

    bool is_call() { return op_id_ == call; }
    bool is_gep() { return op_id_ == getelementptr; }
    bool is_zext() { return op_id_ == zext; }
    

    bool isBinary()
    {
        return (is_add() || is_sub() || is_mul() || is_div() ||
                is_fadd() || is_fsub() || is_fmul() || is_fdiv()) &&
               (get_num_operand() == 2);
    }

    bool isTerminator() { return is_br() || is_ret(); }

protected:
    BasicBlock *parent_;
    OpID op_id_;
    unsigned num_ops_;
};

class BinaryInst : public Instruction
{
private:
    BinaryInst(Type *ty, OpID id, Value *v1, Value *v2,
               BasicBlock *bb);
    BinaryInst(Type *ty, OpID id, BasicBlock *bb): Instruction(ty, id, 2, bb){};

public:
    // create add instruction, auto insert to bb
    static BinaryInst *create_add(Value *v1, Value *v2, BasicBlock *bb, Module *m);

    // create sub instruction, auto insert to bb
    static BinaryInst *create_sub(Value *v1, Value *v2, BasicBlock *bb, Module *m);

    // create mul instruction, auto insert to bb
    static BinaryInst *create_mul(Value *v1, Value *v2, BasicBlock *bb, Module *m);

    // create Div instruction, auto insert to bb
    static BinaryInst *create_sdiv(Value *v1, Value *v2, BasicBlock *bb, Module *m);

    // create fadd instruction, auto insert to bb
    static BinaryInst *create_fadd(Value *v1, Value *v2, BasicBlock *bb, Module *m);

    // create fsub instruction, auto insert to bb
    static BinaryInst *create_fsub(Value *v1, Value *v2, BasicBlock *bb, Module *m);

    // create fmul instruction, auto insert to bb
    static BinaryInst *create_fmul(Value *v1, Value *v2, BasicBlock *bb, Module *m);

    // create fDiv instruction, auto insert to bb
    static BinaryInst *create_fdiv(Value *v1, Value *v2, BasicBlock *bb, Module *m);

    static BinaryInst *create_s_bin(Value *v1, Value *v2, OpID op, BasicBlock *bb, Module *m){
        return new BinaryInst(Type::get_int32_type(m), op, v1, v2, bb);
    };

    static BinaryInst *create_f_bin(Value *v1, Value *v2, OpID op, BasicBlock *bb, Module *m){
        return new BinaryInst(Type::get_float_type(m), op, v1, v2, bb);
    };

    virtual BinaryInst* deepcopy(BasicBlock* parent) override{
        // 复制基本信息
        BinaryInst* newInst = new BinaryInst(type_, op_id_, parent);
        // 复制UseList
        newInst->use_list_.clear();
        for(auto u: use_list_){
            newInst->use_list_.push_back(u);
        }
        // 复制Operands
        newInst->operands_.clear();
        for(auto o: operands_){
            newInst->operands_.push_back(o);
        }
        return newInst;
    };

    virtual std::string print() override;

private:
    void assertValid();
};

class CmpInst : public Instruction
{
public:
    enum CmpOp
    {
        EQ, // ==
        NE, // !=
        GT, // >
        GE, // >=
        LT, // <
        LE  // <=
    };

private:
    CmpInst(Type *ty, CmpOp op, Value *lhs, Value *rhs,
            BasicBlock *bb);
    CmpInst(Type *ty, CmpOp op, BasicBlock* bb): Instruction(ty, Instruction::cmp, 2, bb), cmp_op_(op){};

public:
    static CmpInst *create_cmp(CmpOp op, Value *lhs, Value *rhs,
                               BasicBlock *bb, Module *m);

    CmpOp get_cmp_op() { return cmp_op_; }

    virtual CmpInst* deepcopy(BasicBlock* parent) override{
        // 复制基本信息
        CmpInst* newInst = new CmpInst(type_, cmp_op_, parent);
        // 复制UseList
        newInst->use_list_.clear();
        for(auto u: use_list_){
            newInst->use_list_.push_back(u);
        }
        // 复制Operands
        newInst->operands_.clear();
        for(auto o: operands_){
            newInst->operands_.push_back(o);
        }
        return newInst;
    };

    virtual std::string print() override;

private:
    CmpOp cmp_op_;

    void assertValid();
};

class FCmpInst : public Instruction
{
public:
    enum CmpOp
    {
        EQ, // ==
        NE, // !=
        GT, // >
        GE, // >=
        LT, // <
        LE  // <=
    };

private:
    FCmpInst(Type *ty, CmpOp op, Value *lhs, Value *rhs,
             BasicBlock *bb);
    FCmpInst(Type *ty, CmpOp op, BasicBlock *bb) : Instruction(ty, Instruction::fcmp, 2, bb), cmp_op_(op){};

public:
    static FCmpInst *create_fcmp(CmpOp op, Value *lhs, Value *rhs,
                                 BasicBlock *bb, Module *m);

    CmpOp get_cmp_op() { return cmp_op_; }

    virtual std::string print() override;

    virtual FCmpInst* deepcopy(BasicBlock* parent) override{
        // 复制基本信息
        FCmpInst* newInst = new FCmpInst(type_, cmp_op_, parent);
        // 复制UseList
        newInst->use_list_.clear();
        for(auto u: use_list_){
            newInst->use_list_.push_back(u);
        }
        // 复制Operands
        newInst->operands_.clear();
        for(auto o: operands_){
            newInst->operands_.push_back(o);
        }
        return newInst;
    };

private:
    CmpOp cmp_op_;

    void assert_valid();
};

class CallInst : public Instruction
{
protected:
    CallInst(Function *func, std::vector<Value *> args, BasicBlock *bb);

    CallInst(Type* typ, size_t sz, BasicBlock *bb): Instruction(typ, Instruction::call, sz, bb){};

public:
    static CallInst *create(Function *func, std::vector<Value *> args, BasicBlock *bb);
    FunctionType *get_function_type() const;

    virtual std::string print() override;

    virtual CallInst* deepcopy(BasicBlock* parent) override{
        // 复制基本信息
        CallInst* newInst = new CallInst(type_, operands_.size(), parent);
        // 复制UseList
        newInst->use_list_.clear();
        for(auto u: use_list_){
            newInst->use_list_.push_back(u);
        }
        // 复制Operands
        newInst->operands_.clear();
        for(auto o: operands_){
            newInst->operands_.push_back(o);
        }
        return newInst;
    };

    virtual void transplant(std::map<Value*, Value*> ptMap) override{
        // 替换UseList
        auto it = use_list_.begin();
        auto ite = use_list_.end();
        while (it != ite)
        {
            auto u = *it;
            if(ptMap.find(u.val_) != ptMap.end()){
                *it = Use(ptMap[u.val_], u.arg_no_);
            }
        }
        // 替换Operands，跳过第一个。第一个为函数指针，无需处理
        auto it1 = operands_.begin();
        auto ite1 = operands_.end();
        it1++;
        while(it1 != ite1){
            auto o =  *it1;
            if(ptMap.find(o) != ptMap.end()){
                *it1 = ptMap[o];
            }
        }
        
    };
};

class BranchInst : public Instruction
{
private:
    BranchInst(Value *cond, BasicBlock *if_true, BasicBlock *if_false,
               BasicBlock *bb);
    BranchInst(BasicBlock *if_true, BasicBlock *bb);
    BranchInst(int op_num, BasicBlock *bb);

public:
    static BranchInst *create_cond_br(Value *cond, BasicBlock *if_true, BasicBlock *if_false,
                                      BasicBlock *bb);
    static BranchInst *create_br(BasicBlock *if_true, BasicBlock *bb);

    bool is_cond_br() const;

    virtual std::string print() override;

    virtual BranchInst* deepcopy(BasicBlock* parent) override{
        // 复制基本信息
        BranchInst* newInst = new BranchInst(num_ops_, parent);
        // 复制UseList
        newInst->use_list_.clear();
        for(auto u: use_list_){
            newInst->use_list_.push_back(u);
        }
        // 复制Operands
        newInst->operands_.clear();
        for(auto o: operands_){
            newInst->operands_.push_back(o);
        }
        return newInst;
    };
};

class ReturnInst : public Instruction
{
private:
    ReturnInst(Value *val, BasicBlock *bb);
    ReturnInst(BasicBlock *bb);
    ReturnInst(BasicBlock *bb, size_t num_op);

public:
    static ReturnInst *create_ret(Value *val, BasicBlock *bb);
    static ReturnInst *create_void_ret(BasicBlock *bb);
    bool is_void_ret() const;

    virtual std::string print() override;

    virtual ReturnInst* deepcopy(BasicBlock* parent) override{
        // 复制基本信息
        ReturnInst* newInst = new ReturnInst(parent, num_ops_);
        // 复制UseList
        newInst->use_list_.clear();
        for(auto u: use_list_){
            newInst->use_list_.push_back(u);
        }
        // 复制Operands
        newInst->operands_.clear();
        for(auto o: operands_){
            newInst->operands_.push_back(o);
        }
        return newInst;
    };
};

class GetElementPtrInst : public Instruction
{
private:
    GetElementPtrInst(Value *ptr, std::vector<Value *> idxs, BasicBlock *bb);
    GetElementPtrInst(Type* ty, size_t op_num, BasicBlock* bb): Instruction(ty, Instruction::getelementptr, op_num, bb), element_ty_(ty){};


public:
    static Type *get_element_type(Value *ptr, std::vector<Value *> idxs);
    static GetElementPtrInst *create_gep(Value *ptr, std::vector<Value *> idxs, BasicBlock *bb);
    Type *get_element_type() const;

    virtual std::string print() override;

    virtual GetElementPtrInst* deepcopy(BasicBlock* parent) override{
        // 复制基本信息
        GetElementPtrInst* newInst = new GetElementPtrInst(element_ty_, num_ops_, parent);
        // 复制UseList
        newInst->use_list_.clear();
        for(auto u: use_list_){
            newInst->use_list_.push_back(u);
        }
        // 复制Operands
        newInst->operands_.clear();
        for(auto o: operands_){
            newInst->operands_.push_back(o);
        }
        return newInst;
    };

private:
    Type *element_ty_;
};

class StoreInst : public Instruction
{
private:
    StoreInst(Value *val, Value *ptr, BasicBlock *bb);
    StoreInst(BasicBlock *bb);

public:
    static StoreInst *create_store(Value *val, Value *ptr, BasicBlock *bb);

    Value *get_rval() { return this->get_operand(0); }
    Value *get_lval() { return this->get_operand(1); }

    virtual std::string print() override;

    virtual StoreInst* deepcopy(BasicBlock* parent) override{
        // 复制基本信息
        StoreInst* newInst = new StoreInst(parent);
        // 复制UseList
        newInst->use_list_.clear();
        for(auto u: use_list_){
            newInst->use_list_.push_back(u);
        }
        // 复制Operands
        newInst->operands_.clear();
        for(auto o: operands_){
            newInst->operands_.push_back(o);
        }
        return newInst;
    };
};

class LoadInst : public Instruction
{
private:
    LoadInst(Type *ty, Value *ptr, BasicBlock *bb);
    LoadInst(Type *ty, BasicBlock *bb): Instruction(ty, Instruction::load, 1, bb){};
public:
    static LoadInst *create_load(Type *ty, Value *ptr, BasicBlock *bb);
    Value *get_lval() { return this->get_operand(0); }

    Type *get_load_type() const;

    virtual std::string print() override;

    virtual LoadInst* deepcopy(BasicBlock* parent) override{
        // 复制基本信息
        LoadInst* newInst = new LoadInst(type_, parent);
        // 复制UseList
        newInst->use_list_.clear();
        for(auto u: use_list_){
            newInst->use_list_.push_back(u);
        }
        // 复制Operands
        newInst->operands_.clear();
        for(auto o: operands_){
            newInst->operands_.push_back(o);
        }
        return newInst;
    };
};

class AllocaInst : public Instruction
{
private:
    AllocaInst(Type *ty, BasicBlock *bb);

public:
    static AllocaInst *create_alloca(Type *ty, BasicBlock *bb);

    Type *get_alloca_type() const;

    virtual std::string print() override;

    virtual AllocaInst* deepcopy(BasicBlock* parent) override{
        // 复制基本信息
        AllocaInst* newInst = new AllocaInst(alloca_ty_, parent);
        // 复制UseList
        newInst->use_list_.clear();
        for(auto u: use_list_){
            newInst->use_list_.push_back(u);
        }
        newInst->operands_.clear();
        return newInst;
    };

private:
    Type *alloca_ty_;
};

class ZextInst : public Instruction
{
private:
    ZextInst(OpID op, Value *val, Type *ty, BasicBlock *bb);
    ZextInst(Type* ty, BasicBlock* bb): Instruction(ty, Instruction::zext, 1, bb), dest_ty_(ty){};

public:
    static ZextInst *create_zext(Value *val, Type *ty, BasicBlock *bb);

    Type *get_dest_type() const;

    virtual std::string print() override;

    virtual ZextInst* deepcopy(BasicBlock* parent) override{
        // 复制基本信息
        ZextInst* newInst = new ZextInst(type_, parent);
        // 复制UseList
        newInst->use_list_.clear();
        for(auto u: use_list_){
            newInst->use_list_.push_back(u);
        }
        // 复制Operands
        newInst->operands_.clear();
        for(auto o: operands_){
            newInst->operands_.push_back(o);
        }
        return newInst;
    };

private:
    Type *dest_ty_;
};

class FpToSiInst : public Instruction
{
private:
    FpToSiInst(OpID op, Value *val, Type *ty, BasicBlock *bb);
    FpToSiInst(Type *ty, BasicBlock *bb): Instruction(ty, Instruction::fptosi, 1, bb), dest_ty_(ty){};

public:
    static FpToSiInst *create_fptosi(Value *val, Type *ty, BasicBlock *bb);

    Type *get_dest_type() const;

    virtual std::string print() override;

    virtual FpToSiInst* deepcopy(BasicBlock* parent) override{
        // 复制基本信息
        FpToSiInst* newInst = new FpToSiInst(type_, parent);
        // 复制UseList
        newInst->use_list_.clear();
        for(auto u: use_list_){
            newInst->use_list_.push_back(u);
        }
        // 复制Operands
        newInst->operands_.clear();
        for(auto o: operands_){
            newInst->operands_.push_back(o);
        }
        return newInst;
    };

private:
    Type *dest_ty_;
};

class SiToFpInst : public Instruction
{
private:
    SiToFpInst(OpID op, Value *val, Type *ty, BasicBlock *bb);
    SiToFpInst(Type *ty, BasicBlock *bb): Instruction(ty, Instruction::sitofp, 1, bb), dest_ty_(ty){};
public:
    static SiToFpInst *create_sitofp(Value *val, Type *ty, BasicBlock *bb);

    Type *get_dest_type() const;

    virtual std::string print() override;

    virtual SiToFpInst* deepcopy(BasicBlock* parent) override{
        // 复制基本信息
        SiToFpInst* newInst = new SiToFpInst(type_, parent);
        // 复制UseList
        newInst->use_list_.clear();
        for(auto u: use_list_){
            newInst->use_list_.push_back(u);
        }
        // 复制Operands
        newInst->operands_.clear();
        for(auto o: operands_){
            newInst->operands_.push_back(o);
        }
        return newInst;
    };

private:
    Type *dest_ty_;
};

class PhiInst : public Instruction
{
private:
    PhiInst(OpID op, std::vector<Value *> vals, std::vector<BasicBlock *> val_bbs, Type *ty, BasicBlock *bb);
    PhiInst(Type *ty, unsigned num_ops, BasicBlock *bb)
        : Instruction(ty, Instruction::phi, num_ops, bb) {}
    Value *l_val_;

public:
    static PhiInst *create_phi(Type *ty, BasicBlock *bb);
    Value *get_lval() { return l_val_; }
    void set_lval(Value *l_val) { l_val_ = l_val; }
    void add_phi_pair_operand(Value *val, Value *pre_bb)
    {
        this->add_operand(val);
        this->add_operand(pre_bb);
    }
    virtual std::string print() override;

    virtual PhiInst* deepcopy(BasicBlock* parent) override{
        // 复制基本信息
        PhiInst* newInst = new PhiInst(type_, num_ops_, parent);
        newInst->l_val_ = l_val_;
        // 复制UseList
        newInst->use_list_.clear();
        for(auto u: use_list_){
            newInst->use_list_.push_back(u);
        }
        // 复制Operands
        newInst->operands_.clear();
        for(auto o: operands_){
            newInst->operands_.push_back(o);
        }
        return newInst;
    };

    virtual void transplant(std::map<Value*, Value*> ptMap) override{
        // 替换UseList
        auto it = use_list_.begin();
        auto ite = use_list_.end();
        while (it != ite)
        {
            auto u = *it;
            if(ptMap.find(u.val_) != ptMap.end()){
                *it = Use(ptMap[u.val_], u.arg_no_);
            }
        }
        // 替换Operands
        auto it1 = operands_.begin();
        auto ite1 = operands_.end();
        while(it1 != ite1){
            auto o =  *it1;
            if(ptMap.find(o) != ptMap.end()){
                *it1 = ptMap[o];
            }
        }
        // 替换lval
        if(ptMap.find(l_val_) != ptMap.end())
            l_val_ = ptMap[l_val_];
    };
};

#endif // SYSYC_INSTRUCTION_H
