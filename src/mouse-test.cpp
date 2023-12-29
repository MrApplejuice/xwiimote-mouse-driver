#include <iostream>
#include <string>

#include "virtualmouse.hpp"

int main() {
    try {
        VirtualMouse mouse(1000);

        std::string s;

        std::cout << "Press enter to move mouse" << std::endl;
        std::getline(std::cin, s);

        mouse.moveMouse(10, 500);

        std::cout << "Press enter to move mouse again!" << std::endl;
        std::getline(std::cin, s);

        mouse.moveMouse(500, 50);

        std::cout << "Press enter to perform a right-click!" << std::endl;
        std::getline(std::cin, s);

        mouse.setButtonPressed(2, true);
        mouse.setButtonPressed(2, false);

        std::cout << "Press enter to quit" << std::endl;
        std::getline(std::cin, s);

        std::cout << "Quitting" << std::endl;
    }
    catch (const std::string& s) {
        std::cout << "ERROR: " << s << std::endl;
    }

    return 0;
}
