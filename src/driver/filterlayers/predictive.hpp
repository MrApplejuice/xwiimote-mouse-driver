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

class WMPPredictiveDualIrTracking : public WiiMouseProcessingModule {
private:
    float lockedDistance;

    Vector3f XY_MEASURE_STD_NOISE;

    Vector3f left, right, center;
    float logLikelihoodLeft, logLikelihoodRight, logLikelihoodCenter;
public:
    virtual void process(const WiiMouseProcessingModule& prev) override;

    WMPPredictiveDualIrTracking() {
        XY_MEASURE_STD_NOISE = Vector3f(15.0f, 15.0f, 0);
        lockedDistance = -1;
    }
};
