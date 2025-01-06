#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <fstream>

#include "rip.h"

int main(int argc, char* argv[])
{
	if (argc == 1) {
		std::cout << "No arguments provided. Use --help for usage information." << std::endl;
		return 0;
	}

	if (strcmp(argv[1], "-help") == 0) {
		std::cout << "Usage:" << std::endl;
		std::cout << "  --help             \t\t\tShow this help message." << std::endl;
		std::cout << "  --compile <file>   \t\t\tCompile the specified file." << std::endl;
		return 0;
	}

	if (strcmp(argv[1], "--compile") == 0) {
		if (argc < 3) {
			std::cout << "Error: No file name provided with --compile" << std::endl;
		}
		else {
			std::string filename = argv[2];

			std::ifstream inputFile(filename, std::ios::binary | std::ios::ate);
			if (!inputFile) {
				std::cerr << "Error: Could not open file " << filename << std::endl;
				return;
			}

			std::streamsize fileSize = inputFile.tellg();
			inputFile.close();

			if (fileSize == 0) {
				return;
			}

			RIP rip;
			rip.compiler(inputFile, filename);

			std::string compile_command = "g++ " + filename + " -o " + filename.substr(0, filename.find_last_of('.')) + ".exe";
			int compile_result = std::system(compile_command.c_str());

			if (compile_result == 0) {
				std::cout << "Compilation successful." << std::endl;

				if (std::remove(filename.c_str()) != 0) {
					std::cout << "Error deleting the file " << filename << std::endl;
				}
			}
			else {
				std::cout << "Error during compilation of " << filename << std::endl;
			}
		}
		return 0;
	}

	std::cout << "Unknown argument: " << argv[1] << ". Use --help for usage information." << std::endl;
	return 0;
}