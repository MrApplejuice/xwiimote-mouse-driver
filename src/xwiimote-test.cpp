#include <iostream>
#include <string>

#include <xwiimote.h>

template <typename T>
struct UnRefFunc {
    typedef void (*Func)(T);
};


template <typename T>
struct RefcountRef {
    T ref;

    typename UnRefFunc<T>::Func refFunc;
    typename UnRefFunc<T>::Func unrefFunc;

    RefcountRef<T> operator=(const RefcountRef<T>& other) {
        unrefFunc(ref);
        ref = other.ref;
        refFunc = other.refFunc;
        unrefFunc = other.unrefFunc;
        refFunc(ref);
    }

    RefcountRef(const RefcountRef<T>& other) {
        ref = other.ref;
        refFunc = other.refFunc;
        unrefFunc = other.unrefFunc;
        refFunc(ref);
    }

    RefcountRef(const T& ref, typename UnRefFunc<T>::Func refFunc, typename UnRefFunc<T>::Func unrefFunc) {
        this->ref = ref;
        this->refFunc = refFunc;
        this->unrefFunc = unrefFunc;
    }

    ~RefcountRef() {
        unrefFunc(ref);
    }
};

int main() {
    try {
        RefcountRef<xwii_monitor*> mon(
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
            
            RefcountRef<xwii_iface*> dev(
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
