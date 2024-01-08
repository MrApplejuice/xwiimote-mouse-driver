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
    nullptr
};

const static char* COMMANDLINE_OPTIONS_WITH_ARG[] = {
    "socket-path",
    "config-file",
    nullptr
};

static void printHelp() {
    std::cout << "Usage: xwiimote-mouse-driver [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --socket-path=<path>  Path to the control socket" << std::endl;
    std::cout << "  --config-file=<path>  Path to the config file" << std::endl;
    std::cout << "  --help                Print this help message" << std::endl;
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
