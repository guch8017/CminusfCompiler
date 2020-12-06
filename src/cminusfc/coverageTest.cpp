#include "cminusf_builder.hpp"
#include <iostream>
#include <fstream>
#include <memory>

using namespace std::literals::string_literals;
using namespace std;

const string dirPrefix = "../tests/lab4/my_testcases/";
const string fileNames[] = {"arrayOutOfRangeDynamic.cminus", "arrayOutOfRangeStatic.cminus", "divisionByZero.cminus",
                            "globalVariable.cminus", "quickSort.cminus", "typeChange.cminus", "otherCase.cminus"};

int main(int argc, char **argv) {
    for(std::string input: fileNames){
        std::string target_path;
        std::string input_path = dirPrefix + input;
        bool emit = false;
        if (target_path.empty()) {
            auto pos = input_path.rfind('.');
            if (pos == std::string::npos) {
                std::cerr << argv[0] << ": input file " << input_path << " has unknown filetype!" << std::endl;
                return -1;
            } else {
                if (input_path.substr(pos) != ".cminus") {
                    std::cerr << argv[0] << ": input file " << input_path << " has unknown filetype!" << std::endl;
                    return -1;
                }
                if (emit) {
                    target_path = input_path.substr(0, pos);
                } else {
                    target_path = input_path.substr(0, pos);
                }
            }
        }
        try
        {    
        auto s = parse(input_path.c_str());
        auto a = AST(s);
        CminusfBuilder builder;
        a.run_visitor(builder);
        auto m = builder.getModule();
        auto IR = m->print();

        std::ofstream output_stream;
        auto output_file = target_path+".ll";
        output_stream.open(output_file, std::ios::out);
        output_stream << "; ModuleID = 'cminus'\n";
        output_stream << "source_filename = \""+ input_path +"\"\n\n";
        output_stream << IR;
        output_stream.close();
        }
        catch(const char* e)
        {
            std::cerr << e << std::endl;
        }
        catch(const std::string e)
        {
            std::cerr << e << std::endl;
        }
        if (!emit) {
            auto command_string = "clang -w "s + target_path + ".ll -o " + target_path + " -L/usr/local/lib/ -lcminus_io";
            std::system(command_string.c_str());
            command_string = "rm "s + target_path + ".ll";
            std::system(command_string.c_str());
        }
    }
    return 0;
}
