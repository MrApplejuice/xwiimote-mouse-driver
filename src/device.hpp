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

std::ostream& operator<<(std::ostream& out, const xwii_event_abs& abs) {
    out << "x:" << abs.x << " y:" << abs.y;
    return out;
}

class DevFailed : public std::exception {};
class DevInitFailed : public DevFailed {};

struct WiimoteButtonStates {
    bool a, b, plus, minus, home, one, two, up, down, left, right;

    std::string toMsgState() const {
        std::string result = "";
        if (a) {
            result += ":a";
        }
        if (b) {
            result += ":b";
        }
        if (plus) {
            result += ":+";
        }
        if (minus) {
            result += ":-";
        }
        if (home) {
            result += ":h";
        }
        if (one) {
            result += ":1";
        }
        if (two) {
            result += ":2";
        }
        if (up) {
            result += ":u";
        }
        if (down) {
            result += ":d";
        }
        if (left) {
            result += ":l";
        }
        if (right) {
            result += ":r";
        }
        if (result.size()) {
            result = result.substr(1);
        }
        return result;
    }

    bool operator==(const WiimoteButtonStates& other) const {
        return (
            (a == other.a) &&
            (b == other.b) &&
            (plus == other.plus) &&
            (minus == other.minus) &&
            (home == other.home) &&
            (one == other.one) &&
            (two == other.two) &&
            (up == other.up) &&
            (down == other.down) &&
            (left == other.left) &&
            (right == other.right)
        );
    }

    bool operator!=(const WiimoteButtonStates& other) const {
        return !(*this == other);
    }

    WiimoteButtonStates() {
        a = b = plus = minus = home = one = two = up = down = left = right = false;
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
                if (ev.v.key.code == XWII_KEY_A) {
                    buttonStates.a = ev.v.key.state && true;
                } else if (ev.v.key.code == XWII_KEY_B) {
                    buttonStates.b = ev.v.key.state && true;
                } else if (ev.v.key.code == XWII_KEY_PLUS) {
                    buttonStates.plus = ev.v.key.state && true;
                } else if (ev.v.key.code == XWII_KEY_MINUS) {
                    buttonStates.minus = ev.v.key.state && true;
                } else if (ev.v.key.code == XWII_KEY_HOME) {
                    buttonStates.home = ev.v.key.state && true;
                } else if (ev.v.key.code == XWII_KEY_ONE) {
                    buttonStates.one = ev.v.key.state && true;
                } else if (ev.v.key.code == XWII_KEY_TWO) {
                    buttonStates.two = ev.v.key.state && true;
                } else if (ev.v.key.code == XWII_KEY_UP) {
                    buttonStates.up = ev.v.key.state && true;
                } else if (ev.v.key.code == XWII_KEY_DOWN) {
                    buttonStates.down = ev.v.key.state && true;
                } else if (ev.v.key.code == XWII_KEY_LEFT) {
                    buttonStates.left = ev.v.key.state && true;
                } else if (ev.v.key.code == XWII_KEY_RIGHT) {
                    buttonStates.right = ev.v.key.state && true;
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
