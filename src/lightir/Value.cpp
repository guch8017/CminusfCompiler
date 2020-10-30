#include "Value.h"
#include "Type.h"

Value::Value(Type *ty, const std::string &name )
  : type_(ty), name_(name)
{

}

void Value::add_use(Value *val, unsigned arg_no )
{
    use_list_.push_back(Use(val, arg_no));
}

std::string Value::get_name() const
{
    return name_;
}