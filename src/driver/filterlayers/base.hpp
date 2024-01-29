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

enum class ProcessingOutputHistoryPoint {
    Cluster = 0,
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

    std::map<ProcessingOutputHistoryPoint, WiiMouseProcessingModule*> history;

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

