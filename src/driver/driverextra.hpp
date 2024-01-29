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

#include "intlinalg.hpp"

enum class ButtonNamespace {
    NONE,
    WII,
    VMOUSE
};

struct NamespacedButtonState {
    static const NamespacedButtonState NONE;

    ButtonNamespace ns;
    int buttonId;
    bool state;

    bool matches(const NamespacedButtonState& other) const {
        return ns == other.ns && buttonId == other.buttonId;
    }

    operator bool() const {
        return ns != ButtonNamespace::NONE;
    }

    bool operator==(const NamespacedButtonState& other) const {
        return ns == other.ns && buttonId == other.buttonId && state == other.state;
    }

    NamespacedButtonState(const NamespacedButtonState& other) = default;
    NamespacedButtonState(
        ButtonNamespace ns, int buttonId, bool state
    ) : ns(ns), buttonId(buttonId), state(state) {}
    NamespacedButtonState() {
        *this = NONE;
    }
};
