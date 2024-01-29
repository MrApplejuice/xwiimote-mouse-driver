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

#include "device.hpp"

std::map<WiimoteButton, std::string> WIIMOTE_BUTTON_NAMES = {
    {WiimoteButton::A, "a"},
    {WiimoteButton::B, "b"},
    {WiimoteButton::Plus, "+"},
    {WiimoteButton::Minus, "-"},
    {WiimoteButton::Home, "h"},
    {WiimoteButton::One, "1"},
    {WiimoteButton::Two, "2"},
    {WiimoteButton::Up, "u"},
    {WiimoteButton::Down, "d"},
    {WiimoteButton::Left, "l"},
    {WiimoteButton::Right, "r"}
};

std::map<WiimoteButton, std::string> WIIMOTE_BUTTON_READABLE_NAMES = {
    {WiimoteButton::A, "A"},
    {WiimoteButton::B, "B"},
    {WiimoteButton::Plus, "Plus"},
    {WiimoteButton::Minus, "Minus"},
    {WiimoteButton::Home, "Home"},
    {WiimoteButton::One, "One"},
    {WiimoteButton::Two, "Two"},
    {WiimoteButton::Up, "Up"},
    {WiimoteButton::Down, "Down"},
    {WiimoteButton::Left, "Left"},
    {WiimoteButton::Right, "Right"}
};

std::map<int, WiimoteButton> XWIIMOTE_BUTTON_MAP = {
    {XWII_KEY_A, WiimoteButton::A},
    {XWII_KEY_B, WiimoteButton::B},
    {XWII_KEY_PLUS, WiimoteButton::Plus},
    {XWII_KEY_MINUS, WiimoteButton::Minus},
    {XWII_KEY_HOME, WiimoteButton::Home},
    {XWII_KEY_ONE, WiimoteButton::One},
    {XWII_KEY_TWO, WiimoteButton::Two},
    {XWII_KEY_UP, WiimoteButton::Up},
    {XWII_KEY_DOWN, WiimoteButton::Down},
    {XWII_KEY_LEFT, WiimoteButton::Left},
    {XWII_KEY_RIGHT, WiimoteButton::Right}
};

std::ostream& operator<<(std::ostream& out, const xwii_event_abs& abs) {
    out << "x:" << abs.x << " y:" << abs.y;
    return out;
}
