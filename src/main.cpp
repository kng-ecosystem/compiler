#include "compiler.h"

#include <fstream>
#include <sstream>
#include <cxxopts.hpp>

int main(int argc, char** argv) {

    cxxopts::Options options("kng compiler", "The kng compiler");

    options.add_options()
        ("f,file", "File to compile", cxxopts::value<std::string>())
        ("d,debug", "Enable debugging", cxxopts::value<bool>()->default_value("false"))
        ("h,help", "Print usage")
        ;

    auto result = options.parse(argc, argv);


    if (result.count("help"))
    {
        std::cout << options.help() << std::endl;
        exit(0);
    }
    bool debug = result["debug"].as<bool>();
    if (debug)
        log("kng compiler debug enabled!");

    std::string file;
    if (result.count("file")) {
        file = result["file"].as<std::string>();
        log("file: {}", file);
        
        // open a test file
        std::ifstream f;
        f.open(file);

        std::stringstream buffer;
        buffer << f.rdbuf();

        std::string contents = buffer.str();
       
        log("file contents: {}", contents);

        Compiler compiler;

        compiler.compile({ file, contents });
    }



    return 0;
}