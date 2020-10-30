#ifndef SYSYC_BASICBLOCK_H
#define SYSYC_BASICBLOCK_H

#include "Value.h"
#include "Instruction.h"
#include "Module.h"
#include "Function.h"

#include <list>
#include <string>

class Function;
class Instruction;
class Module;

class BasicBlock : public Value
{
public:
    static BasicBlock *create(Module *m, const std::string &name ,
                            Function *parent ) {
        return new BasicBlock(m, name, parent);
    }

    // return parent, or null if none.
    Function *get_parent() { return parent_; }
    
    Module *get_module();

    /// Returns the terminator instruction if the block is well formed or null
    /// if the block is not well formed.
    const Instruction *get_terminator() const;
    Instruction *get_terminator() {
        return const_cast<Instruction *>(
            static_cast<const BasicBlock *>(this)->get_terminator());
    }
    
    void add_instruction(Instruction *instr);

    bool empty() { return instr_list_.empty(); }
    // void eraseFromParent() { parent_->Remove(this); }

    int get_num_of_instr() { return instr_list_.size(); }
    std::list<Instruction *> get_instructions() { return instr_list_; }
    
    void erase_from_parent();// todo : 测试是否正确

    virtual std::string print() override;

private:
    explicit BasicBlock(Module *m, const std::string &name ,
                        Function *parent );

    std::list<Instruction *> instr_list_;
    Function *parent_;
};

#endif // SYSYC_BASICBLOCK_H