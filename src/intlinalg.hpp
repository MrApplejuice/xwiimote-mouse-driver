#pragma once

#include <stdint.h>

#include <iostream>
#include <algorithm>

static int isqrt(int x) {
    if (x <= 1) {
        return x;
    }

    int start = 1;
    int end = x / 2;
    int result = start;
    while (start <= end) {
        const int mid = (start + end) / 2;
        const int mid2 = mid * mid;

        if (mid2 == x) {
            return mid;
        }

        if ((mid2 < x) && ((mid2 + 2 * mid + 1) > x)) {
            return mid;
        }

        if (mid2 < x) {
            start = mid + 1;
            result = mid;
        } else {
            end = mid - 1;
        }
    }

    return result;
}

struct Vector3 {
    int64_t values[3];
    int64_t base;
    int64_t cachedLen;

    Vector3 operator-() const {
        return Vector3(
            -values[0],
            -values[1],
            -values[2],
            base
        );
    }

    Vector3 operator+(const Vector3& o) const {
        if (base < o.base) {
            return o + *this;
        }

        return Vector3(
            values[0] * o.base / base + o.values[0],
            values[1] * o.base / base + o.values[1],
            values[2] * o.base / base + o.values[2],
            o.base
        );
    }

    Vector3 operator-(const Vector3& o) const {
        return (-o) + *this;
    }

    Vector3 operator*(int64_t f) const {
        return Vector3(values[0] * f, values[1] * f, values[2] * f, base);
    }

    Vector3 operator*(const Vector3& o) const {
        return Vector3(
            values[0] * o.values[0],
            values[1] * o.values[1],
            values[2] * o.values[2],
            base * o.base
        );
    }

    Vector3 operator/(int64_t f) const {
        if (f < 0) {
            return -(*this) / -f;
        }
        return Vector3(values[0], values[1], values[2], base * f);
    }

    Vector3 rebase(int64_t newBase) const {
        return Vector3(
            values[0] * newBase / base,
            values[1] * newBase / base,
            values[2] * newBase / base,
            abs(newBase)
        );
    }

    Vector3 unbase() const {
        return rebase(1L);
    }

    int64_t len() {
        if (cachedLen < 0) {
            cachedLen = isqrt(len2());
        }
        return cachedLen;
    }

    int64_t len2() {
        Vector3 t = (*this) * (*this);
        return (t.values[0] + t.values[1] + t.values[2]) / t.base;
    }

    Vector3& operator=(const Vector3& other) {
        values[0] = other.values[0];
        values[1] = other.values[1];
        values[2] = other.values[2];
        base = other.base;
        cachedLen = other.cachedLen;
        return *this;
    }

    Vector3() {
        values[0] = values[1] = values[2] = 0L;
        base = 1;
        cachedLen = 0;
    }

    Vector3(const Vector3& other) {
        *this = other;
    }

    Vector3(int64_t _base) {
        values[0] = values[1] = values[2] = 0L;
        base = _base;
        cachedLen = 0;
    }

    Vector3(int64_t x, int64_t y, int64_t z, int64_t _base) {
        values[0] = x;
        values[1] = y;
        values[2] = z;
        base = _base;
        cachedLen = -1;
    }
};

std::ostream& operator<<(std::ostream& out, const Vector3& v) {
    out << "Vector3(x=" << v.values[0] << "/" << v.base 
        << " y=" << v.values[1] << "/" << v.base 
        << " z=" << v.values[2] << "/" << v.base << ")";
    return out;
}