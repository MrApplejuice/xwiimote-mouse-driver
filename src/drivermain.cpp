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
        xwii_iface_open(rawdev, XWII_IFACE_ACCEL);
        accelX = accelY = accelZ = 0;
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

int main() {
    XwiimoteMonitor monitor;



    while (true) {
        monitor.poll();
        std::cout << "Number of wii devices: " << monitor.count() << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        for (int i = 0; i < monitor.count(); i++) {
            std::cout << "Dev " << i + 1 << std::endl;
            Xwiimote::Ptr dev = monitor.get_device(i);
            dev->poll();
            std::cout << "  accel = " << dev->accelX << " " << dev->accelY << " " << dev->accelZ << std::endl;
        }
    }


    return 0;
}
