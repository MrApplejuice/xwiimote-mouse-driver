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
#include <fstream>
#include <sstream>
#include <string>
#include <memory>
#include <unordered_map>

#include "intlinalg.hpp"
#include "stringtools.hpp"

static const std::string DEFAULT_CONFIG_PATH = "./wiimote-mouse.conf";
extern const char* VECTOR_OPTIONS[];

std::shared_ptr<Vector3> parseVector3(const std::string& str);
std::string vector3ToString(const Vector3& vec);

bool isVectorOption(const std::string& key);
bool isValidOption(const std::string& key);

class Config {
private:
    std::string filePath;
public:
    std::unordered_map<std::string, std::string> stringOptions;
    std::unordered_map<std::string, Vector3> vectorOptions;

    Config(const std::string& filePath) : filePath(filePath) {}

    void provideDefault(std::string key, const std::string& value) {
        key = asciiLower(key);
        if (isVectorOption(key)) {
            auto vec = parseVector3(value);
            if (!vec) {
                throw std::runtime_error("Failed to parse vector option: " + key);
            }
            if (vectorOptions.find(key) == vectorOptions.end()) {
                vectorOptions[key] = *vec;
            }
        } else {
            if (stringOptions.find(key) == stringOptions.end()) {
                stringOptions[key] = value;
            }
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
            line += "\n"; // Add back the newline so that the value-getline can engage with it
            
            std::istringstream iss(line);
            std::string key, value;
            if (std::getline(iss, key, '=') && std::getline(iss, value)) {
                if (!isValidOption(key)) {
                    std::cerr << "Ignoring invalid option: " << key << std::endl;
                    continue;
                }
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
            configFile << asciiLower(pair.first) << "=" << pair.second << std::endl;
        }
        for (auto& pair : vectorOptions) {
            configFile << asciiLower(pair.first) << "=" << vector3ToString(pair.second) << std::endl;
        }

        configFile.close();
        return true;
    }
};
