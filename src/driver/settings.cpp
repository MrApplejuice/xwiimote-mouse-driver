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

#include "settings.hpp"

#include "stringtools.hpp"

const char* VALID_OPTIONS[] = {
    "socket_address",
    "calmatx",
    "calmaty",
    "screen_top_left",
    "screen_bottom_right",
    "button_a",
    "button_b",
    "button_plus",
    "button_minus",
    "button_home",
    "button_one",
    "button_two",
    "button_up",
    "button_down",
    "button_left",
    "button_right",
    "button_a_offscreen",
    "button_b_offscreen",
    "button_plus_offscreen",
    "button_minus_offscreen",
    "button_home_offscreen",
    "button_one_offscreen",
    "button_two_offscreen",
    "button_up_offscreen",
    "button_down_offscreen",
    "button_left_offscreen",
    "button_right_offscreen",
    "default_ir_distance",
    "smoothing_clicked_released_delay",
    "towed_circle_radius",
    nullptr
};

const char* VECTOR_OPTIONS[] = {
    "calmatx",
    "calmaty",
    "screen_top_left",
    "screen_bottom_right",
    "smoothing_clicked_released_delay",
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
