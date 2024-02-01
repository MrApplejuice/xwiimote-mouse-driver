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


#include "smoother.hpp"

void WMPSmoother :: process(const WiiMouseProcessingModule& prev) {
    copyFromPrev(prev);

    if (prev.nValidIrSpots == 0) {
        hasPosition = false;
    }

    const bool buttonIsPressed = isButtonPressed(ButtonNamespace::VMOUSE, BTN_LEFT) 
        || isButtonPressed(ButtonNamespace::VMOUSE, BTN_RIGHT) 
        || isButtonPressed(ButtonNamespace::VMOUSE, BTN_MIDDLE);
    clickReleaseTimer = maxf(clickReleaseTimer - deltaT / 1000.0f, 0.0f);
    
    float posMix, accelMix;
    if (buttonIsPressed) {
        accelMix = pow(accelMixFactorClicked, deltaT / 1000.0f);
        if (!buttonWasPressed) {
            clickReleaseTimer = clickReleaseBlendDelay + clickReleaseFreezeDelay;
        }
        clickReleaseTimer = maxf(clickReleaseTimer, clickReleaseBlendDelay);
    } else {
        accelMix = pow(accelMixFactor, deltaT / 1000.0f);
        clickReleaseTimer = minf(clickReleaseTimer, clickReleaseBlendDelay);
    }
    buttonWasPressed = buttonIsPressed;

    if (clickReleaseTimer <= 0) {
        posMix = positionMixFactor;
    } else if (
        (clickReleaseFreezeDelay > 0) && 
        (clickReleaseTimer > clickReleaseBlendDelay)
    ) {
        posMix = 1;
    } else if (clickReleaseBlendDelay <= 0) {
        if (buttonIsPressed) {
            posMix = positionMixFactorClicked;
        } else {
            posMix = positionMixFactor;
        }
    } else {
        const float m = clickReleaseTimer / clickReleaseBlendDelay;
        posMix = positionMixFactor * (1.0f - m) + positionMixFactorClicked * m;
    }
    posMix = pow(posMix, deltaT / 1000.0f);

    if (hasPosition && enabled) {
        for (int i = 0; i < 4; i++) {
            trackingDots[i] = lastPositions[i] = (
                (trackingDots[i] * (1.0f - posMix)) + 
                (lastPositions[i] * posMix)
            );
        }
    }

    if (hasAccel && enabled) {
        accelVector = lastAccel = (
            (accelVector * (1.0f - accelMix)) +
            (lastAccel * accelMix)
        );
    }

    if (!hasAccel) {
        lastAccel = prev.accelVector;
        hasAccel = true;
    }
    if (!hasPosition) {
        if (prev.nValidIrSpots > 0) {
            std::copy(
                prev.trackingDots, 
                prev.trackingDots + 4, 
                lastPositions
            );
            hasPosition = true;
        }
    }
}
