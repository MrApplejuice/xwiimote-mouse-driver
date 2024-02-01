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

#include "base.hpp"

class WMPSmoother : public WiiMouseProcessingModule {
private:
    bool hasAccel;
    Vector3f lastAccel;

    bool hasPosition;
    Vector3f lastPositions[4];

    bool buttonWasPressed;
    float clickReleaseTimer;
public:
    bool enabled;

    // influence of old values after 1 second
    float positionMixFactor; 
    float accelMixFactor; 
    float positionMixFactorClicked;
    float accelMixFactorClicked;
    float clickReleaseBlendDelay;
    float clickReleaseFreezeDelay;

    virtual void process(const WiiMouseProcessingModule& prev) override;

    WMPSmoother() {
        enabled = true;
        hasAccel = false;
        hasPosition = false;

        buttonWasPressed = false;
        clickReleaseTimer = 0.0f;

        accelMixFactor = 0.0f;
        accelMixFactorClicked = 0.0f;

        positionMixFactor = 0.00001f;
        positionMixFactorClicked = 0.1f;
        clickReleaseBlendDelay = 0.25f;
        clickReleaseFreezeDelay = 0.1f;
    }
};
