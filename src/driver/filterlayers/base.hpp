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

#include <algorithm>
#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include <exception>

#include "../base.hpp"
#include "../intlinalg.hpp"
#include "../floatlinalg.hpp"
#include "../virtualmouse.hpp"
#include "../driverextra.hpp"

enum class ProcessingOutputHistoryPoint {
    Cluster = 0,
    LastLeftRight,
};

class WiiMouseProcessingModule {
public:
    static const int MAX_BUTTONS = 32;
protected:
    void copyFromPrev(const WiiMouseProcessingModule& prev) {
        history = prev.history;
        deltaT = prev.deltaT;
        std::copy(
            prev.pressedButtons, 
            prev.pressedButtons + MAX_BUTTONS, 
            pressedButtons
        );
        nValidIrSpots = prev.nValidIrSpots;
        std::copy(
            prev.trackingDots, 
            prev.trackingDots + 4, 
            trackingDots
        );
        accelVector = prev.accelVector;
    }
public:
    int deltaT; // milli seconds

    std::map<ProcessingOutputHistoryPoint, const WiiMouseProcessingModule*> history;

    NamespacedButtonState pressedButtons[MAX_BUTTONS];

    int nValidIrSpots;
    Vector3f trackingDots[4];
    Vector3f accelVector;

    bool isButtonPressed(ButtonNamespace ns, int buttonId) const {
        const auto searchFor = NamespacedButtonState(ns, buttonId, false);
        for (int i = 0; i < MAX_BUTTONS; i++) {
            if (!pressedButtons[i]) {
                break;
            }
            if (pressedButtons[i].matches(searchFor)) {
                return pressedButtons[i].state;
            }
        }
        return false;
    }

    virtual void process(const WiiMouseProcessingModule& prev) = 0;
};


class WMPDummy : public WiiMouseProcessingModule {
public:
    virtual void process(const WiiMouseProcessingModule& prev) override {
        copyFromPrev(prev);
    }
};

