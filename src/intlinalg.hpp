#pragma once

#include <stdint.h>

#include <iostream>
#include <algorithm>
#include <exception>

struct DivisionByZeroError : std::exception {};

static int64_t isqrt(int64_t x) {
    if (x <= 1) {
        return x;
    }

    int64_t start = 1;
    int64_t end = x / 2;
    int64_t result = start;
    while (start <= end) {
        const int64_t mid = (start + end) / 2;
        const int64_t mid2 = mid * mid;

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

struct Scalar {
    int64_t value;
    int64_t divisor;

    bool operator<(const Scalar& other) const {
        if (divisor >= other.divisor) {
            return value < other.redivide(divisor).value;
        } else {
            return redivide(other.divisor).value < other.value;
        }
    }

    bool operator>(const Scalar& other) const {
        if (divisor >= other.divisor) {
            return value > other.redivide(divisor).value;
        } else {
            return redivide(other.divisor).value > other.value;
        }
    }

    bool operator==(const Scalar& other) const {
        if (divisor >= other.divisor) {
            return value == other.redivide(divisor).value;
        } else {
            return other == *this;
        }
    }

    Scalar operator-() const {
        return Scalar(-value, divisor);
    }

    Scalar operator+(const Scalar& o) const {
        if (divisor < o.divisor) {
            return o + *this;
        }

        if (o.divisor == 0) {
            std::cout << o.value << "/" << o.divisor << std::endl;
            throw DivisionByZeroError();
        }

        return Scalar(
            o.redivide(divisor).value + value,
            divisor
        );
    }

    Scalar operator-(const Scalar& o) const {
        return (-o) + *this;
    }

    Scalar operator*(const Scalar& o) const {
        return Scalar(
            value * o.value,
            divisor * o.divisor
        );
    }

    Scalar operator/(const Scalar& f) const {
        if (f < 0) {
            return Scalar(value * -f.divisor, divisor * -f.value);
        } else {
            return Scalar(value * f.divisor, divisor * f.value);
        }
    }

    Scalar redivide(int64_t newDivisor) const {
        return Scalar(
            value * newDivisor / divisor,
            abs(newDivisor)
        );
    }

    Scalar undivide() const {
        return redivide(1L);
    }

    Scalar& operator=(const Scalar& other) {
        if (other.divisor == 0) {
            throw DivisionByZeroError();
        }
        value = other.value;
        divisor = other.divisor;
        return *this;
    }

    Scalar sqrt(int64_t extraPrec=100L) const {
        extraPrec *= extraPrec;
        return Scalar(
            isqrt(value * extraPrec), 
            isqrt(divisor * extraPrec)
        );
    }

    Scalar() {
        value = 0L;
        divisor = 1L;
    }

    Scalar(const Scalar& other) {
        *this = other;
    }

    Scalar(int64_t _value) {
        value = _value;
        divisor = 1L;
    }

    Scalar(int64_t _value, int64_t _divisor) {
        value = _value;
        divisor = _divisor;
        if (_divisor == 0) {
            throw DivisionByZeroError();
        }
    }
};


struct Vector3 {
    Scalar values[3];
    Scalar cachedLen;

    bool operator==(const Vector3& other) const {
        return values[0] == other.values[0] && 
            values[1] == other.values[1] && 
            values[2] == other.values[2];
    }    

    Vector3 operator-() const {
        return Vector3(
            -values[0],
            -values[1],
            -values[2]
        );
    }

    Vector3 operator+(const Vector3& o) const {
        return Vector3(
            values[0] + o.values[0],
            values[1] + o.values[1],
            values[2] + o.values[2]
        );
    }

    Vector3 operator-(const Vector3& o) const {
        return (-o) + *this;
    }

    Vector3 operator*(int64_t f) const {
        return Vector3(values[0] * f, values[1] * f, values[2] * f);
    }

    Vector3 operator*(const Scalar& f) const {
        return Vector3(values[0] * f, values[1] * f, values[2] * f);
    }

    Vector3 operator*(const Vector3& o) const {
        return Vector3(
            values[0] * o.values[0],
            values[1] * o.values[1],
            values[2] * o.values[2]
        );
    }

    Vector3 operator/(int64_t f) const {
        if (f == 0) {
            throw DivisionByZeroError();
        }
        return Vector3(values[0] / f, values[1] / f, values[2] / f);
    }

    Vector3 operator/(const Scalar& f) const {
        if (f == 0) {
            throw DivisionByZeroError();
        }
        return Vector3(values[0] / f, values[1] / f, values[2] / f);
    }

    Vector3 redivide(int64_t newDivisor) const {
        return Vector3(
            values[0].redivide(newDivisor),
            values[1].redivide(newDivisor),
            values[2].redivide(newDivisor)
        );
    }

    Vector3 undivide() const {
        return redivide(1L);
    }

    Scalar len(int64_t extraPrec=100L) {
        if (cachedLen < 0) {
            cachedLen = dot(*this).sqrt(extraPrec);
        }
        return cachedLen;
    }

    Scalar dot(const Vector3& other) {
        Vector3 t = (*this) * other;
        return t.values[0] + t.values[1] + t.values[2];
    }

    Vector3& operator=(const Vector3& other) {
        values[0] = other.values[0];
        values[1] = other.values[1];
        values[2] = other.values[2];
        cachedLen = other.cachedLen;
        return *this;
    }

    Vector3() {
        values[0] = values[1] = values[2] = 0L;
        cachedLen = 0;
    }

    Vector3(const Vector3& other) {
        *this = other;
    }

    Vector3(int64_t x, int64_t y, int64_t z, int64_t _divisor) {
        values[0] = Scalar(x, _divisor);
        values[1] = Scalar(y, _divisor);
        values[2] = Scalar(z, _divisor);
        cachedLen = -1;
    }

    Vector3(const Scalar& x, const Scalar& y, const Scalar& z) {
        values[0] = x;
        values[1] = y;
        values[2] = z;
        cachedLen = -1;
    }
};

static std::ostream& operator<<(std::ostream& out, const Scalar& s) {
    out << s.value << "/" << s.divisor;
    return out;
}

static std::ostream& operator<<(std::ostream& out, const Vector3& v) {
    out << "Vector3(x=" << v.values[0] << " y=" << v.values[1] << " z=" << v.values[2] << ")";
    return out;
}
