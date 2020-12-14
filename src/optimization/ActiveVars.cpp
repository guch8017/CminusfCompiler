#include "ActiveVars.hpp"

void ActiveVars::run()
{
    std::ofstream output_active_vars;
    output_active_vars.open("active_vars.json", std::ios::out);
    output_active_vars << "[";
    for (auto &func : this->m_->get_functions()) {
        if (func->get_basic_blocks().empty()) {
            continue;
        }
        else
        {
            func_ = func;  

            // 在此分析 func_ 的每个bb块的活跃变量，并存储在 live_in live_out 结构内

            output_active_vars << print();
            output_active_vars << ",";
        }
    }
    output_active_vars << "]";
    output_active_vars.close();
    return ;
}
    
std::string ActiveVars::print()
{
    std::string active_vars;
    func_->set_instr_name();
    active_vars +=  "{\n";
    active_vars +=  "\"function\": \"";
    active_vars +=  func_->get_name();
    active_vars +=  "\",\n";

    active_vars +=  "\"live_in\": {\n";
    for (auto &p : live_in) {
        active_vars +=  "  \"";
        active_vars +=  p.first->get_name();
        active_vars +=  "\": [" ;
        for (auto &v : p.second) {
            active_vars +=  "\"%";
            active_vars +=  v->get_name();
            active_vars +=  "\",";
        }
        active_vars.pop_back();
        active_vars += "]" ;
        active_vars += ",\n";
    }
    active_vars.pop_back();
    active_vars.pop_back();
    active_vars += "\n";
    active_vars +=  "    },\n";
    
    active_vars +=  "\"live_out\": {\n";
    for (auto &p : live_out) {
        active_vars +=  "  \"";
        active_vars +=  p.first->get_name();
        active_vars +=  "\": [" ;
        for (auto &v : p.second) {
            active_vars +=  "\"%";
            active_vars +=  v->get_name();
            active_vars +=  "\",";
        }
        active_vars.pop_back();
        active_vars += "]";
        active_vars += ",\n";
    }
    active_vars.pop_back();
    active_vars.pop_back();
    active_vars += "\n";
    active_vars += "    }\n";

    active_vars += "}\n";
    active_vars += "\n";
    return active_vars;
}