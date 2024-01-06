#include "settings.hpp"

const char* VECTOR_OPTIONS[128] = {
    "calmatX",
    "calmatY",
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
    for (int i = 0; VECTOR_OPTIONS[i]; i++) {
        if (key == VECTOR_OPTIONS[i]) {
            return true;
        }
    }
    return false;
}
