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

#include "clustering.hpp"

void IrSpotClustering :: processIrSpots(const IRData* irSpots) {
    int noValid = 0;
    const IRData* validList[4];
    std::fill(validList, validList + 4, nullptr);
    for (int i = 0; i < 4; i++) {
        if (irSpots[i].valid) {
            validList[noValid] = irSpots + i;
            noValid++;
        }
    }

    Scalar distanceMatrix[4][4];
    for (int i = 0; i < noValid; i++) {
        for (int j = 0; j < noValid; j++) {
            if (i > j) {
                distanceMatrix[i][j] = distanceMatrix[j][i];
            } else if (i == j) {
                distanceMatrix[i][j] = 0;
            } else {
                distanceMatrix[i][j] = (validList[i]->point - validList[j]->point).len();
            }
        }
    }

    switch (noValid) {
        case 1:
            valid = true;
            rightPoint = leftPoint = validList[0]->point;
            break;
        case 2:
        case 3:
        case 4:
            {
                valid = true;
            
                // do a quick two-iterations k-means clustering
                Vector3 clusterPoints[2] = {
                    leftPoint.redivide(100),
                    rightPoint.redivide(100)
                };
                if (leftPoint == rightPoint) {
                    clusterPoints[1] = rightPoint + Vector3(1, 0, 0);
                }

                for (int _iter = 0; _iter < 2; _iter++) {
                    Vector3 clusterSums[2] = {Vector3(), Vector3()};
                    int clusterCounts[2] = {0, 0};

                    for (int i = 0; i < noValid; i++) {
                        int closestCluster = -1;
                        Scalar closestDistance = 0L;
                        for (int c = 0; c < 2; c++) {
                            Scalar d = (validList[i]->point - clusterPoints[c]).len();

                            if ((d < closestDistance) || (closestCluster < 0)) {
                                closestDistance = d;
                                closestCluster = c;
                            }
                        }

                        clusterSums[closestCluster] = clusterSums[closestCluster] + validList[i]->point;
                        clusterCounts[closestCluster]++;
                    }

                    for (int i = 0; i < 2; i++) {
                        if (clusterCounts[i] > 0) {
                            clusterPoints[i] = clusterSums[i] / clusterCounts[i];
                        }
                        clusterPoints[i] = clusterPoints[i].redivide(100);
                    }

                    // if one of the two clusters is empty, we assign a point
                    // from the filled cluster that is furthest away from the
                    // newly computed clusterPoint
                    if (clusterCounts[0] == 0) {
                        clusterCounts[0] = clusterCounts[1];
                        clusterPoints[0] = clusterPoints[1];
                        clusterCounts[1] = 0;
                    }
                    if (clusterCounts[1] == 0) {
                        Scalar maxDistance = 0;
                        int maxDistanceIndex = 0;
                        for (int i = 0; i < noValid; i++) {
                            Scalar d = (validList[i]->point - clusterPoints[0]).len();
                            if (d > maxDistance) {
                                maxDistance = d;
                                maxDistanceIndex = i;
                            }
                        }
                        clusterPoints[1] = validList[maxDistanceIndex]->point.redivide(100);
                    }
                }

                leftPoint = clusterPoints[0].undivide();
                rightPoint = clusterPoints[1].undivide();
            }
            break;
        default:
            valid = false;
            break;
    }
}

IrSpotClustering :: IrSpotClustering() : defaultDistance(300) {
    valid = false;
}
