#include <iostream>
#include <string>

#include "virtualmouse.hpp"

int main() {
    try {
        VirtualMouse mouse;

        std::string s;

        std::cout << "Press enter to move mouse" << std::endl;
        std::getline(std::cin, s);

        libevdev_uinput_write_event(mouse.uinput, EV_KEY, BTN_TOOL_MOUSE, 1);
        libevdev_uinput_write_event(mouse.uinput, EV_ABS, ABS_X, 10);
        libevdev_uinput_write_event(mouse.uinput, EV_ABS, ABS_Y, 500);
        libevdev_uinput_write_event(mouse.uinput, EV_SYN, SYN_REPORT, 0);

        std::cout << "Press enter to move mouse again!" << std::endl;
        std::getline(std::cin, s);

        libevdev_uinput_write_event(mouse.uinput, EV_KEY, BTN_TOOL_MOUSE, 1);
        libevdev_uinput_write_event(mouse.uinput, EV_ABS, ABS_X, 500);
        libevdev_uinput_write_event(mouse.uinput, EV_ABS, ABS_Y, 50);
        libevdev_uinput_write_event(mouse.uinput, EV_SYN, SYN_REPORT, 0);

        std::cout << "Press enter to perform a right-click!" << std::endl;
        std::getline(std::cin, s);

        libevdev_uinput_write_event(mouse.uinput, EV_KEY, BTN_TOOL_MOUSE, 1);
        libevdev_uinput_write_event(mouse.uinput, EV_KEY, BTN_RIGHT, 1);
        libevdev_uinput_write_event(mouse.uinput, EV_SYN, SYN_REPORT, 0);

        libevdev_uinput_write_event(mouse.uinput, EV_KEY, BTN_TOOL_MOUSE, 1);
        libevdev_uinput_write_event(mouse.uinput, EV_KEY, BTN_RIGHT, 0);
        libevdev_uinput_write_event(mouse.uinput, EV_SYN, SYN_REPORT, 0);

        std::cout << "Press enter to quit" << std::endl;
        std::getline(std::cin, s);

        std::cout << "Quitting" << std::endl;
    }
    catch (const std::string& s) {
        std::cout << "ERROR: " << s << std::endl;
    }

    return 0;
}
