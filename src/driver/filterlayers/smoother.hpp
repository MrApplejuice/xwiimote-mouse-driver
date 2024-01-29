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

    virtual void process(const WiiMouseProcessingModule& prev) override {
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
