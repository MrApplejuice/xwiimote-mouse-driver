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

#include <cmath>

#include "intlinalg.hpp"

struct Vector3f {
    float values[3];
    float cachedLen;

    bool operator==(const Vector3f& other) const {
        return values[0] == other.values[0] && 
            values[1] == other.values[1] && 
            values[2] == other.values[2];
    }

    Vector3f operator+(const Vector3f& other) const {
        return Vector3f(
            values[0] + other.values[0],
            values[1] + other.values[1],
            values[2] + other.values[2]
        );
    }

    Vector3f& operator+=(const Vector3f& other) {
        values[0] += other.values[0];
        values[1] += other.values[1];
        values[2] += other.values[2];
        return *this;
    }

    Vector3f operator-(const Vector3f& other) const {
        return Vector3f(
            values[0] - other.values[0],
            values[1] - other.values[1],
            values[2] - other.values[2]
        );
    }

    Vector3f operator*(float scalar) const {
        return Vector3f(
            values[0] * scalar,
            values[1] * scalar,
            values[2] * scalar
        );
    }

    Vector3f operator/(float scalar) const {
        return Vector3f(
            values[0] / scalar,
            values[1] / scalar,
            values[2] / scalar
        );
    }

    Vector3f& operator/=(float scalar) {
        values[0] /= scalar;
        values[1] /= scalar;
        values[2] /= scalar;
        return *this;
    }

    float& operator[](int index) {
        assert(index >= 0 && index < 3);
        return values[index];
    }

    const float& operator[](int index) const {
        assert(index >= 0 && index < 3);
        return values[index];
    }

    float dot(const Vector3f& other) const {
        return values[0] * other.values[0] + 
            values[1] * other.values[1] + 
            values[2] * other.values[2];
    }

    float len() {
        if (cachedLen < 0) {
            cachedLen = sqrt(dot(*this));
        }
        return cachedLen;
    }

    Vector3 toVector3(int64_t divisor) const {
        return Vector3(
            Scalar(values[0], divisor),
            Scalar(values[1], divisor),
            Scalar(values[2], divisor)
        );
    }

    Vector3f() : values{0, 0, 0}, cachedLen(0) {}
    Vector3f(float x, float y, float z) : values{x, y, z}, cachedLen(-1) {}
    Vector3f(const Vector3& other) {
        values[0] = other.values[0].toFloat();
        values[1] = other.values[1].toFloat();
        values[2] = other.values[2].toFloat();
        cachedLen = other.cachedLen.toFloat();
    }

    Vector3f& operator=(const Vector3f& other) = default;
    Vector3f(const Vector3f& other) = default;
};

static std::ostream& operator<<(std::ostream& out, const Vector3f& v) {
    out << "Vector3f(x=" << v.values[0] << " y=" << v.values[1] << " z=" << v.values[2] << ")";
    return out;
}

static constexpr float clamp(float v, float min, float max) {
    return (v < min) ? min : ((v > max) ? max : v);
}
