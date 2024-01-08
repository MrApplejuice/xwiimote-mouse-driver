#include "stringtools.hpp"

std::vector<std::string> split(const std::string& str, char delim) {
    std::vector<std::string> result;
    std::string current = "";
    for (char c : str) {
        if (c == delim) {
            result.push_back(current);
            current = "";
        } else {
            current += c;
        }
    }
    if (current.length()) {
        result.push_back(current);
    }
    return result;
}

std::string trim(const std::string& str) {
    int start = 0;
    int end = str.length() - 1;
    while ((start < end) && isWhitespace(str[start])) {
        start++;
    }
    while ((start < end) && isWhitespace(str[end])) {
        end--;
    }
    return str.substr(start, end - start + 1);
}

bool isStringInCstrList(const std::string& key, const char* list[]) {
    for (int i = 0; list[i]; i++) {
        if (key == list[i]) {
            return true;
        }
    }
    return false;
}
