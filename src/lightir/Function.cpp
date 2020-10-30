#include "Module.h"
#include "Function.h"
#include "IRprinter.h"

Function::Function(FunctionType *ty, const std::string &name, Module *parent)
    : Value(ty, name), parent_(parent)
{
    // num_args_ = ty->getNumParams();
    parent->add_function(this);
    build_args();
}

Function *Function::create(FunctionType *ty, const std::string &name, Module *parent)
{
    return new Function(ty, name, parent);
}

FunctionType *Function::get_function_type() const
{
    return static_cast<FunctionType *>(get_type());
}

Type *Function::get_return_type() const
{
    return get_function_type()->get_return_type();
}

unsigned Function::get_num_of_args() const
{
    return get_function_type()->get_num_of_args();
}

Module *Function::get_parent() const
{
    return parent_;
}

void Function::build_args()
{
    auto *func_ty = get_function_type();
    unsigned num_args = get_num_of_args();
    for (int i = 0; i < num_args; i++) {
        arguments_.push_back(new Argument(func_ty->get_param_type(i), "", this, i));
    }
}

void Function::add_basic_block(BasicBlock *bb)
{
    basic_blocks_.push_back(bb);
}

void Function::set_instr_name()
{
    std::map<Value *, int> seq;
    for (auto arg : this->get_args())
    {
        if ( seq.find(arg) == seq.end())
        {
            auto seq_num = seq.size();
            if ( arg->set_name(std::to_string(seq_num) ))
            {
                seq.insert( {arg, seq_num} );
            }
        }
    }
    for (auto bb : basic_blocks_)
    {
        if ( seq.find(bb) == seq.end())
        {
            auto seq_num = seq.size();
            if ( bb->set_name(std::to_string(seq_num) ))
            {
                seq.insert( {bb, seq_num} );
            }
        }
        for (auto instr : bb->get_instructions())
        {
            if ( !instr->is_void() && seq.find(instr) == seq.end())
            {
                auto seq_num = seq.size();
                if ( instr->set_name(std::to_string(seq_num) ))
                {
                    seq.insert( {instr, seq_num} );
                }
            }
        }
    }
}

std::string Function::print()
{
    set_instr_name();
    std::string func_ir;
    if ( this->is_declaration() ) 
    {
        func_ir += "declare ";
    }    
    else
    {
        func_ir += "define ";
    }
    
    func_ir += this->get_return_type()->print();
    func_ir += " ";
    func_ir += print_as_op(this, false);
    func_ir += "(";

    //print arg
    if ( this->is_declaration() ) 
    {
        for ( int i = 0 ; i < this->get_num_of_args() ; i++)
        {
            if(i)
                func_ir += ", ";
            func_ir += static_cast<FunctionType *>(this->get_type())->get_param_type(i)->print();
        }
    }
    else
    {
        for ( auto arg = this->arg_begin(); arg != arg_end() ; arg++ )
        {
            if( arg != this->arg_begin() )
            {
                func_ir += ", ";
            }
            func_ir += static_cast<Argument *>(*arg)->print();
        }
    }
    func_ir += ")";

    //print bb
    if( this->is_declaration() ) {
        func_ir += "\n";
    }
    else
    {
        func_ir += " {";
        func_ir += "\n";
        for ( auto bb : this->get_basic_blocks() )
        {
            func_ir += bb->print();
        }
        func_ir += "}";
    }
    
    return func_ir;
}

std::string Argument::print()
{
    std::string arg_ir;
    arg_ir += this->get_type()->print();
    arg_ir += " %";
    arg_ir += this->get_name();
    return arg_ir;
}