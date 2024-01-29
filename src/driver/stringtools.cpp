/*
This file is part of xwiimote-mouse-driver.

xwiimote-mouse-driver is free software: you can redistribute it and/or modify it under 
the terms of the GNU General Public License as published by the Free 
Software Foundation, either version 3 of the License, or (at your option) 
any later version.

xwiimote-mouse-driver is distributed in the hope that it will be useful, but WITHOUT 
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with 
xwiimote-mouse-driver. If not, see <https://www.gnu.org/licenses/>. 
*/

#include "stringtools.hpp"

#include <algorithm>

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
    result.push_back(current);
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

std::string replaceAll(const std::string& str, const std::string& from, const std::string& to) {
    std::string result = str;
    size_t pos = 0;
    while ((pos = result.find(from, pos)) != std::string::npos) {
        result.replace(pos, from.length(), to);
        pos += to.length();
    }
    return result;
}

bool isStringInCstrList(const std::string& key, const char* list[]) {
    for (int i = 0; list[i]; i++) {
        if (key == list[i]) {
            return true;
        }
    }
    return false;
}

std::string asciiLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}  