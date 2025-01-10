#include <iostream>
#include <fstream>
#include <string>
#include <stack>
#include <sstream>
#include <cctype>
#include <algorithm>

#include "rip.h"

std::string RIP::trimTrailingSpaces(const std::string& str) {
    size_t end = str.find_last_not_of(" \t");
    return (end == std::string::npos) ? "" : str.substr(0, end + 1);
}

void RIP::compile(const std::string& filename, bool& isError) {
    size_t dotPosition = filename.find_last_of('.');

    std::string newFilename;
    if (dotPosition != std::string::npos) {
        newFilename = filename.substr(0, dotPosition) + ".rip";
    }

    std::ifstream file(newFilename);

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
    bool insideMultilineComment = false;

    if (file.eof()) {
        std::cerr << "Error: the file is empty." << std::endl;
    }
    else {
        while (std::getline(file, line)) {
            if (file.fail()) {
                std::cerr << "Error: Failed to read from file " << filename << std::endl;
                break;
            }

            // Trim trailing white spaces
            line = trimTrailingSpaces(line);

            checkLineEnd(line, lineNumber, isError, insideMultilineComment);

            outputFile << convert(line) << std::endl;
            lineNumber++;
        }
    }

    if (outputFile.fail()) {
        std::cerr << "Error: Failed to write to the file " << filename << std::endl;
    }

    outputFile.close();
}

void RIP::reportError(const std::string& message, int lineNumber, const std::string& line) {
    std::cerr << "Error: " << message << " at line " << lineNumber << ":\n\t" << line << std::endl;
}

std::string RIP::convert(const std::string& line) {
    if (line.find("@import") == 0) {
        size_t start = line.find('"') + 1;
        size_t end = line.find('"', start);

        if (start != std::string::npos && end != std::string::npos) {
            std::string filename = line.substr(start, end - start);
            if (filename.find("stdio") != std::string::npos) {
                return "#include <iostream>";
            }
            else {
                return "#include \"" + filename + "\"";
            }
        }
    }

    if (line.find("def") != std::string::npos) {
        size_t nameStart = line.find("def") + 4;
        size_t nameEnd = line.find('(');
        std::string functionName = line.substr(nameStart, nameEnd - nameStart);

        size_t paramStart = nameEnd + 1;
        size_t paramEnd = line.find(')', paramStart);
        std::string parameters = line.substr(paramStart, paramEnd - paramStart);

        size_t returnTypeStart = paramEnd + 1;
        size_t returnTypeEnd = line.find('{', returnTypeStart);
        std::string returnType = line.substr(returnTypeStart, returnTypeEnd - returnTypeStart);

        return returnType + functionName + "(" + parameters + ") {";
    }

    if (line.find("print(") != std::string::npos) {
        std::string content = line.substr(line.find('(') + 1);
        content = content.substr(0, content.find(')'));
        return "std::cout << " + content + ";";
    }

    if (line.find("println(") != std::string::npos) {
        std::string content = line.substr(line.find('(') + 1);
        content = content.substr(0, content.find(')'));
        return "std::cout << " + content + " << std::endl;";
    }

    return line;
}

bool RIP::isMultilineCommentStart(const std::string line) {
    return line.find("/*") != std::string::npos;
}

bool RIP::isMultilineCommentEnd(const std::string& line) {
    return line.find("*/") != std::string::npos;
}

bool RIP::isSingleLineComment(const std::string& line) {
    return line.find("//") != std::string::npos;
}

void RIP::checkLineEnd(const std::string& line, int lineNumber, bool& isError, bool& insideMultilineComment) {
    if (insideMultilineComment) {
        if (isMultilineCommentEnd(line)) {
            insideMultilineComment = false;
        }
        return;
    }

    if (isMultilineCommentStart(line)) {
        insideMultilineComment = true;
        return;
    }

    if (isSingleLineComment(line)) {
        return;
    }

    if (line.empty() || line.front() == '/' || std::all_of(line.begin(), line.end(), ::isspace)) {
        return;
    }

    if (line.back() == ';' || line.back() == '{' || line.back() == '}') {
        return;
    }

    if (line.find("def ") == 0) {
        if (line.back() != '{') {
            std::cout << "Error: Missing '{' after function definition in line " << lineNumber << "\n\t" << line << std::endl;
            isError = true;
        }
    }
    else if (line.find(')') != std::string::npos) {
        if (line.back() != ';' && line.back() != '{') {
            std::cout << "Error: Missing ';' or '{' for function call in line " << lineNumber << "\n\t" << line << std::endl;
            isError = true;
        }
    }
    else if (line.find('(') != std::string::npos) {
        if (line.back() != '{') {
            std::cout << "Error: Missing '{' after function definition in line " << lineNumber << "\n\t" << line << std::endl;
            isError = true;
        }
    }
    else if (line.back() == '}') {
        if (line.size() > 1 && line[line.size() - 2] != ';') {
            std::cout << "Error: Missing ';' before closing '}' in line " << lineNumber << "\n\t" << line << std::endl;
            isError = true;
        }
    }
    else if (line.back() != '{') {
        std::cout << "Error: Missing '{' at the end of the line " << lineNumber << "\n\t" << line << std::endl;
        isError = true;
    }
}