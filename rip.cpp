#include <iostream>
#include <fstream>
#include <string>

#include "rip.h"

void RIP::compile(const std::string& filename) {
    std::cout << "Compiling file: " << filename << std::endl;

    size_t dotPosition = filename.find_last_of('.');

    std::string newFilename;
    if (dotPosition != std::string::npos) {
        newFilename = filename.substr(0, dotPosition) + ".rip";
    }

    std::ifstream  file(newFilename);
    
    if (!file) {
        std::cerr << "Error: Invalid file pointer." << std::endl;
        return;
    }

    std::ofstream outputFile(filename);

    if (!outputFile) {
        std::cerr << "Error: Could not create " << filename << std::endl;
        return;
    }

    std::string line;
    int lineNumber = 1;

    if (file.eof()) {
        std::cerr << "Error: the file is empty." << std::endl;
    }
    else {
        while (std::getline(file, line)) {
            if (file.fail()) {
                std::cerr << "Error: Failed to read from file " << filename << std::endl;
                break;
            }

            outputFile << line << std::endl;
            std::cout << line << std::endl;
            lineNumber++;
        }
    }

    outputFile.close();

    if (outputFile.fail()) {
        std::cerr << "Error: Failed to write to the file " << filename << std::endl;
    }
    else {
        std::cout << "File " << filename << " successfully written." << std::endl;
    }
}

void RIP::reportError(const std::string& message, int lineNumber, const std::string& line) {
    std::cerr << "Error: " << message << " at line " << lineNumber << ":\n\t" << line << std::endl;
}
