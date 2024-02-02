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

#include "predictive.hpp"

static float logNormal2d(const Vector3f& point, const Vector3f& pointStd) {
    const float sqrt2pi = 2.5066282746310002f;
    const float x = point[0] / pointStd[0];
    const float y = point[1] / pointStd[1];
    return (-0.5f * (x * x + y * y)) - (sqrt2pi + pointStd[0] + pointStd[1]);
}

void WMPPredictiveDualIrTracking :: process(const WiiMouseProcessingModule& prev) {
    copyFromPrev(prev);

    const WiiMouseProcessingModule& irData = *history[ProcessingOutputHistoryPoint::Cluster];
    int clusterNValidIrSpots = irData.nValidIrSpots;
    if (clusterNValidIrSpots == 2) {
        if (irData.trackingDots[0] == irData.trackingDots[1]) {
            clusterNValidIrSpots = 1;
        }
    }

    if (clusterNValidIrSpots == 2) {
        left = trackingDots[0];
        right = trackingDots[1];
        center = (trackingDots[0] + trackingDots[1]) / 2.0f;
        logLikelihoodLeft = logLikelihoodRight = logLikelihoodCenter = 0.0f;
        lockedDistance = (left - right).len();
    } else if (clusterNValidIrSpots == 1) {
        if (lockedDistance < 0) {
            return;
        } 

        Vector3f newPoint = (trackingDots[0] + trackingDots[1]) / 2.0f;

        logLikelihoodLeft += logNormal2d(newPoint - left, XY_MEASURE_STD_NOISE);
        logLikelihoodRight += logNormal2d(newPoint - right, XY_MEASURE_STD_NOISE);
        logLikelihoodCenter += logNormal2d(newPoint - center, XY_MEASURE_STD_NOISE);

        {
            const float likelihoodMax = maxf(
                logLikelihoodLeft, 
                logLikelihoodRight,
                logLikelihoodCenter
            );
            logLikelihoodLeft = maxf(logLikelihoodLeft - likelihoodMax, -100000.0f);
            logLikelihoodRight = maxf(logLikelihoodRight - likelihoodMax, -100000.0f);
            logLikelihoodCenter = maxf(logLikelihoodCenter - likelihoodMax, -100000.0f);
        }

        const float normalization = exp(logLikelihoodLeft) 
            + exp(logLikelihoodRight) 
            + exp(logLikelihoodCenter);
        Vector3f predPoint = (
            left * exp(logLikelihoodLeft)
            + right * exp(logLikelihoodRight)
            + center * exp(logLikelihoodCenter)
        ) / normalization;
        
        const Vector3f offset = newPoint - predPoint;
        left = left + offset;
        right = right + offset;
        center = center + offset;
        predPoint = predPoint + offset;

        nValidIrSpots = 2;
        if (logLikelihoodLeft >= 0.0f) {
            trackingDots[0] = predPoint;
            trackingDots[1] = predPoint + Vector3f(lockedDistance, 0, 0);
        } else if (logLikelihoodRight >= 0.0f) {
            trackingDots[0] = predPoint - Vector3f(lockedDistance, 0, 0);
            trackingDots[1] = predPoint;
        } else {
            trackingDots[0] = predPoint - Vector3f(lockedDistance / 2.0f, 0, 0);
            trackingDots[1] = predPoint + Vector3f(lockedDistance / 2.0f, 0, 0);
        }

        left = trackingDots[0];
        right = trackingDots[1];
        center = (trackingDots[0] + trackingDots[1]) / 2.0f;
    } else {
        lockedDistance = -1;
    }
}
