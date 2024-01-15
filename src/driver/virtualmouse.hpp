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

#pragma once

#include <algorithm>
#include <exception>
#include <vector>
#include <string>

#include <libevdev/libevdev-uinput.h>

#define VIRTUAL_MOUSE_MAX_BUTTONS 16

class VirtualMouseError : public std::exception {};
class VirtualMouseInvalidArgumentError : public VirtualMouseError {};

struct SupportedButton {
    int code;
    const char* rawKeyName;
    const char* name;
    const char* category;
};

extern const std::vector<SupportedButton> SUPPORTED_BUTTONS;

struct VirtualMouse {
    libevdev* dev;
    libevdev_uinput* uinput;

    int maxAbsValue;

    int buttons[VIRTUAL_MOUSE_MAX_BUTTONS];

    VirtualMouse(int maxAbsValue) : dev(nullptr), uinput(nullptr), maxAbsValue(maxAbsValue) {
        dev = libevdev_new();

        libevdev_set_name(dev, "Wiimote-Mouse Virtual Pointer");
        libevdev_set_id_version(dev, 0x3);

        libevdev_enable_property(dev, INPUT_PROP_POINTER);
        libevdev_enable_property(dev, INPUT_PROP_BUTTONPAD);

        input_absinfo absx_data;
        absx_data.minimum = 0;
        absx_data.maximum = maxAbsValue;
        absx_data.flat = 0;
        absx_data.fuzz = 0;
        absx_data.value = 0;
        absx_data.resolution = 20;

        libevdev_enable_event_type(dev, EV_ABS);
        libevdev_enable_event_code(dev, EV_ABS, ABS_X, &absx_data);

        input_absinfo absy_data;
        absy_data.minimum = 0;
        absy_data.maximum = maxAbsValue;
        absy_data.flat = 0;
        absy_data.fuzz = 0;
        absy_data.value = 0;
        absy_data.resolution = 20;

        libevdev_enable_event_code(dev, EV_ABS, ABS_Y, &absy_data);

        std::fill(buttons, buttons + VIRTUAL_MOUSE_MAX_BUTTONS, KEY_RESERVED);
        buttons[0] = BTN_LEFT;
        buttons[1] = BTN_MIDDLE;
        buttons[2] = BTN_RIGHT;

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

    void move(int x, int y) {
        libevdev_uinput_write_event(uinput, EV_KEY, BTN_TOOL_MOUSE, 1);
        libevdev_uinput_write_event(uinput, EV_ABS, ABS_X, x);
        libevdev_uinput_write_event(uinput, EV_ABS, ABS_Y, y);
        libevdev_uinput_write_event(uinput, EV_SYN, SYN_REPORT, 0);
    }

    void setButtonPressed(int i, bool pressed) {
        if ((i < 0) || (i >= VIRTUAL_MOUSE_MAX_BUTTONS)) {
            throw VirtualMouseInvalidArgumentError();
        }
        int b = buttons[i];
        if (b == KEY_RESERVED) {
            throw VirtualMouseInvalidArgumentError();
        }

        int state = pressed ? 1 : 0;

        libevdev_uinput_write_event(uinput, EV_KEY, BTN_TOOL_MOUSE, 1);
        libevdev_uinput_write_event(uinput, EV_KEY, b, state);
        libevdev_uinput_write_event(uinput, EV_SYN, SYN_REPORT, 0);
    }

    ~VirtualMouse() {
        if (uinput) {
            libevdev_uinput_destroy(uinput);
        }
        libevdev_free(dev);
    }
};
