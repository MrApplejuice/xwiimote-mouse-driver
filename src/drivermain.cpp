#include <algorithm>
#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include <exception>

#include <xwiimote.h>

#include "base.hpp"
#include "intlinalg.hpp"
#include "virtualmouse.hpp"

std::ostream& operator<<(std::ostream& out, const xwii_event_abs& abs) {
    out << "x:" << abs.x << " y:" << abs.y;
    return out;
}

class DevFailed : public std::exception {};
class DevInitFailed : public DevFailed {};

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

constexpr int64_t clamp(int64_t v, int64_t min, int64_t max) {
    return (v < min) ? min : ((v > max) ? max : v);
}

class WiiMouse {
private:
    Xwiimote::Ptr wiimote;
    VirtualMouse vmouse;

    std::chrono::time_point<std::chrono::steady_clock> lastupdate;
public:
    void process() {
        std::chrono::time_point<std::chrono::steady_clock> now = 
            std::chrono::steady_clock::now();

        wiimote->poll();

        Vector3 accelVector = Vector3(wiimote->accelX, wiimote->accelY, wiimote->accelZ);
        accelVector = accelVector / accelVector.len();
        accelVector.redivide(10000);

        int x5k = clamp(accelVector.values[0].value, -5000, 5000);
        int y5k = clamp(accelVector.values[1].value, -5000, 5000);

        vmouse.move(x5k + 5000, y5k + 5000);

        lastupdate = now;
    }

    WiiMouse(Xwiimote::Ptr wiimote) : wiimote(wiimote), vmouse(10001) {
        lastupdate = std::chrono::steady_clock::now();
    }
};

int main() {
    XwiimoteMonitor monitor;

    monitor.poll();
    if (monitor.count() <= 0) {
        std::cout << "No Wiimote found. Please pair a new Wiimote now." << std::endl;
        while (monitor.count() <= 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            monitor.poll();
        }
    } 

    std::cout << "Mouse driver started!" << std::endl;
    WiiMouse wmouse(monitor.get_device(0));
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        wmouse.process();
    }

    return 0;
}
