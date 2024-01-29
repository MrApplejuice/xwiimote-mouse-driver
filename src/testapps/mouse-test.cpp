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
#include <string>

#include "../driver/virtualmouse.hpp"

int main() {
    try {
        VirtualMouse mouse(1000);

        std::string s;

        std::cout << "Press enter to move mouse" << std::endl;
        std::getline(std::cin, s);

        mouse.move(10, 500);

        std::cout << "Press enter to move mouse again!" << std::endl;
        std::getline(std::cin, s);

        mouse.move(500, 50);

        std::cout << "Press enter to press the A-key!" << std::endl;
        std::getline(std::cin, s);

        mouse.button(KEY_A, true);
        mouse.button(KEY_A, false);

        std::cout << "Press enter to quit" << std::endl;
        std::getline(std::cin, s);

        std::cout << "Quitting" << std::endl;
    }
    catch (const std::string& s) {
        std::cout << "ERROR: " << s << std::endl;
    }

    return 0;
}
