#pragma once

#include <string>
#include <vector>

static constexpr bool isWhitespace(char c) {
    return (c == ' ') || (c == '\t') || (c == '\n') || (c == '\r');
}

extern std::vector<std::string> split(const std::string& str, char delim);
extern std::string trim(const std::string& str);
extern bool isStringInCstrList(const std::string& key, const char* list[]);
