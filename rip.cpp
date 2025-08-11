#include <iostream>
#include <fstream>
#include <string>
#include <stack>
#include <sstream>
#include <cctype>
#include <regex>
#include <algorithm>
#include <vector>
#include <chrono>
#include <random>
#include <set>

#include "rip.h"

static std::vector<int> make_range(int start, int end) {
    std::vector<int> result;
    for (int i = start; i <= end; ++i) {
        result.push_back(i);
    }
    return result;
}

std::vector<std::string> RIP::splitParameters(const std::string& paramsStr) {
    std::vector<std::string> paramList;
    std::stringstream ss(paramsStr);
    std::string segment;
    while (std::getline(ss, segment, ',')) {
        paramList.push_back(trim(segment));
    }
    return paramList;
}

std::string RIP::trimTrailingSpaces(const std::string& str) {
    size_t end = str.find_last_not_of(" \t");
    return (end == std::string::npos) ? "" : str.substr(0, end + 1);
}

std::string RIP::generate_unique_id() {
    using namespace std::chrono;
    static std::mt19937 gen(system_clock::now().time_since_epoch().count());
    static std::uniform_int_distribution<int> dist(1000, 9999);
    return "_vec_" + std::to_string(dist(gen)) + "_" + std::to_string(time(nullptr));
}

void RIP::loadDataTypes(const std::string& archFilename, bool& isError) {
    std::ifstream file(archFilename);
    if (!file) {
        std::cerr << "Error: Could not open architecture file " << archFilename << std::endl;
        isError = true;
        return;
    }

    std::string line;
    enum { NONE, NORMAL, ARRAY } currentBlock = NONE;

    while (std::getline(file, line)) {
        std::string trimmedLine = trim(line);

        if (trimmedLine == "#normal_datatypes {") {
            currentBlock = NORMAL;
            continue;
        }
        else if (trimmedLine == "#array_datatypes {") {
            currentBlock = ARRAY;
            continue;
        }
        else if (trimmedLine == "}") {
            currentBlock = NONE;
            continue;
        }

        if (currentBlock == NORMAL && !trimmedLine.empty()) {
            normalDataTypes.insert(trimmedLine);
        }
        else if (currentBlock == ARRAY && !trimmedLine.empty()) {
            arrayDataTypes.insert(trimmedLine);
        }
    }
    file.close();

    if (normalDataTypes.empty() && arrayDataTypes.empty()) {
        std::cerr << "Warning: No data types loaded from " << archFilename << ". Please check the file format and content." << std::endl;
    }
}

bool RIP::isNormalDataType(const std::string& type) {
    if (type == "void") return true;
    return normalDataTypes.count(type);
}

bool RIP::isArrayDataType(const std::string& type) {
    return arrayDataTypes.count(type);
}

void RIP::compile(const std::string& filename, bool& isError) {
    isError = false;

    std::string archFilename = ".riparch";
    loadDataTypes(archFilename, isError);
    if (isError) {
        std::cerr << "Compilation aborted due to errors in architecture file: " << archFilename << std::endl;
        return;
    }

    size_t dotPosition = filename.find_last_of('.');
    std::string newFilename = (dotPosition != std::string::npos) ?
        filename.substr(0, dotPosition) + ".rip" : filename + ".rip";
    std::ifstream file(newFilename);
    if (!file) {
        std::cerr << "Error: Could not open input file " << newFilename << std::endl;
        isError = true;
        return;
    }
    std::ofstream outputFile(filename);
    if (!outputFile) {
        std::cerr << "Error: Could not create output file " << filename << std::endl;
        isError = true;
        file.close();
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

        std::string converted = convert(currentLine, i + 1, isError);
        if (isError) {
            std::cerr << "Compilation aborted due to conversion errors." << std::endl;
            outputFile.close();
            file.close();
            return;
        }

        checkLineEnd(converted, i + 1, isError, insideMultilineComment, nextLine);
        if (isError) {
            std::cerr << "Compilation aborted due to line ending errors." << std::endl;
            outputFile.close();
            file.close();
            return;
        }
        outputFile << converted << std::endl;
    }

    outputFile.close();
    file.close();
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

std::string RIP::convert(const std::string& line, int lineNumber, bool& isError) {
    std::string trimmed = trim(line);
    std::string convertedLine = line;

    if (trimmed.find("@import") == 0) {
        size_t start = trimmed.find('"') + 1;
        size_t end = trimmed.find('"', start);
        if (start != std::string::npos && end != std::string::npos) {
            std::string filename = trimmed.substr(start, end - start);
            return (filename.find("stdio") != std::string::npos) ?
                "#include<iostream>\n#include<vector>\n#include<string>\n" : "#include \"" + filename + "\"";
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

            std::string lookupReturnType = returnType;
            if (lookupReturnType == "string") {
                lookupReturnType = "std::string";
            }
            if (!isNormalDataType(lookupReturnType)) {
                reportError("Invalid return type '" + returnType + "' for function '" + funcName + "'", lineNumber, line);
                isError = true;
                return "";
            }

            std::vector<std::string> paramSegments = splitParameters(params);
            std::string convertedParams = "";
            for (size_t i = 0; i < paramSegments.size(); ++i) {
                const std::string& paramSegment = paramSegments[i];
                if (paramSegment.empty()) continue;

                size_t spacePos = paramSegment.find(' ');
                if (spacePos != std::string::npos) {
                    std::string paramType = trim(paramSegment.substr(0, spacePos));
                    std::string paramName = trim(paramSegment.substr(spacePos + 1));

                    std::string lookupParamType = paramType;
                    if (lookupParamType == "string") {
                        lookupParamType = "std::string";
                    }

                    if (!isNormalDataType(lookupParamType)) {
                        reportError("Invalid parameter type '" + paramType + "' in function '" + funcName + "'", lineNumber, line);
                        isError = true;
                        return "";
                    }
                    convertedParams += (paramType == "string" ? "std::string" : paramType) + " " + paramName;
                }
                else {
                    reportError("Invalid parameter format '" + paramSegment + "' in function '" + funcName + "'. Expected 'type name'.", lineNumber, line);
                    isError = true;
                    return "";
                }
                if (i < paramSegments.size() - 1) {
                    convertedParams += ", ";
                }
            }

            size_t defPos = line.find("def ");
            if (defPos != std::string::npos) {
                convertedLine = line.substr(0, defPos) + (returnType == "string" ? "std::string" : returnType) + " " + funcName + "(" + convertedParams + ")";
            }
        }
    }

    // Range Expression
    std::regex array_decl_with_range_regex(R"(^\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*\[\]\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*=\s*\[(.*?)\]\s*;?\s*$)");
    std::smatch array_matches;
    if (std::regex_match(trimmed, array_matches, array_decl_with_range_regex)) {
        std::string type = array_matches[1].str();
        std::string var_name = array_matches[2].str();
        std::string range_expr_content = array_matches[3].str();
        size_t dots_pos = range_expr_content.find("..");

        if (dots_pos != std::string::npos) {
            std::string start_str = trim(range_expr_content.substr(0, dots_pos));
            std::string end_str = trim(range_expr_content.substr(dots_pos + 2));

            std::string id = generate_unique_id();
            std::string cpp_type = (type == "string" ? "std::string" : type);
            std::string temp_vec_creation = "([&]() { std::vector<" + cpp_type + "> v" + id + ";";
            temp_vec_creation += cpp_type + " start_val = " + start_str + "; ";
            temp_vec_creation += cpp_type + " end_val = " + end_str + "; ";

            temp_vec_creation += "if (start_val <= end_val) { ";
            temp_vec_creation += "for(" + cpp_type + " i = start_val; i <= end_val; ++i) { v" + id + ".push_back(i); } ";
            temp_vec_creation += "} else { ";
            temp_vec_creation += "for(" + cpp_type + " i = start_val; i >= end_val; --i) { v" + id + ".push_back(i); } ";
            temp_vec_creation += "} return v" + id + "; }())";

            convertedLine = line.substr(0, line.find(trimmed)) + "std::vector<" + cpp_type + "> " + var_name + " = " + temp_vec_creation + ";";
            return convertedLine;
        }
    }
    
    std::regex array_decl_regex(R"(^\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*\[\]\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*(.*)$)");
    std::smatch matches;
    if (std::regex_match(trimmed, matches, array_decl_regex)) {
        std::string type = matches[1].str();
        std::string var_name = matches[2].str();
        std::string remainder = matches[3].str();

        std::string lookupType = type;
        if (lookupType == "string") {
            lookupType = "std::string";
        }

        if (!isArrayDataType(lookupType)) {
            reportError("Invalid array data type '" + type + "'. Type not found in array_datatypes.", lineNumber, line);
            isError = true;
            return "";
        }

        convertedLine = "std::vector<" + (type == "string" ? "std::string" : type) + "> " + var_name + remainder;
        return convertedLine;
    }


    // Handle range expressions (e.g., "[1..5]" or "[1..(6+i)]")
    size_t original_bracket_open_pos = line.find("[");
    size_t original_bracket_close_pos = line.find("]", original_bracket_open_pos);

    if (original_bracket_open_pos != std::string::npos &&
        original_bracket_close_pos != std::string::npos &&
        line.substr(original_bracket_open_pos, original_bracket_close_pos - original_bracket_open_pos + 1).find("..") != std::string::npos) {

        std::string range_expr_content = line.substr(original_bracket_open_pos + 1, original_bracket_close_pos - original_bracket_open_pos - 1);
        size_t dots_pos = range_expr_content.find("..");

        if (dots_pos != std::string::npos) {
            std::string start_str = trim(range_expr_content.substr(0, dots_pos));
            std::string end_str = trim(range_expr_content.substr(dots_pos + 2));

            auto isIntegerLiteral = [](const std::string& s) {
                return !s.empty() && std::all_of(s.begin(), s.end(), [](unsigned char c) {
                    return std::isdigit(c) || c == '-' || c == '+';
                });
            };

            std::string type = "int";
            if (start_str.find('.') != std::string::npos || end_str.find('.') != std::string::npos) {
                type = "double";
            }

            std::string lookupRangeType = type;
            if (lookupRangeType == "string") lookupRangeType = "std::string";
            if (!isNormalDataType(lookupRangeType)) {
                reportError("Invalid type inferred for range expression. Type '" + type + "' not found in normal_datatypes.", lineNumber, line);
                isError = true;
                return "";
            }

            if (isIntegerLiteral(start_str) && isIntegerLiteral(end_str)) {
                int start_val = std::stoi(start_str);
                int end_val = std::stoi(end_str);

                std::ostringstream oss;
                oss << "{";
                if (start_val <= end_val) {
                    for (int x = start_val; x <= end_val; ++x) {
                        oss << x;
                        if (x != end_val) oss << ", ";
                    }
                }
                else {
                    for (int x = start_val; x >= end_val; --x) {
                        oss << x;
                        if (x != end_val) oss << ", ";
                    }
                }
                oss << "}";

                convertedLine = line.substr(0, original_bracket_open_pos) +
                    oss.str() +
                    line.substr(original_bracket_close_pos + 1);
                return convertedLine;
            }

            // **Runtime expansion (existing lambda logic)**
            std::string id = generate_unique_id();
            std::string range_vec_lambda = "[&]() { std::vector<" + type + "> v" + id + ";";
            range_vec_lambda += type + " start_val = " + start_str + "; ";
            range_vec_lambda += type + " end_val = " + end_str + "; ";
            range_vec_lambda += "if (start_val <= end_val) { ";
            range_vec_lambda += "for(" + type + " i = start_val; i <= end_val; ++i) { v" + id + ".push_back(i); } ";
            range_vec_lambda += "} else { ";
            range_vec_lambda += "for(" + type + " i = start_val; i >= end_val; --i) { v" + id + ".push_back(i); } ";
            range_vec_lambda += "} return v" + id + "; }()";

            convertedLine = line.substr(0, original_bracket_open_pos) +
                range_vec_lambda +
                line.substr(original_bracket_close_pos + 1);
            return convertedLine;
        }
    }


    // Handle range-based for loops (e.g., "for (int i : [1..10]) {")
    // This block converts RIP's range-based for to a traditional C++ for loop over a generated vector.
    else if (trimmed.find("for (") == 0 && trimmed.find(":[") != std::string::npos) {
        size_t parenStart = trimmed.find('(');
        size_t colonPos = trimmed.find(':', parenStart);
        size_t bracketOpen = trimmed.find('[', colonPos);
        size_t bracketClose = trimmed.find(']', bracketOpen);

        if (parenStart != std::string::npos && colonPos != std::string::npos &&
            bracketOpen != std::string::npos && bracketClose != std::string::npos) {

            std::string loop_var_decl = trimmed.substr(parenStart + 1, colonPos - (parenStart + 1));
            std::string rangeStr = trimmed.substr(bracketOpen + 1, bracketClose - (bracketOpen + 1));

            std::string varType;
            std::string varName;
            size_t lastSpaceInDecl = loop_var_decl.find_last_of(' ');
            if (lastSpaceInDecl != std::string::npos) {
                varType = trim(loop_var_decl.substr(0, lastSpaceInDecl));
                varName = trim(loop_var_decl.substr(lastSpaceInDecl + 1));
            }
            else {
                reportError("Invalid loop variable declaration in for loop: '" + loop_var_decl + "'. Expected 'type name'.", lineNumber, line);
                isError = true;
                return "";
            }

            size_t dotsPos = rangeStr.find("..");
            if (dotsPos != std::string::npos) {
                std::string start_expr = trim(rangeStr.substr(0, dotsPos));
                std::string end_expr = trim(rangeStr.substr(dotsPos + 2));

                // Type validation for loop variable
                std::string lookupVarType = varType;
                if (lookupVarType == "string") {
                    lookupVarType = "std::string";
                }
                if (!isNormalDataType(lookupVarType)) {
                    reportError("Invalid loop variable type '" + varType + "' in for loop", lineNumber, line);
                    isError = true;
                    return "";
                }

                // Generate a temporary vector using a lambda for the range
                std::string id = generate_unique_id();
                std::string temp_vec_creation = "([&]() { std::vector<" + (varType == "string" ? "std::string" : varType) + "> v" + id + ";";
                temp_vec_creation += (varType == "string" ? "std::string" : varType) + " start_val = " + start_expr + "; ";
                temp_vec_creation += (varType == "string" ? "std::string" : varType) + " end_val = " + end_expr + "; ";

                // Handle ascending and descending ranges for the loop
                temp_vec_creation += "if (start_val <= end_val) { for(" + (varType == "string" ? "std::string" : varType) + " i = start_val; i <= end_val; ++i) { v" + id + ".push_back(i); } } else { for(" + (varType == "string" ? "std::string" : varType) + " i = start_val; i >= end_val; --i) { v" + id + ".push_back(i); } }";
                temp_vec_creation += " return v" + id + "; }())"; // Immediate invocation

                // Generate C++ range-based for loop over the temporary vector
                std::string prefix = line.substr(0, line.find("for")); // Preserve leading indentation
                convertedLine = prefix + "for (" + (varType == "string" ? "std::string" : varType) + " " + varName + " : " + temp_vec_creation + ") {";
                return convertedLine;
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
            if (content != "")
                convertedLine = line.substr(0, line.find("println(")) + "std::cout << " + content + " << std::endl;";
            else convertedLine = line.substr(0, line.find("println(")) + "std::cout << std::endl;";
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

    // Lines that inherently do not require a semicolon:
    // 1. Empty lines
    if (trimmedLine.empty()) {
        return;
    }

    // 2. Comments (single-line or multi-line parts)
    if (trimmedLine.find("//") == 0 || trimmedLine.find("/*") == 0 || trimmedLine.find("*/") != std::string::npos) {
        return;
    }

    // 3. Preprocessor directives
    if (trimmedLine.find("#") == 0) {
        return;
    }

    // 4. Standalone braces
    if (trimmedLine == "{" || trimmedLine == "}") {
        return;
    }

    if ((trimmedLine.find("if (") == 0 ||
        trimmedLine.find("for (") == 0 ||
        trimmedLine.find("while (") == 0 ||
        trimmedLine == "else" ||
        trimmedLine == "do" ||
        trimmedLine == "do {") &&
        trimmedLine.back() != ';')
    {
        return;
    }

    std::regex func_def_regex(R"(^\s*([a-zA-Z_][a-zA-Z0-9_:]*)\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*\([^\)]*\)\s*$)");
    if (std::regex_match(trimmedLine, func_def_regex)) {
        return;
    }

    // Special case for `while (condition);` (valid C++ single-line loop)
    if (trimmedLine.find("while (") != std::string::npos && trimmedLine.back() == ';') {
        return;
    }

    // If none of the above conditions apply, the line *should* end with a semicolon.
    if (trimmedLine.back() != ';') {
        reportError("Missing ';' at end of statement", lineNumber, line);
        isError = true;
    }
}
