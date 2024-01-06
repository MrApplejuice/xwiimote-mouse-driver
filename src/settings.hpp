#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <memory>
#include <unordered_map>

#include "intlinalg.hpp"

static const std::string DEFAULT_CONFIG_PATH = "./wiimote-mouse.conf";
extern const char* VECTOR_OPTIONS[128];

std::shared_ptr<Vector3> parseVector3(const std::string& str);
std::string vector3ToString(const Vector3& vec);
bool isVectorOption(const std::string& key);

class Config {
private:
    std::string filePath;
public:
    std::unordered_map<std::string, std::string> stringOptions;
    std::unordered_map<std::string, Vector3> vectorOptions;

    Config(const std::string& filePath) : filePath(filePath) {}

    void provideDefault(const std::string& key, const std::string& value) {
        if (stringOptions.find(key) == stringOptions.end()) {
            stringOptions[key] = value;
        }
    }

    bool parseConfigFile() {
        std::ifstream configFile(filePath);
        if (!configFile.is_open()) {
            std::cerr << "Failed to open config file: " << filePath << std::endl;
            return false;
        }

        stringOptions.clear();
        vectorOptions.clear();

        std::string line;
        while (std::getline(configFile, line)) {
            std::istringstream iss(line);
            std::string key, value;
            if (std::getline(iss, key, '=') && std::getline(iss, value)) {
                if (isVectorOption(key)) {
                    auto vec = parseVector3(value);
                    if (!vec) {
                        std::cerr << "Failed to parse vector option: " << key << std::endl;
                        return false;
                    }
                    vectorOptions[key] = *vec;
                } else {
                    stringOptions[key] = value;
                }
            }
        }

        configFile.close();
        return true;
    }

    bool writeConfigFile() {
        std::ofstream configFile(filePath);
        if (!configFile.is_open()) {
            std::cerr << "Failed to open config file: " << filePath << std::endl;
            return false;
        }

        for (auto& pair : stringOptions) {
            configFile << pair.first << "=" << pair.second << std::endl;
        }
        for (auto& pair : vectorOptions) {
            configFile << pair.first << "=" << vector3ToString(pair.second) << std::endl;
        }

        configFile.close();
        return true;
    }
};
