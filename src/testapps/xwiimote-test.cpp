#include <iostream>
#include <string>

#include <xwiimote.h>

#include "../driver/base.hpp"

int main() {
    try {
        XwiiRefcountRef<xwii_monitor*> mon(
            xwii_monitor_new(true, false),
            &xwii_monitor_ref,
            &xwii_monitor_unref
        );

        char* devname;
        while (devname = xwii_monitor_poll(mon.ref)) {
            std::cout << "dev: " << devname << std::endl;

            xwii_iface* rawdev;
            if (xwii_iface_new(&rawdev, devname) < 0) {
                std::cout << "  error allocating device" << std::endl;
                continue;
            }
            
            XwiiRefcountRef<xwii_iface*> dev(
                rawdev,
                &xwii_iface_ref,
                &xwii_iface_unref
            );
            uint8_t battery;
            xwii_iface_get_battery(dev.ref, &battery);
            std::cout << "Battery level: " << (int) battery << std::endl;

            unsigned int ifaces = xwii_iface_available(dev.ref);
            std::cout << "Interfaces: " << (int) ifaces << std::endl;
        }
    }
    catch (const std::string& s) {
        std::cout << "ERROR: " << s << std::endl;
    }

    return 0;
}
