#include <iostream>

#include "intlinalg.hpp"

int main() {
    Vector3 v1(1, 1, 1, 10);
    Vector3 v2(10, 10, 10, 100);
    
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

    return 0;
}