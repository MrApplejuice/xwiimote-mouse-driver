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

struct Extents {
    int width;
    int height;

    Extents() {
        width = 0;
        height = 0;
    }
    Extents(int width, int height) {
        this->width = width;
        this->height = height;
    }
    Extents(const Extents& other) = default;
};