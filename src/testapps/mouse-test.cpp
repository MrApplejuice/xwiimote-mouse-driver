/*
This file is part of xwiimote-mouse-driver.

Foobar is free software: you can redistribute it and/or modify it under 
the terms of the GNU General Public License as published by the Free 
Software Foundation, either version 3 of the License, or (at your option) 
any later version.

Foobar is distributed in the hope that it will be useful, but WITHOUT 
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with 
Foobar. If not, see <https://www.gnu.org/licenses/>. 
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
