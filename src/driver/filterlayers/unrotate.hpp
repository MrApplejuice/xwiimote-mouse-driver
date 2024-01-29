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

class WMPUnrotate : public WiiMouseProcessingModule {
private:
    void doUnrotate(const Vector3f& unrotateX, const Vector3f& unrotateY) {
        const static Vector3f HALF_RES(
            WIIMOTE_IR_SENSOR_EXTENTS.width / 2.0f,
            WIIMOTE_IR_SENSOR_EXTENTS.height / 2.0f,
            0
        );

        for (int i = 0; i < nValidIrSpots; i++) {
            Vector3f dot = trackingDots[i] - HALF_RES;
            dot = Vector3f(
                dot.dot(unrotateX),
                dot.dot(unrotateY),
                0
            );
            trackingDots[i] = dot + HALF_RES;
        }
    }

    void unrotateUsingAccel() {
        // Takes the acceleration vector and unrotates the tracking dots
        // to compensate for the rotation of the wiimote.
        Vector3f normAccel = accelVector;
        normAccel[1] = 0;
        if (normAccel.len() <= 0.01) {
            return;
        }
        normAccel = normAccel / accelVector.len();

        const Vector3f unrotateX(normAccel[2], normAccel[0], 0);
        const Vector3f unrotateY(-normAccel[0], normAccel[2], 0);
        doUnrotate(unrotateX, unrotateY);
    }

    void unrotateUsingDualPoint() {
        // Takes the two tracking dots and unrotates the tracking dots
        // to compensate for the rotation of the wiimote.
        if (nValidIrSpots != 2) {
            return;
        }

        Vector3f horizontal = trackingDots[1] - trackingDots[0];
        if (horizontal.len() <= 0.01) {
            return;
        }
        horizontal = horizontal / horizontal.len();

        const Vector3f unrotateX(horizontal[0], horizontal[1], 0);
        const Vector3f unrotateY(-horizontal[1], horizontal[0], 0);        
        doUnrotate(unrotateX, unrotateY);
    }

    void assignLeftRight() {
        if (nValidIrSpots != 2) {
            return;
        }

        if (trackingDots[1][0] < trackingDots[0][0]) {
            std::swap(trackingDots[0], trackingDots[1]);
        }
    }
public:
    virtual void process(const WiiMouseProcessingModule& prev) override {
        copyFromPrev(prev);

        unrotateUsingAccel();
        assignLeftRight();
        unrotateUsingDualPoint();
        assignLeftRight();
    }
};
