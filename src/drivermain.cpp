#include <algorithm>
#include <vector>
#include <string>
#include <iostream>

#include <xwiimote.h>

#include "base.hpp"

class Xwiimote {

};

class XwiimoteMonitor {
private:
    XwiiRefcountRef<xwii_monitor*> monitor;

    std::vector<std::string> devices;
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
            if (std::find(devices.begin(), devices.end(), path) == devices.end()) {
                devices.push_back(path);
            }
        }
    }

    int count() {
        return devices.size();
    }
};

int main() {
    XwiimoteMonitor monitor;

    monitor.poll();

    std::cout << "Number of wii devices: " << monitor.count() << std::endl;

    return 0;
}
