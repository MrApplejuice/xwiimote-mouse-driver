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

#include <iostream>

#include "../driver/intlinalg.hpp"

int main() {
    Vector3 v1(1, 1, 1, 10);
    Vector3 v2(10, 10, 10, 100);

    std::cout << "----- Vectors -----" << std::endl;

    {
        Vector3 r = ((v1 + v2) * 10).undivide();
        std::cout << r << std::endl;
        std::cout << "Expected: [2, 2, 2]" << std::endl;
    }

    {
        Vector3 r = ((v1 - v2) * 10).undivide();
        std::cout << r << std::endl;
        std::cout << "Expected: [0, 0, 0]" << std::endl;
    }

    {
        Vector3 r = ((v2 / -2) * 100).undivide();
        std::cout << r << std::endl;
        std::cout << "Expected: [-5, -5, -5]" << std::endl;
    }

    {
        Vector3 r = v1 * 10;
        std::cout << r << std::endl;
        std::cout << "len=" << r.len() << std::endl;
        std::cout << "Expected: len=17" << std::endl;
    }

    {
        Vector3 r = v1 * -100;
        std::cout << r << std::endl;
        std::cout << "len=" << r.len().redivide(100) << std::endl;
        std::cout << "Expected: len=17" << std::endl;
    }

    {
        Vector3 r = -v1;
        r = (r / r.len()).redivide(100);
        std::cout << r << std::endl;
        std::cout << "Expected: ~[-.57, -.57, -.57]" << std::endl;
    }

    std::cout << "----- ilog -----" << std::endl;
    {
        std::cout << "ilog(27, 3)" << std::endl;
        std::cout << int64log(27, 3) << std::endl;
        std::cout << "Expected: 3" << std::endl;
    }

    {
        std::cout << "ilog(1, 3)" << std::endl;
        std::cout << int64log(1, 3) << std::endl;
        std::cout << "Expected: 0" << std::endl;
    }

    {
        std::cout << "ilog(30, 3)" << std::endl;
        std::cout << int64log(30, 3) << std::endl;
        std::cout << "Expected: 3" << std::endl;
    }

    {
        std::cout << "Scalar(450, 10000).logWithBase(3)" << std::endl;
        std::cout << Scalar(450, 10000).logWithBase(3) << std::endl;
        std::cout << "Expected: -2" << std::endl;
        std::cout << int64log(4500, 3) << std::endl;
        std::cout << int64log(10000, 3) << std::endl;
    }

    return 0;
}