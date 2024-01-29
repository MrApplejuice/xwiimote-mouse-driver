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

#pragma once

#include <iostream>
#include <string>
#include <unordered_map>
#include <exception>

#include "stringtools.hpp"

struct OptionsMap : public std::unordered_map<std::string, std::string> {
    std::string defaultString(const std::string& key, const std::string& defaultValue) const {
        auto it = find(key);
        if (it == end()) {
            return defaultValue;
        }
        return it->second;
    }
};

class InvalidOptionException : public std::exception {
private:
    std::string arg;
    std::string fullWhatMessage;

    void initWhatMessage() {
        fullWhatMessage = "Invalid option";
        if (!arg.empty()) {
            fullWhatMessage = fullWhatMessage + ": " + arg;
        }
    }
public:
    InvalidOptionException() { 
        initWhatMessage();
    }
    InvalidOptionException(const std::string& arg) : arg(arg) {
        initWhatMessage();
    }
    InvalidOptionException(const InvalidOptionException& other) = default;

    const char* what() const noexcept override {
        return fullWhatMessage.c_str();
    }
};

const static char* VALID_COMMANDLINE_OPTIONS[] = {
    "socket-path",
    "config-file",
    "help",
    "version",
    nullptr
};

const static char* COMMANDLINE_OPTIONS_WITH_ARG[] = {
    "socket-path",
    "config-file",
    nullptr
};

static const std::string HELP_TEXT = 
R"(Usage: xwiimote-mouse-driver [options]
Options:
    --socket-path=<path>  Path to the control socket
    --config-file=<path>  Path to the config file
    --help                Print this help message
    --version             Print the version number
)";

static void printHelp() {
    std::cout << HELP_TEXT << std::endl;
}


static const std::string VERSION_STRING = "0.1";
static const std::string VERSION_TEXT =
R"(xwiimote-mouse-driver ##VERSION_STRING##

xwiimote-mouse-driver is a user-space mouse driver that allows using a wiimote
as a mouse on a desktop computer.


Copyright (C) 2024  Paul Konstantin Gerke

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
)";

static void printVersion() {
    std::cout << replaceAll(VERSION_TEXT, "##VERSION_STRING##", VERSION_STRING) << std::endl;
}

static OptionsMap parseOptions(int argc, char* argv[]) {
    OptionsMap options;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg.substr(0, 2) == "--") {
            std::string::size_type pos = arg.find("=");

            std::string key;
            std::string value;

            if (pos == std::string::npos) {
                key = arg.substr(2);

                if (isStringInCstrList(key, COMMANDLINE_OPTIONS_WITH_ARG)) {
                    if (i + 1 < argc) {
                        value = argv[++i];
                    } else {
                        throw InvalidOptionException(arg);
                    }
                } else {
                    value = "1";
                }
            } else {
                key = arg.substr(2, pos - 2);
                value = arg.substr(pos + 1);

                if (!isStringInCstrList(key, COMMANDLINE_OPTIONS_WITH_ARG)) {
                    throw InvalidOptionException(arg);
                }
            }
            
            if (!isStringInCstrList(key, VALID_COMMANDLINE_OPTIONS)) {  
                throw InvalidOptionException(arg);
            }
            options[key] = value;
        } else {
            throw InvalidOptionException(arg);
        }
    }

    return options;
}
