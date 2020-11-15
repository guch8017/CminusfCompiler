#ifndef SYSYC_INSTRUCTION_H
#define SYSYC_INSTRUCTION_H

#include "User.h"
#include "Type.h"
#include "BasicBlock.h"

class BasicBlock;
class Function;

class Instruction : public User
{
public:
    enum OpID {
        // Terminator Instructions
        Ret,
        Br,
        // Standard binary operators
        Add,
        Sub,
        Mul,
        Div,
        // float binary operators
        FAdd,
        FSub,
        FMul,
        FDiv,
        // Memory operators
        Alloca,
        Load,
        Store,
        // Other operators
        Cmp,
        FCmp,
        PHI,
        Call,
        GEP,    // GetElementPtr
        ZExt,    // zero extend
        FpToSi,
        SiToFp
        // float binary operators Logical operators
        
    };
    // create instruction, auto insert to bb
    // ty here is result type
    Instruction(Type *ty, OpID id, unsigned num_ops,
                BasicBlock *parent);

    inline const BasicBlock *get_parent() const { return parent_; }
    inline       BasicBlock *get_parent()       { return parent_; }

    // Return the function this instruction belongs to.
    Function *get_function();
    Module *get_module();
    
    OpID get_instr_type() { return op_id_;}
    bool is_void() { return ((op_id_== Ret) 
                                || (op_id_== Br) 
                                || (op_id_ == Store) 
                                || (op_id_ == Call && this->get_type()->is_void_type())); }

private:
    BasicBlock *parent_;
    OpID op_id_;
    unsigned num_ops_;
};

class BinaryInst : public Instruction {
private:
    BinaryInst(Type *ty, OpID id, Value *v1, Value *v2, 
            BasicBlock *bb);

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

    virtual std::string print() override;

private:
    void assertValid();
};

class CmpInst : public Instruction {
public:
    enum CmpOp {
        EQ,     // ==
        NE,     // !=
        GT,     // >
        GE,     // >=
        LT,     // <
        LE      // <=
    };

private:
    CmpInst(Type *ty, CmpOp op, Value *lhs, Value *rhs, 
            BasicBlock *bb);

public:
    static CmpInst *create_cmp(CmpOp op, Value *lhs, Value *rhs, 
                        BasicBlock *bb, Module *m);
    
    CmpOp get_cmp_op() { return cmp_op_; }

    virtual std::string print() override;

private:
    CmpOp cmp_op_;

    void assertValid();

};

class FCmpInst : public Instruction {
public:
    enum CmpOp {
        EQ,     // ==
        NE,     // !=
        GT,     // >
        GE,     // >=
        LT,     // <
        LE      // <=
    };

private:
    FCmpInst(Type *ty, CmpOp op, Value *lhs, Value *rhs, 
            BasicBlock *bb);

public:
    static FCmpInst *create_fcmp(CmpOp op, Value *lhs, Value *rhs, 
                        BasicBlock *bb, Module *m);
    
    CmpOp get_cmp_op() { return cmp_op_; }

    virtual std::string print() override;

private:
    CmpOp cmp_op_;

    void assert_valid();

};

class CallInst : public Instruction {
protected:
    CallInst(Function *func, std::vector<Value *> args, BasicBlock *bb);
public:
    static CallInst *create(Function *func, std::vector<Value *> args, BasicBlock *bb);
    FunctionType *get_function_type() const;

    virtual std::string print() override;

};

class BranchInst : public Instruction {
private:
    BranchInst(Value *cond, BasicBlock *if_true, BasicBlock *if_false,
            BasicBlock *bb);
    BranchInst(BasicBlock *if_true, BasicBlock *bb);
public:
    static BranchInst *create_cond_br(Value *cond, BasicBlock *if_true, BasicBlock *if_false,
                                    BasicBlock *bb);
    static BranchInst *create_br(BasicBlock *if_true, BasicBlock *bb);

    bool is_cond_br() const;

    virtual std::string print() override;

};

class ReturnInst : public Instruction {
private:
    ReturnInst(Value *val, BasicBlock *bb);
    ReturnInst(BasicBlock *bb);
public:
    static ReturnInst *create_ret(Value *val, BasicBlock *bb);
    static ReturnInst *create_void_ret(BasicBlock *bb);
    bool is_void_ret() const;

    virtual std::string print() override;

};

class GetElementPtrInst : public Instruction {
private:
    GetElementPtrInst(Value *ptr, std::vector<Value *> idxs, BasicBlock *bb);
public:
    static Type *get_element_type(Value *ptr, std::vector<Value *> idxs);
    static GetElementPtrInst *create_gep(Value *ptr, std::vector<Value *> idxs, BasicBlock *bb);
    Type *get_element_type() const;

    virtual std::string print() override;

private:
    Type *element_ty_;
};

class StoreInst : public Instruction {
private:
    StoreInst(Value *val, Value *ptr, BasicBlock *bb);
public:
    static StoreInst *create_store(Value *val, Value *ptr, BasicBlock *bb);
    
    virtual std::string print() override;

};

class LoadInst : public Instruction {
private:
    LoadInst(Type *ty, Value *ptr, BasicBlock *bb);
public:
    static LoadInst *create_load(Type *ty, Value *ptr, BasicBlock *bb);

    Type *get_load_type() const;

    virtual std::string print() override;

};

class AllocaInst : public Instruction {
private:
    AllocaInst(Type *ty, BasicBlock *bb);
public:
    static AllocaInst *create_alloca(Type *ty, BasicBlock *bb);

    Type *get_alloca_type() const;

    virtual std::string print() override;

private:
    Type *alloca_ty_;
};

class ZextInst : public Instruction {
private:
    ZextInst(OpID op, Value *val, Type *ty, BasicBlock *bb);
public:
    static ZextInst *create_zext(Value *val, Type *ty, BasicBlock *bb);

    Type *get_dest_type() const;

    virtual std::string print() override;

private:
    Type *dest_ty_;
};

class FpToSiInst : public Instruction {
private:
    FpToSiInst(OpID op, Value *val, Type *ty, BasicBlock *bb);
public:
    static FpToSiInst *create_fptosi(Value *val, Type *ty, BasicBlock *bb);

    Type *get_dest_type() const;

    virtual std::string print() override;

private:
    Type *dest_ty_;
};

class SiToFpInst : public Instruction {
private:
    SiToFpInst(OpID op, Value *val, Type *ty, BasicBlock *bb);
public:
    static SiToFpInst *create_sitofp(Value *val, Type *ty, BasicBlock *bb);

    Type *get_dest_type() const;

    virtual std::string print() override;

private:
    Type *dest_ty_;
};

#endif // SYSYC_INSTRUCTION_H
