#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <fstream>

#include "rip.h"

int main(int argc, char* argv[])
{
	bool isError = false;

	if (argc == 1) {
		std::cout << "No arguments provided. Use --help for usage information." << std::endl;
		return 0;
	}

	if (strcmp(argv[1], "--help") == 0) {
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
				return -1;
			}

			if (filename.length() > 4 && filename.substr(filename.length() - 4) == ".rip") {
				filename.erase(filename.length() - 4);
			}

			std::streamsize fileSize = inputFile.tellg();
			inputFile.close();

			if (fileSize == 0) {
				std::cout << "Warning: the file " << filename << "is empty." << std::endl;
				return -1;
			}

			RIP rip;
			filename.append(".cpp");
			rip.compile(filename, isError);

			std::ifstream file(filename);
			if (file.peek() == std::ifstream::traits_type::eof()) {
				std::cout << "The file is empty." << std::endl;
				return 0;
			}
			else if (isError) {
				return 0;
			}
			else {
				file.close();
				std::string compile_command = "g++ " + filename + " -o " + filename.substr(0, filename.find_last_of('.')) + ".exe";
				int compile_result = std::system(compile_command.c_str());

				if (compile_result == 0) {
					std::cout << "Compilation successful." << std::endl;
					std::string del_cmd = "del " + filename;
					if (std::system(del_cmd.c_str()) != 0) {
						std::cout << "Error deleting the file" << std::endl;
					}
				}
				else {
					std::cout << "Error during compilation of " << filename << std::endl;
				}
			}
		}
		return 0;
	}

	std::cout << "Unknown argument: " << argv[1] << ". Use --help for usage information." << std::endl;
	return 0;
}
