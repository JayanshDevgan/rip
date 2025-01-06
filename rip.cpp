#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <regex>

#include "rip.h"

void RIP::compiler(std::ifstream& file, const std::string filename)
{
    std::cout << "Compiling file: " << filename << std::endl;

    if (!file) {
        std::cerr << "Error: Invalid file pointer." << std::endl;
        return;
    }

    std::string cpp_filename = filename + ".cpp";
    std::ofstream outputFile(cpp_filename);

    if (!outputFile) {
        std::cerr << "Error: Could not create " << cpp_filename << std::endl;
        return;
    }

    outputFile << "#include <iostream>" << std::endl;
    outputFile << "int main() {" << std::endl;
    
    // TODO: Compiling the code here

    outputFile << "return 0;}" << std::endl;
    outputFile.close();
}
