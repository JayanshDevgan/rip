#ifndef RIP_H
#define RIP_H

#include <string>
#include <vector>
#include <set>

class RIP {
public:
    void compile(const std::string& filename, bool& isError);

private:
    std::set<std::string> normalDataTypes;
    std::set<std::string> arrayDataTypes;

    void loadDataTypes(const std::string& archFilename, bool& isError);
    bool isNormalDataType(const std::string& type);
    bool isArrayDataType(const std::string& type);

    std::vector<std::string> splitParameters(const std::string& paramsStr);

    std::string trimTrailingSpaces(const std::string& str);
    std::string generate_unique_id();
    void reportError(const std::string& message, int lineNumber, const std::string& line);
    std::string trim(const std::string& str);
    std::string convert(const std::string& line, int lineNumber, bool& isError);
    bool isMultilineCommentStart(const std::string line);
    bool isMultilineCommentEnd(const std::string& line);
    bool isSingleLineComment(const std::string& line);
    void checkLineEnd(const std::string& line, int lineNumber, bool& isError, bool& insideMultilineComment, const std::string& nextLine);
};

#endif // RIP_H
