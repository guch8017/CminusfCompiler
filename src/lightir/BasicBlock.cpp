#include "Module.h"
#include "BasicBlock.h"
#include "Function.h"
#include <cassert>

BasicBlock::BasicBlock(Module *m, const std::string &name = "",
                      Function *parent = nullptr)
    : Value(Type::get_label_type(m), name), parent_(parent)
{
    assert(parent && "currently parent should not be nullptr");
    parent_->add_basic_block(this);
}

Module *BasicBlock::get_module()
{
    return get_parent()->get_parent();
}

void BasicBlock::add_instruction(Instruction *instr)
{
    // auto seq = atoi(this->getName().c_str());
    // seq += instr_list_.size();
    // instr->setName(std::to_string(seq));
    instr_list_.push_back(instr);
}

const Instruction *BasicBlock::get_terminator() const
{
    if (instr_list_.empty()){
        return nullptr;
    }
    switch (instr_list_.back()->get_instr_type())
    {
    case Instruction::Ret:
        return instr_list_.back();
        break;
    
    case Instruction::Br:
        return instr_list_.back();
        break;

    default:
        return nullptr;
        break;
    }
}

void BasicBlock::erase_from_parent()
{
    this->get_parent()->remove(this);
}

std::string BasicBlock::print()
{
    std::string bb_ir;
    bb_ir += this->get_name();
    bb_ir += ":";
    if ( !this->get_parent() )
    {
        bb_ir += "\n";
        bb_ir += "; Error: Block without parent!";
    }
    bb_ir += "\n";
    for ( auto instr : this->get_instructions() )
    {
        bb_ir += "  ";
        bb_ir += instr->print();
        bb_ir += "\n";
    }

    return bb_ir;
}