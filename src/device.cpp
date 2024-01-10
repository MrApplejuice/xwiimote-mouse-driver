#include "device.hpp"

std::map<WiimoteButton, std::string> WIIMOTE_BUTTON_NAMES = {
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
