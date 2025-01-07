#pragma once


class RIP {
public:
    void compile(std::ifstream& file, const std::string& filename);

private:
    void reportError(const std::string& message, int lineNumber, const std::string& line);
};
