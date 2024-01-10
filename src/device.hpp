#pragma once

#include <algorithm>
#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include <exception>

#include <csignal>

#include <xwiimote.h>

#include "base.hpp"

std::ostream& operator<<(std::ostream& out, const xwii_event_abs& abs);

class DevFailed : public std::exception {};
class DevInitFailed : public DevFailed {};

enum class WiimoteButton {
    A, B, Plus, Minus, Home, One, Two, Up, Down, Left, Right, COUNT
};

extern std::map<int, WiimoteButton> XWIIMOTE_BUTTON_MAP;
extern std::map<WiimoteButton, std::string> WIIMOTE_BUTTON_NAMES;

struct WiimoteButtonStates {
    bool pressedButtons[(int) WiimoteButton::COUNT];

    bool isPressed(WiimoteButton button) const {
        return pressedButtons[(int) button];
    }

    std::string toMsgState() const {
        std::string result;
        for (int i = 0; i < (int) WiimoteButton::COUNT; i++) {
            if (pressedButtons[i]) {
                result += ":" + WIIMOTE_BUTTON_NAMES[(WiimoteButton) i];
            }
        }
        if (result.size()) {
            result = result.substr(1);
        }
        return result;
    }

    bool operator==(const WiimoteButtonStates& other) const {
        for (int i = 0; i < (int) WiimoteButton::COUNT; i++) {
            if (pressedButtons[i] != other.pressedButtons[i]) {
                return false;
            }
        }
        return true;
    }

    bool operator!=(const WiimoteButtonStates& other) const {
        return !(*this == other);
    }

    WiimoteButtonStates() {
        std::fill(pressedButtons, pressedButtons + (int) WiimoteButton::COUNT, false);
    }
};

class Xwiimote {
public:
    typedef std::shared_ptr<Xwiimote> Ptr;
private:
    XwiiRefcountRef<xwii_iface*> dev;

    void processAccel(xwii_event& ev) {
        accelX = ev.v.abs->x;
        accelY = ev.v.abs->y;
        accelZ = ev.v.abs->z;
    }
public:
    WiimoteButtonStates buttonStates;
    int accelX, accelY, accelZ;
    xwii_event_abs irdata[4];

    Xwiimote(std::string devName) {
        xwii_iface* rawdev;
        if (xwii_iface_new(&rawdev, devName.c_str()) < 0) {
            throw DevInitFailed();
        }
        dev = XwiiRefcountRef<xwii_iface*>(
            rawdev,
            &xwii_iface_ref,
            &xwii_iface_unref
        );
        xwii_iface_open(rawdev, XWII_IFACE_CORE | XWII_IFACE_ACCEL | XWII_IFACE_IR);
        accelX = accelY = accelZ = 0;
        
        irdata[0].x = irdata[0].y = irdata[0].z = 1023;
        irdata[1].x = irdata[1].y = irdata[1].z = 1023;
        irdata[2].x = irdata[2].y = irdata[2].z = 1023;
        irdata[3].x = irdata[3].y = irdata[3].z = 1023;
    }

    void poll() {
        int err;
        xwii_event ev;
        while (true) {
            err = xwii_iface_dispatch(dev.ref, &ev, sizeof(ev));
            if (err != 0) {
                break;
            }
            if (ev.type == XWII_EVENT_ACCEL) {
                processAccel(ev);
            } else if (ev.type == XWII_EVENT_IR) {
                std::copy(ev.v.abs, ev.v.abs + 4, irdata);
            } else if (ev.type == XWII_EVENT_KEY) {
                auto found = XWIIMOTE_BUTTON_MAP.find(ev.v.key.code);
                if (found != XWIIMOTE_BUTTON_MAP.end()) {
                    buttonStates.pressedButtons[(int) found->second] = ev.v.key.state && true;
                }
            }
        }
        if (err != -EAGAIN) {
            throw DevFailed();
        }
    }
};

class XwiimoteMonitor {
private:
    XwiiRefcountRef<xwii_monitor*> monitor;

    std::vector<std::string> deviceNames;
    std::map<std::string, Xwiimote::Ptr> openedDevices;
public:
    XwiimoteMonitor() {
        monitor = XwiiRefcountRef<xwii_monitor*>(
            xwii_monitor_new(true, false),
            &xwii_monitor_ref,
            &xwii_monitor_unref
        );
    }

    void poll() {
        char* xwii_path; 
        while (xwii_path = xwii_monitor_poll(monitor.ref)) {
            std::string path = xwii_path;
            if (std::find(deviceNames.begin(), deviceNames.end(), path) == deviceNames.end()) {
                deviceNames.push_back(path);
            }
            free(xwii_path);
        }
    }

    int count() {
        return deviceNames.size();
    }

    Xwiimote::Ptr get_device(int i) {
        std::string devName = deviceNames[i];
        auto found = openedDevices.find(devName);
        if (found != openedDevices.cend()) {
            return found->second;
        }
        Xwiimote::Ptr result(new Xwiimote(devName));
        openedDevices[devName] = result;
        return result;
    }
};
