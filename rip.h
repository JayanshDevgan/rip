#pragma once


class RIP {
public:
    void compile(const std::string& filename, bool& isError);

private:
    std::string trimTrailingSpaces(const std::string& str);
    void reportError(const std::string& message, int lineNumber, const std::string& line);
    std::string trim(const std::string& str);
    std::string convert(const std::string& line);
    bool isMultilineCommentStart(const std::string line);
    bool isMultilineCommentEnd(const std::string& line);
    bool isSingleLineComment(const std::string& line);
    void checkLineEnd(const std::string& line, int lineNumber, bool& isError, bool& insideMultilineComment, const std::string& nextLine);
};
