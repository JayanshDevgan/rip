#include <iostream>
#include <fstream>
#include <string>
#include <stack>
#include <sstream>
#include <cctype>
#include <algorithm>
#include <vector>

#include "rip.h"

std::string RIP::trimTrailingSpaces(const std::string& str) {
    size_t end = str.find_last_not_of(" \t");
    return (end == std::string::npos) ? "" : str.substr(0, end + 1);
}

void RIP::compile(const std::string& filename, bool& isError) {
    size_t dotPosition = filename.find_last_of('.');
    std::string newFilename = (dotPosition != std::string::npos) ?
        filename.substr(0, dotPosition) + ".rip" : filename + ".rip";
    std::ifstream file(newFilename);
    if (!file) {
        std::cerr << "Error: Could not open file " << newFilename << std::endl;
        return;
    }
    std::ofstream outputFile(filename);
    if (!outputFile) {
        std::cerr << "Error: Could not create output file " << filename << std::endl;
        return;
    }
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    bool insideMultilineComment = false;
    for (size_t i = 0; i < lines.size(); i++) {
        std::string currentLine = lines[i];
        std::string trimmedCurrentLine = trim(currentLine);
        std::string nextLine = (i + 1 < lines.size()) ? lines[i + 1] : "";

        if (insideMultilineComment) {
            if (isMultilineCommentEnd(currentLine)) {
                insideMultilineComment = false;
            }
            continue;
        }
        if (isMultilineCommentStart(currentLine)) {
            insideMultilineComment = true;
            continue;
        }
        if (isSingleLineComment(currentLine) || trimmedCurrentLine.empty()) {
            continue;
        }

        if (trimmedCurrentLine == "{" || trimmedCurrentLine == "}") {
            outputFile << currentLine << std::endl;
            continue;
        }

        std::string converted = convert(currentLine);
        checkLineEnd(converted, i + 1, isError, insideMultilineComment, nextLine);
        outputFile << converted << std::endl;
    }
    outputFile.close();
}

void RIP::reportError(const std::string& message, int lineNumber, const std::string& line) {
    std::cerr << "Error: " << message << " at line " << lineNumber << ":\n\t" << line << std::endl;
}

std::string RIP::trim(const std::string& str) {
    const std::string whitespace = " \t\n\r\f\v";
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::string::npos) return "";

    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}

std::string RIP::convert(const std::string& line) {
    std::string trimmed = trim(line);
    std::string convertedLine = line;

    if (trimmed.find("@import") == 0) {
        size_t start = trimmed.find('"') + 1;
        size_t end = trimmed.find('"', start);
        if (start != std::string::npos && end != std::string::npos) {
            std::string filename = trimmed.substr(start, end - start);
            return (filename.find("stdio") != std::string::npos) ?
                "#include <iostream>" : "#include \"" + filename + "\"";
        }
    }

    if (trimmed.find("def ") == 0) {
        size_t nameStart = trimmed.find(' ') + 1;
        size_t nameEnd = trimmed.find('(', nameStart);
        if (nameEnd != std::string::npos) {
            std::string funcName = trimmed.substr(nameStart, nameEnd - nameStart);
            size_t paramsEnd = trimmed.find(')', nameEnd);
            std::string params = trimmed.substr(nameEnd + 1, paramsEnd - nameEnd - 1);
            size_t returnTypeStart = paramsEnd + 1;
            std::string returnType = trim(trimmed.substr(returnTypeStart));

            if (!returnType.empty() && returnType.back() == '{') {
                returnType.pop_back();
                returnType = trim(returnType);
            }
            size_t defPos = line.find("def ");
            if (defPos != std::string::npos) {
                convertedLine = line.substr(0, defPos) + returnType + " " + funcName + "(" + params + ")";
            }
        }
    }
    else if (trimmed.find("if (") == 0 ||
        trimmed == "else" ||
        trimmed.find("for (") == 0 ||
        trimmed.find("while (") == 0 ||
        trimmed == "do") {
        convertedLine = line;
    }
    else if (trimmed.find("print(") == 0) {
        size_t contentStart = trimmed.find('(') + 1;
        size_t contentEnd = trimmed.find(')', contentStart);
        if (contentEnd != std::string::npos) {
            std::string content = trimmed.substr(contentStart, contentEnd - contentStart);
            convertedLine = line.substr(0, line.find("print(")) + "std::cout << " + content + ";";
        }
    }
    else if (trimmed.find("println(") == 0) {
        size_t contentStart = trimmed.find('(') + 1;
        size_t contentEnd = trimmed.find(')', contentStart);
        if (contentEnd != std::string::npos) {
            std::string content = trimmed.substr(contentStart, contentEnd - contentStart);
            convertedLine = line.substr(0, line.find("println(")) + "std::cout << " + content + " << std::endl;";
        }
    }

    size_t pos = 0;
    while ((pos = convertedLine.find("boolean", pos)) != std::string::npos) {
        if ((pos == 0 || !std::isalpha(convertedLine[pos - 1])) &&
            (pos + 7 == convertedLine.length() || !std::isalpha(convertedLine[pos + 7]))) {
            convertedLine.replace(pos, 7, "bool");
            pos += 4;
        }
        else {
            pos += 7;
        }
    }

    pos = 0;
    while ((pos = convertedLine.find("string", pos)) != std::string::npos) {
        if ((pos == 0 || !std::isalpha(convertedLine[pos - 1])) &&
            (pos + 6 == convertedLine.length() || !std::isalpha(convertedLine[pos + 6]))) {
            convertedLine.replace(pos, 6, "std::string");
            pos += 11;
        }
        else {
            pos += 6;
        }
    }

    return convertedLine;
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

void RIP::checkLineEnd(const std::string& line, int lineNumber, bool& isError, bool& insideMultilineComment, const std::string& nextLine) {
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

    std::string trimmedLine = trim(line);

    if (trimmedLine.empty() || trimmedLine.front() == '/' || trimmedLine.find("#include") == 0) {
        return;
    }

    // Lines that are block starters (function definitions, if, else, for, while, do)
    // or standalone braces should not require a semicolon.
    if ((trimmedLine.find("void ") == 0 && trimmedLine.find("(") != std::string::npos) ||
        (trimmedLine.find("int ") == 0 && trimmedLine.find("main") != std::string::npos) ||
        trimmedLine.find("if (") == 0 ||
        trimmedLine == "else" ||
        trimmedLine.find("for (") == 0 ||
        trimmedLine.find("while (") == 0 ||
        trimmedLine == "do" ||
        trimmedLine == "do {" ||
        trimmedLine == "{" ||
        trimmedLine == "}")
    {
            return;
    }

    if (trimmedLine.find("while (") != std::string::npos && trimmedLine.back() == ';') {
        return;
    }

    if (trimmedLine.back() != ';') {
        reportError("Missing ';' at end of statement", lineNumber, line);
        isError = true;
    }
}
