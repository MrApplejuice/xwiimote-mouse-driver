#include <iostream>
#include <string>

#include <libevdev/libevdev-uinput.h>

struct Mouse {
    libevdev* dev;
    libevdev_uinput* uinput;

    Mouse() {
        dev = libevdev_new();
        uinput = nullptr;
        libevdev_set_name(dev, "Wiimote-Mouse Virtual Pointer");
        libevdev_set_id_version(dev, 0x3);

        libevdev_enable_property(dev, INPUT_PROP_POINTER);
        libevdev_enable_property(dev, INPUT_PROP_DIRECT);
        libevdev_enable_property(dev, INPUT_PROP_BUTTONPAD);

        input_absinfo absx_data;
        absx_data.minimum = 0;
        absx_data.maximum = 1000;
        absx_data.flat = 0;
        absx_data.fuzz = 0;
        absx_data.value = 0;
        absx_data.resolution = 20;

        libevdev_enable_event_type(dev, EV_ABS);
        libevdev_enable_event_code(dev, EV_ABS, ABS_X, &absx_data);

        input_absinfo absy_data;
        absy_data.minimum = 0;
        absy_data.maximum = 1000;
        absy_data.flat = 0;
        absy_data.fuzz = 0;
        absy_data.value = 0;
        absy_data.resolution = 20;

        libevdev_enable_event_code(dev, EV_ABS, ABS_Y, &absy_data);

        libevdev_enable_event_type(dev, EV_KEY);
        libevdev_enable_event_code(dev, EV_KEY, BTN_LEFT, nullptr);
        libevdev_enable_event_code(dev, EV_KEY, BTN_RIGHT, nullptr);
        libevdev_enable_event_code(dev, EV_KEY, BTN_MIDDLE, nullptr);
        libevdev_enable_event_code(dev, EV_KEY, BTN_TOOL_MOUSE, nullptr);

        int err = libevdev_uinput_create_from_device(dev, LIBEVDEV_UINPUT_OPEN_MANAGED, &uinput);
        if (err < 0) {
            throw std::string("Failed to create uinput device");
        }
    }

    ~Mouse() {
        if (uinput) {
            libevdev_uinput_destroy(uinput);
        }
        libevdev_free(dev);
    }
};

int main() {
    try {
        Mouse mouse;

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
