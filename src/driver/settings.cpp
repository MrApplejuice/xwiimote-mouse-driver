#include "settings.hpp"

#include "stringtools.hpp"

const char* VALID_OPTIONS[] = {
    "socket_address",
    "calmatX",
    "calmatY",
    "screen_top_left",
    "screen_bottom_right",
    nullptr
};

const char* VECTOR_OPTIONS[] = {
    "calmatX",
    "calmatY",
    "screen_top_left",
    "screen_bottom_right",
    nullptr
};

std::shared_ptr<Vector3> parseVector3(const std::string& str) {
    std::istringstream iss(str);
    std::vector<Scalar> values;

    std::string value;
    while (std::getline(iss, value, ',')) {
        std::istringstream vss(value);

        std::string svalue, sdivisor;
        if (!std::getline(vss, svalue, '/') || !std::getline(vss, sdivisor)) {
            return nullptr;
        }
        try {
            values.push_back(Scalar(std::stoll(svalue), std::stoll(sdivisor)));
        } catch (const std::exception& e) {
            return nullptr;
        }
    }

    if (values.size() != 3) {
        return nullptr;
    }

    return std::shared_ptr<Vector3>(new Vector3(values[0], values[1], values[2]));
}

std::string vector3ToString(const Vector3& vec) {
    std::ostringstream oss;
    oss << vec.values[0] << "," << vec.values[1] << "," << vec.values[2];
    return oss.str();
}

bool isVectorOption(const std::string& key) {
    return isStringInCstrList(key, VECTOR_OPTIONS);
}

bool isValidOption(const std::string& key) {
    return isStringInCstrList(key, VALID_OPTIONS);
}
