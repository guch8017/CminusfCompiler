#include "cminusf_builder.hpp"
#include <iostream>
#include <fstream>
#include <memory>

using namespace std::literals::string_literals;

void print_help(std::string exe_name) {
    std::cout << "Usage: " << exe_name <<
        " [ -h | --help ] [ -o <target-file> ] [ -emit-llvm ] <input-file>" << std::endl;
}

int main(int argc, char **argv) {
    std::string target_path;
    std::string input_path;
    bool emit = false;
    for (int i = 1;i < argc;++i) {
        if (argv[i] == "-h"s || argv[i] == "--help"s) {
            print_help(argv[0]);
            return 0;
        } else if (argv[i] == "-o"s) {
            if (target_path.empty() && i + 1 < argc) {
                target_path = argv[i + 1];
                i += 1;
            } else {
                print_help(argv[0]);
                return 0;
            }
        } else if (argv[i] == "-emit-llvm"s) {
            emit = true;
        } else {
            if (input_path.empty()) {
                input_path = argv[i];
            } else {
                print_help(argv[0]);
                return 0;
            }
        }
    }
    if (input_path.empty()) {
        print_help(argv[0]);
        return 0;
    }

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
    if (!emit) {
        auto command_string = "clang -w "s + target_path + ".ll -o " + target_path + " -L. -lcminus_io";
        std::system(command_string.c_str());
        command_string = "rm "s + target_path + ".ll";
        std::system(command_string.c_str());
    }

    return 0;
}
