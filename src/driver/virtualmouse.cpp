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

#include "virtualmouse.hpp"

#include <unordered_map>

const std::vector<SupportedButton> SUPPORTED_BUTTONS = {
     {BTN_LEFT, "BTN_LEFT", "Left Button", "Mouse"},
     {BTN_RIGHT, "BTN_RIGHT", "Right Button", "Mouse"},
     {BTN_MIDDLE, "BTN_MIDDLE", "Middle Button", "Mouse"},
     {BTN_FORWARD, "BTN_FORWARD", "Forward", "Mouse"},
     {BTN_BACK, "BTN_BACK", "Back", "Mouse"},
     {BTN_SIDE, "BTN_SIDE", nullptr, "Mouse"},
     {BTN_EXTRA, "BTN_EXTRA", nullptr, "Mouse"},
     {BTN_TASK, "BTN_TASK", nullptr, "Mouse"},
     {KEY_ESC, "KEY_ESC", "Escape", "Keyboard"},
     {KEY_ENTER, "KEY_ENTER", "Enter", "Keyboard"},
     {KEY_1, "KEY_1", "1", "Keyboard"},
     {KEY_2, "KEY_2", "2", "Keyboard"},
     {KEY_3, "KEY_3", "3", "Keyboard"},
     {KEY_4, "KEY_4", "4", "Keyboard"},
     {KEY_5, "KEY_5", "5", "Keyboard"},
     {KEY_6, "KEY_6", "6", "Keyboard"},
     {KEY_7, "KEY_7", "7", "Keyboard"},
     {KEY_8, "KEY_8", "8", "Keyboard"},
     {KEY_9, "KEY_9", "9", "Keyboard"},
     {KEY_0, "KEY_0", "0", "Keyboard"},
     {KEY_MINUS, "KEY_MINUS", "-", "Keyboard"},
     {KEY_EQUAL, "KEY_EQUAL", "=", "Keyboard"},
     {KEY_BACKSPACE, "KEY_BACKSPACE", "Backspace", "Keyboard"},
     {KEY_TAB, "KEY_TAB", "Tab", "Keyboard"},
     {KEY_Q, "KEY_Q", "Q", "Keyboard"},
     {KEY_W, "KEY_W", "W", "Keyboard"},
     {KEY_E, "KEY_E", "E", "Keyboard"},
     {KEY_R, "KEY_R", "R", "Keyboard"},
     {KEY_T, "KEY_T", "T", "Keyboard"},
     {KEY_Y, "KEY_Y", "Y", "Keyboard"},
     {KEY_U, "KEY_U", "U", "Keyboard"},
     {KEY_I, "KEY_I", "I", "Keyboard"},
     {KEY_O, "KEY_O", "O", "Keyboard"},
     {KEY_P, "KEY_P", "P", "Keyboard"},
     {KEY_LEFTBRACE, "KEY_LEFTBRACE", "[", "Keyboard"},
     {KEY_RIGHTBRACE, "KEY_RIGHTBRACE", "]", "Keyboard"},
     {KEY_A, "KEY_A", "A", "Keyboard"},
     {KEY_S, "KEY_S", "S", "Keyboard"},
     {KEY_D, "KEY_D", "D", "Keyboard"},
     {KEY_F, "KEY_F", "F", "Keyboard"},
     {KEY_G, "KEY_G", "G", "Keyboard"},
     {KEY_H, "KEY_H", "H", "Keyboard"},
     {KEY_J, "KEY_J", "J", "Keyboard"},
     {KEY_K, "KEY_K", "K", "Keyboard"},
     {KEY_L, "KEY_L", "L", "Keyboard"},
     {KEY_SEMICOLON, "KEY_SEMICOLON", ";", "Keyboard"},
     {KEY_APOSTROPHE, "KEY_APOSTROPHE", "'", "Keyboard"},
     {KEY_GRAVE, "KEY_GRAVE", "`", "Keyboard"},
     {KEY_BACKSLASH, "KEY_BACKSLASH", "\\", "Keyboard"},
     {KEY_Z, "KEY_Z", "Z", "Keyboard"},
     {KEY_X, "KEY_X", "X", "Keyboard"},
     {KEY_C, "KEY_C", "C", "Keyboard"},
     {KEY_V, "KEY_V", "V", "Keyboard"},
     {KEY_B, "KEY_B", "B", "Keyboard"},
     {KEY_N, "KEY_N", "N", "Keyboard"},
     {KEY_M, "KEY_M", "M", "Keyboard"},
     {KEY_COMMA, "KEY_COMMA", ",", "Keyboard"},
     {KEY_DOT, "KEY_DOT", ".", "Keyboard"},
     {KEY_SLASH, "KEY_SLASH", "/", "Keyboard"},
     {KEY_SPACE, "KEY_SPACE", "Space", "Keyboard"},
     {KEY_F1, "KEY_F1", "F1", "Keyboard"},
     {KEY_F2, "KEY_F2", "F2", "Keyboard"},
     {KEY_F3, "KEY_F3", "F3", "Keyboard"},
     {KEY_F4, "KEY_F4", "F4", "Keyboard"},
     {KEY_F5, "KEY_F5", "F5", "Keyboard"},
     {KEY_F6, "KEY_F6", "F6", "Keyboard"},
     {KEY_F7, "KEY_F7", "F7", "Keyboard"},
     {KEY_F8, "KEY_F8", "F8", "Keyboard"},
     {KEY_F9, "KEY_F9", "F9", "Keyboard"},
     {KEY_F10, "KEY_F10", "F10", "Keyboard"},
     {KEY_F11, "KEY_F11", "F11", "Keyboard"},
     {KEY_F12, "KEY_F12", "F12", "Keyboard"},
     {KEY_LEFTALT, "KEY_LEFTALT", "Left Alt", "Keyboard"},
     {KEY_LEFTSHIFT, "KEY_LEFTSHIFT", "Left Shift", "Keyboard"},
     {KEY_LEFTCTRL, "KEY_LEFTCTRL", "Left Control", "Keyboard"},
     {KEY_LEFTMETA, "KEY_LEFTMETA", "Left Meta", "Keyboard"},
     {KEY_RIGHTSHIFT, "KEY_RIGHTSHIFT", "Right Shift", "Keyboard"},
     {KEY_RIGHTALT, "KEY_RIGHTALT", "Right Alt", "Keyboard"},
     {KEY_RIGHTCTRL, "KEY_RIGHTCTRL", "Right Control", "Keyboard"},
     {KEY_RIGHTMETA, "KEY_RIGHTMETA", "Right Meta", "Keyboard"},
     {KEY_CAPSLOCK, "KEY_CAPSLOCK", "Caps Lock", "Keyboard"},
     {KEY_NUMLOCK, "KEY_NUMLOCK", "Numlock", "Keyboard"},
     {KEY_SCROLLLOCK, "KEY_SCROLLLOCK", "Scrollock", "Keyboard"},
     {KEY_KPASTERISK, "KEY_KPASTERISK", "Keypad *", "Keyboard"},
     {KEY_KP1, "KEY_KP1", "Keypad 1", "Keyboard"},
     {KEY_KP2, "KEY_KP2", "Keypad 2", "Keyboard"},
     {KEY_KP3, "KEY_KP3", "Keypad 3", "Keyboard"},
     {KEY_KP4, "KEY_KP4", "Keypad 4", "Keyboard"},
     {KEY_KP5, "KEY_KP5", "Keypad 5", "Keyboard"},
     {KEY_KP6, "KEY_KP6", "Keypad 6", "Keyboard"},
     {KEY_KP7, "KEY_KP7", "Keypad 7", "Keyboard"},
     {KEY_KP8, "KEY_KP8", "Keypad 8", "Keyboard"},
     {KEY_KP9, "KEY_KP9", "Keypad 9", "Keyboard"},
     {KEY_KP0, "KEY_KP0", "Keypad 0", "Keyboard"},
     {KEY_KPPLUS, "KEY_KPPLUS", "Keypad +", "Keyboard"},
     {KEY_KPMINUS, "KEY_KPMINUS", "Keypad -", "Keyboard"},
     {KEY_KPDOT, "KEY_KPDOT", "Keypad .", "Keyboard"},
     {KEY_KPENTER, "KEY_KPENTER", "Keypad Enter", "Keyboard"},
     {KEY_KPSLASH, "KEY_KPSLASH", "Keypad /", "Keyboard"},
     {KEY_SYSRQ, "KEY_SYSRQ", "Sys Rq", "Keyboard"},
     {KEY_INSERT, "KEY_INSERT", "Insert", "Keyboard"},
     {KEY_DELETE, "KEY_DELETE", "Delete", "Keyboard"},
     {KEY_HOME, "KEY_HOME", "Home", "Keyboard"},
     {KEY_END, "KEY_END", "End", "Keyboard"},
     {KEY_PAGEUP, "KEY_PAGEUP", "Page Up", "Keyboard"},
     {KEY_PAGEDOWN, "KEY_PAGEDOWN", "Page Down", "Keyboard"},
     {KEY_LEFT, "KEY_LEFT", "Left", "Keyboard"},
     {KEY_RIGHT, "KEY_RIGHT", "Right", "Keyboard"},
     {KEY_UP, "KEY_UP", "Up", "Keyboard"},
     {KEY_DOWN, "KEY_DOWN", "Down", "Keyboard"},
     {KEY_KPJPCOMMA, "KEY_KPJPCOMMA", nullptr, "Extended Keyboard"},
     {KEY_ZENKAKUHANKAKU, "KEY_ZENKAKUHANKAKU", nullptr, "Extended Keyboard"},
     {KEY_102ND, "KEY_102ND", nullptr, "Extended Keyboard"},
     {KEY_RO, "KEY_RO", nullptr, "Extended Keyboard"},
     {KEY_KATAKANA, "KEY_KATAKANA", nullptr, "Extended Keyboard"},
     {KEY_HIRAGANA, "KEY_HIRAGANA", nullptr, "Extended Keyboard"},
     {KEY_HENKAN, "KEY_HENKAN", nullptr, "Extended Keyboard"},
     {KEY_KATAKANAHIRAGANA, "KEY_KATAKANAHIRAGANA", nullptr, "Extended Keyboard"},
     {KEY_MUHENKAN, "KEY_MUHENKAN", nullptr, "Extended Keyboard"},
     {KEY_LINEFEED, "KEY_LINEFEED", nullptr, "Extended Keyboard"},
     {KEY_MACRO, "KEY_MACRO", nullptr, "Extended Keyboard"},
     {KEY_MUTE, "KEY_MUTE", nullptr, "Extended Keyboard"},
     {KEY_VOLUMEDOWN, "KEY_VOLUMEDOWN", nullptr, "Extended Keyboard"},
     {KEY_VOLUMEUP, "KEY_VOLUMEUP", nullptr, "Extended Keyboard"},
     {KEY_POWER, "KEY_POWER", nullptr, "Extended Keyboard"},
     {KEY_KPEQUAL, "KEY_KPEQUAL", nullptr, "Extended Keyboard"},
     {KEY_KPPLUSMINUS, "KEY_KPPLUSMINUS", nullptr, "Extended Keyboard"},
     {KEY_PAUSE, "KEY_PAUSE", nullptr, "Extended Keyboard"},
     {KEY_SCALE, "KEY_SCALE", nullptr, "Extended Keyboard"},
     {KEY_KPCOMMA, "KEY_KPCOMMA", nullptr, "Extended Keyboard"},
     {KEY_HANGEUL, "KEY_HANGEUL", nullptr, "Extended Keyboard"},
     {KEY_HANJA, "KEY_HANJA", nullptr, "Extended Keyboard"},
     {KEY_YEN, "KEY_YEN", nullptr, "Extended Keyboard"},
     {KEY_COMPOSE, "KEY_COMPOSE", nullptr, "Extended Keyboard"},
     {KEY_STOP, "KEY_STOP", nullptr, "Extended Keyboard"},
     {KEY_AGAIN, "KEY_AGAIN", nullptr, "Extended Keyboard"},
     {KEY_PROPS, "KEY_PROPS", nullptr, "Extended Keyboard"},
     {KEY_UNDO, "KEY_UNDO", nullptr, "Extended Keyboard"},
     {KEY_FRONT, "KEY_FRONT", nullptr, "Extended Keyboard"},
     {KEY_COPY, "KEY_COPY", nullptr, "Extended Keyboard"},
     {KEY_OPEN, "KEY_OPEN", nullptr, "Extended Keyboard"},
     {KEY_PASTE, "KEY_PASTE", nullptr, "Extended Keyboard"},
     {KEY_FIND, "KEY_FIND", nullptr, "Extended Keyboard"},
     {KEY_CUT, "KEY_CUT", nullptr, "Extended Keyboard"},
     {KEY_HELP, "KEY_HELP", nullptr, "Extended Keyboard"},
     {KEY_MENU, "KEY_MENU", nullptr, "Extended Keyboard"},
     {KEY_CALC, "KEY_CALC", nullptr, "Extended Keyboard"},
     {KEY_SETUP, "KEY_SETUP", nullptr, "Extended Keyboard"},
     {KEY_SLEEP, "KEY_SLEEP", nullptr, "Extended Keyboard"},
     {KEY_WAKEUP, "KEY_WAKEUP", nullptr, "Extended Keyboard"},
     {KEY_FILE, "KEY_FILE", nullptr, "Extended Keyboard"},
     {KEY_SENDFILE, "KEY_SENDFILE", nullptr, "Extended Keyboard"},
     {KEY_DELETEFILE, "KEY_DELETEFILE", nullptr, "Extended Keyboard"},
     {KEY_XFER, "KEY_XFER", nullptr, "Extended Keyboard"},
     {KEY_PROG1, "KEY_PROG1", nullptr, "Extended Keyboard"},
     {KEY_PROG2, "KEY_PROG2", nullptr, "Extended Keyboard"},
     {KEY_WWW, "KEY_WWW", nullptr, "Extended Keyboard"},
     {KEY_MSDOS, "KEY_MSDOS", nullptr, "Extended Keyboard"},
     {KEY_COFFEE, "KEY_COFFEE", nullptr, "Extended Keyboard"},
     {KEY_SCREENLOCK, "KEY_SCREENLOCK", nullptr, "Extended Keyboard"},
     {KEY_ROTATE_DISPLAY, "KEY_ROTATE_DISPLAY", nullptr, "Extended Keyboard"},
     {KEY_DIRECTION, "KEY_DIRECTION", nullptr, "Extended Keyboard"},
     {KEY_CYCLEWINDOWS, "KEY_CYCLEWINDOWS", nullptr, "Extended Keyboard"},
     {KEY_MAIL, "KEY_MAIL", nullptr, "Extended Keyboard"},
     {KEY_BOOKMARKS, "KEY_BOOKMARKS", nullptr, "Extended Keyboard"},
     {KEY_COMPUTER, "KEY_COMPUTER", nullptr, "Extended Keyboard"},
     {KEY_BACK, "KEY_BACK", nullptr, "Extended Keyboard"},
     {KEY_FORWARD, "KEY_FORWARD", nullptr, "Extended Keyboard"},
     {KEY_CLOSECD, "KEY_CLOSECD", nullptr, "Extended Keyboard"},
     {KEY_EJECTCD, "KEY_EJECTCD", nullptr, "Extended Keyboard"},
     {KEY_EJECTCLOSECD, "KEY_EJECTCLOSECD", nullptr, "Extended Keyboard"},
     {KEY_NEXTSONG, "KEY_NEXTSONG", nullptr, "Extended Keyboard"},
     {KEY_PLAYPAUSE, "KEY_PLAYPAUSE", nullptr, "Extended Keyboard"},
     {KEY_PREVIOUSSONG, "KEY_PREVIOUSSONG", nullptr, "Extended Keyboard"},
     {KEY_STOPCD, "KEY_STOPCD", nullptr, "Extended Keyboard"},
     {KEY_RECORD, "KEY_RECORD", nullptr, "Extended Keyboard"},
     {KEY_REWIND, "KEY_REWIND", nullptr, "Extended Keyboard"},
     {KEY_PHONE, "KEY_PHONE", nullptr, "Extended Keyboard"},
     {KEY_ISO, "KEY_ISO", nullptr, "Extended Keyboard"},
     {KEY_CONFIG, "KEY_CONFIG", nullptr, "Extended Keyboard"},
     {KEY_HOMEPAGE, "KEY_HOMEPAGE", nullptr, "Extended Keyboard"},
     {KEY_REFRESH, "KEY_REFRESH", nullptr, "Extended Keyboard"},
     {KEY_EXIT, "KEY_EXIT", nullptr, "Extended Keyboard"},
     {KEY_MOVE, "KEY_MOVE", nullptr, "Extended Keyboard"},
     {KEY_EDIT, "KEY_EDIT", nullptr, "Extended Keyboard"},
     {KEY_SCROLLUP, "KEY_SCROLLUP", nullptr, "Extended Keyboard"},
     {KEY_SCROLLDOWN, "KEY_SCROLLDOWN", nullptr, "Extended Keyboard"},
     {KEY_KPLEFTPAREN, "KEY_KPLEFTPAREN", nullptr, "Extended Keyboard"},
     {KEY_KPRIGHTPAREN, "KEY_KPRIGHTPAREN", nullptr, "Extended Keyboard"},
     {KEY_NEW, "KEY_NEW", nullptr, "Extended Keyboard"},
     {KEY_REDO, "KEY_REDO", nullptr, "Extended Keyboard"},
     {KEY_F13, "KEY_F13", nullptr, "Extended Keyboard"},
     {KEY_F14, "KEY_F14", nullptr, "Extended Keyboard"},
     {KEY_F15, "KEY_F15", nullptr, "Extended Keyboard"},
     {KEY_F16, "KEY_F16", nullptr, "Extended Keyboard"},
     {KEY_F17, "KEY_F17", nullptr, "Extended Keyboard"},
     {KEY_F18, "KEY_F18", nullptr, "Extended Keyboard"},
     {KEY_F19, "KEY_F19", nullptr, "Extended Keyboard"},
     {KEY_F20, "KEY_F20", nullptr, "Extended Keyboard"},
     {KEY_F21, "KEY_F21", nullptr, "Extended Keyboard"},
     {KEY_F22, "KEY_F22", nullptr, "Extended Keyboard"},
     {KEY_F23, "KEY_F23", nullptr, "Extended Keyboard"},
     {KEY_F24, "KEY_F24", nullptr, "Extended Keyboard"},
     {KEY_PLAYCD, "KEY_PLAYCD", nullptr, "Extended Keyboard"},
     {KEY_PAUSECD, "KEY_PAUSECD", nullptr, "Extended Keyboard"},
     {KEY_PROG3, "KEY_PROG3", nullptr, "Extended Keyboard"},
     {KEY_PROG4, "KEY_PROG4", nullptr, "Extended Keyboard"},
     {KEY_ALL_APPLICATIONS, "KEY_ALL_APPLICATIONS", nullptr, "Extended Keyboard"},
     {KEY_DASHBOARD, "KEY_DASHBOARD", nullptr, "Extended Keyboard"},
     {KEY_SUSPEND, "KEY_SUSPEND", nullptr, "Extended Keyboard"},
     {KEY_CLOSE, "KEY_CLOSE", nullptr, "Extended Keyboard"},
     {KEY_PLAY, "KEY_PLAY", nullptr, "Extended Keyboard"},
     {KEY_FASTFORWARD, "KEY_FASTFORWARD", nullptr, "Extended Keyboard"},
     {KEY_BASSBOOST, "KEY_BASSBOOST", nullptr, "Extended Keyboard"},
     {KEY_PRINT, "KEY_PRINT", nullptr, "Extended Keyboard"},
     {KEY_HP, "KEY_HP", nullptr, "Extended Keyboard"},
     {KEY_CAMERA, "KEY_CAMERA", nullptr, "Extended Keyboard"},
     {KEY_SOUND, "KEY_SOUND", nullptr, "Extended Keyboard"},
     {KEY_QUESTION, "KEY_QUESTION", nullptr, "Extended Keyboard"},
     {KEY_EMAIL, "KEY_EMAIL", nullptr, "Extended Keyboard"},
     {KEY_CHAT, "KEY_CHAT", nullptr, "Extended Keyboard"},
     {KEY_SEARCH, "KEY_SEARCH", nullptr, "Extended Keyboard"},
     {KEY_CONNECT, "KEY_CONNECT", nullptr, "Extended Keyboard"},
     {KEY_FINANCE, "KEY_FINANCE", nullptr, "Extended Keyboard"},
     {KEY_SPORT, "KEY_SPORT", nullptr, "Extended Keyboard"},
     {KEY_SHOP, "KEY_SHOP", nullptr, "Extended Keyboard"},
     {KEY_ALTERASE, "KEY_ALTERASE", nullptr, "Extended Keyboard"},
     {KEY_CANCEL, "KEY_CANCEL", nullptr, "Extended Keyboard"},
     {KEY_BRIGHTNESSDOWN, "KEY_BRIGHTNESSDOWN", nullptr, "Extended Keyboard"},
     {KEY_BRIGHTNESSUP, "KEY_BRIGHTNESSUP", nullptr, "Extended Keyboard"},
     {KEY_MEDIA, "KEY_MEDIA", nullptr, "Extended Keyboard"},
     {KEY_SWITCHVIDEOMODE, "KEY_SWITCHVIDEOMODE", nullptr, "Extended Keyboard"},
     {KEY_KBDILLUMTOGGLE, "KEY_KBDILLUMTOGGLE", nullptr, "Extended Keyboard"},
     {KEY_KBDILLUMDOWN, "KEY_KBDILLUMDOWN", nullptr, "Extended Keyboard"},
     {KEY_KBDILLUMUP, "KEY_KBDILLUMUP", nullptr, "Extended Keyboard"},
     {KEY_SEND, "KEY_SEND", nullptr, "Extended Keyboard"},
     {KEY_REPLY, "KEY_REPLY", nullptr, "Extended Keyboard"},
     {KEY_FORWARDMAIL, "KEY_FORWARDMAIL", nullptr, "Extended Keyboard"},
     {KEY_SAVE, "KEY_SAVE", nullptr, "Extended Keyboard"},
     {KEY_DOCUMENTS, "KEY_DOCUMENTS", nullptr, "Extended Keyboard"},
     {KEY_BATTERY, "KEY_BATTERY", nullptr, "Extended Keyboard"},
     {KEY_BLUETOOTH, "KEY_BLUETOOTH", nullptr, "Extended Keyboard"},
     {KEY_WLAN, "KEY_WLAN", nullptr, "Extended Keyboard"},
     {KEY_UWB, "KEY_UWB", nullptr, "Extended Keyboard"},
     {KEY_VIDEO_NEXT, "KEY_VIDEO_NEXT", nullptr, "Extended Keyboard"},
     {KEY_VIDEO_PREV, "KEY_VIDEO_PREV", nullptr, "Extended Keyboard"},
     {KEY_BRIGHTNESS_CYCLE, "KEY_BRIGHTNESS_CYCLE", nullptr, "Extended Keyboard"},
     {KEY_BRIGHTNESS_AUTO, "KEY_BRIGHTNESS_AUTO", nullptr, "Extended Keyboard"},
     {KEY_BRIGHTNESS_ZERO, "KEY_BRIGHTNESS_ZERO", nullptr, "Extended Keyboard"},
     {KEY_DISPLAY_OFF, "KEY_DISPLAY_OFF", nullptr, "Extended Keyboard"},
     {KEY_WWAN, "KEY_WWAN", nullptr, "Extended Keyboard"},
     {KEY_WIMAX, "KEY_WIMAX", nullptr, "Extended Keyboard"},
     {KEY_RFKILL, "KEY_RFKILL", nullptr, "Extended Keyboard"},
     {KEY_MICMUTE, "KEY_MICMUTE", nullptr, "Extended Keyboard"},
};

const SupportedButton* findButtonByName(const std::string& rawName) {
    static std::unordered_map<std::string, const SupportedButton*> QUICK_LOOKUP;
    if (QUICK_LOOKUP.empty()) {
        for (auto& button : SUPPORTED_BUTTONS) {
            if (button.name) {
                QUICK_LOOKUP[button.name] = &button;
            }
        }
        for (auto& button : SUPPORTED_BUTTONS) {
            QUICK_LOOKUP[button.rawKeyName] = &button;
        }
    }

    auto it = QUICK_LOOKUP.find(rawName);
    if (it == QUICK_LOOKUP.end()) {
        return nullptr;
    }
    return it->second;
}

const SupportedButton* findButtonByCode(int id) {
    static std::unordered_map<int, const SupportedButton*> QUICK_LOOKUP;
    if (QUICK_LOOKUP.empty()) {
        for (auto& button : SUPPORTED_BUTTONS) {
            QUICK_LOOKUP[button.code] = &button;
        }
    }

    auto it = QUICK_LOOKUP.find(id);
    if (it == QUICK_LOOKUP.end()) {
        return nullptr;
    }
    return it->second;
}
