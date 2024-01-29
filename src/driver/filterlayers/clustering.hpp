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

struct IRData {
    bool valid;
    Vector3 point;
};

static const IRData INVALID_IR = {
    false,
    Vector3()
};

class IrSpotClustering {
private:
public:
    bool valid;
    Vector3 leftPoint;
    Vector3 rightPoint;

    float defaultDistance;

    void processIrSpots(const IRData* irSpots);
    IrSpotClustering();
};

class WMPClustering : public WiiMouseProcessingModule {
private:
public:
    bool enablePointCollapse;
    IRData irData[4];
    IrSpotClustering irSpotClustering;

    WMPClustering() {
        std::fill(irData, irData + 4, INVALID_IR);
        irSpotClustering.defaultDistance = 100.0;
        enablePointCollapse = true;
    }

    void process(const WiiMouseProcessingModule& prev) {
        copyFromPrev(prev);
        history[ProcessingOutputHistoryPoint::Cluster] = this;

        for (int i = 0; i < 4; i++) {
            if (i >= nValidIrSpots) {
                irData[i] = INVALID_IR;
            } else {
                irData[i].valid = true;
                irData[i].point = trackingDots[i].toVector3(1);
            }
        }

        irSpotClustering.processIrSpots(irData);

        nValidIrSpots = irSpotClustering.valid ? 2 : 0;
        if (irSpotClustering.valid) {
            trackingDots[0] = irSpotClustering.leftPoint;
            trackingDots[1] = irSpotClustering.rightPoint;

            if (enablePointCollapse) {
                const float threshold = 0.5f * irSpotClustering.defaultDistance;
                if ((trackingDots[0] - trackingDots[1]).len() < threshold) {
                    nValidIrSpots = 1;
                    trackingDots[0] = (trackingDots[0] + trackingDots[1]) / 2.0f;
                }
            }
        }
    }
};
