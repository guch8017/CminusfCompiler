#include "Module.h"

Module::Module(std::string name) 
    : module_name_(name)
{
    void_ty_ = new Type(Type::VoidTyID);
    label_ty_ = new Type(Type::LabelTyID);
    int1_ty_ = new IntegerType(1);
    int32_ty_ = new IntegerType(32);
    int32ptr_ty_ = new PointerType(int32_ty_);
    float32_ty_ = new FloatType();
    float32ptr_ty_ = new PointerType(float32_ty_);
    // init instr_id2string
    instr_id2string_.insert({ Instruction::Ret, "ret" }); 
    instr_id2string_.insert({ Instruction::Br, "br" }); 
    
    instr_id2string_.insert({ Instruction::Add, "add" });
    instr_id2string_.insert({ Instruction::Sub, "sub" });
    instr_id2string_.insert({ Instruction::Mul, "mul" });
    instr_id2string_.insert({ Instruction::Div, "sdiv" });
    
    instr_id2string_.insert({ Instruction::FAdd, "fadd" });
    instr_id2string_.insert({ Instruction::FSub, "fsub" });
    instr_id2string_.insert({ Instruction::FMul, "fmul" });
    instr_id2string_.insert({ Instruction::FDiv, "fdiv" });

    instr_id2string_.insert({ Instruction::Alloca, "alloca" });
    instr_id2string_.insert({ Instruction::Load, "load" });
    instr_id2string_.insert({ Instruction::Store, "store" });
    instr_id2string_.insert({ Instruction::Cmp, "icmp" });
    instr_id2string_.insert({ Instruction::FCmp, "fcmp" });
    instr_id2string_.insert({ Instruction::PHI, "phi" });
    instr_id2string_.insert({ Instruction::Call, "call" });
    instr_id2string_.insert({ Instruction::GEP, "getelementptr" });
    instr_id2string_.insert({ Instruction::ZExt, "zext" });
    instr_id2string_.insert({ Instruction::SiToFp, "sitofp" });
    instr_id2string_.insert({ Instruction::FpToSi, "fptosi" });
}

Module::~Module()
{
    delete void_ty_;
    delete label_ty_;
    delete int1_ty_;
    delete int32_ty_;
    delete int32ptr_ty_;
    delete float32_ty_;
    delete float32ptr_ty_;
}

Type *Module::get_void_type()
{
    return void_ty_;
}

Type *Module::get_label_type()
{
    return label_ty_;
}

IntegerType *Module::get_int1_type()
{
    return int1_ty_;
}

IntegerType *Module::get_int32_type()
{
    return int32_ty_;
}

PointerType *Module::get_int32_ptr_type()
{
    return int32ptr_ty_;
}

FloatType *Module::get_float_type()
{
    return float32_ty_;
}

PointerType *Module::get_float_ptr_type()
{
    return float32ptr_ty_;
}

void Module::add_function(Function *f)
{
    function_list_.push_back(f);
}

void Module::add_global_variable(GlobalVariable* g)
{
    global_list_.push_back(g);
}

void Module::set_print_name()
{
    
}

std::string Module::print()
{
    std::string module_ir;
    for ( auto global_val : this->global_list_)
    {
        module_ir += global_val->print();
        module_ir += "\n";
    }
    for ( auto func : this->function_list_)
    {
        module_ir += func->print();
        module_ir += "\n";
    }
    return module_ir;
}
