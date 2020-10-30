#include "User.h"
#include <cassert>

User::User(Type *ty, const std::string &name , unsigned num_ops )
    : Value(ty, name), num_ops_(num_ops)
{
    // if (num_ops_ > 0)
    //   operands_.reset(new std::list<Value *>());
    operands_.resize(num_ops_, nullptr);
}

std::vector<Value *>& User::get_operands()
{
    return operands_;
}

Value *User::get_operand(unsigned i) const
{
    return operands_[i];
}

void User::set_operand(unsigned i, Value *v)
{
    assert(i < num_ops_ && "set_operand out of index");
    assert(operands_[i] == nullptr && "ith operand is not null");
    operands_[i] = v;  
    v->add_use(this, i);
}

unsigned User::get_num_operand() const
{
    return num_ops_;
}
