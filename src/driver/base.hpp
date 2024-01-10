#pragma once

template <typename T>
struct XwiiUnRefFunc {
    typedef void (*Func)(T);
};

template <typename T>
struct XwiiRefcountRef {
    T ref;

    typename XwiiUnRefFunc<T>::Func refFunc;
    typename XwiiUnRefFunc<T>::Func unrefFunc;

    XwiiRefcountRef<T> operator=(const XwiiRefcountRef<T>& other) {
        if (unrefFunc) {
            unrefFunc(ref);
        }

        ref = other.ref;
        refFunc = other.refFunc;
        unrefFunc = other.unrefFunc;

        if (refFunc) {
            refFunc(ref);
        }
        return *this;
    }

    XwiiRefcountRef() {
        ref = nullptr;
        refFunc = nullptr;
        unrefFunc = nullptr;
    }

    XwiiRefcountRef(const XwiiRefcountRef<T>& other) {
        ref = other.ref;
        refFunc = other.refFunc;
        unrefFunc = other.unrefFunc;
        refFunc(ref);
    }

    XwiiRefcountRef(const T& ref, typename XwiiUnRefFunc<T>::Func refFunc, typename XwiiUnRefFunc<T>::Func unrefFunc) {
        this->ref = ref;
        this->refFunc = refFunc;
        this->unrefFunc = unrefFunc;
    }

    ~XwiiRefcountRef() {
        if (unrefFunc) {
            unrefFunc(ref);
        }
    }
};
