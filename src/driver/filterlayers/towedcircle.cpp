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

#include "towedcircle.hpp"

void WMPTowedCircle :: process(const WiiMouseProcessingModule& prev) {
    copyFromPrev(prev);
    history[ProcessingOutputHistoryPoint::LastLeftRight] = &prev;

    if ((nValidIrSpots <= 0) || (radius <= 0)) {
        validCircle = false;
        return;
    }
    
    Vector3f center;
    for (int i = 0; i < nValidIrSpots; i++) {
        center += trackingDots[i];
    }
    center /= nValidIrSpots;

    if (!validCircle) {
        circleCenter = center;
    } else {
        Vector3f delta = center - circleCenter;
        delta[1] *= aspectRatio;
        
        float diff = delta.len() - radius * 1024.0f;
        if (diff > 0.0) {
            delta = delta / delta.len() * diff;
            delta[1] /= aspectRatio;
            circleCenter += delta;
        }
    }

    nValidIrSpots = 1;
    trackingDots[0] = circleCenter;
    validCircle = true;
}
