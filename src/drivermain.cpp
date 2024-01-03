#include <algorithm>
#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include <exception>

#include <csignal>

#include <xwiimote.h>

#include "base.hpp"
#include "intlinalg.hpp"
#include "virtualmouse.hpp"
#include "controlsocket.hpp"
#include "device.hpp"


constexpr int64_t clamp(int64_t v, int64_t min, int64_t max) {
    return (v < min) ? min : ((v > max) ? max : v);
}

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

    Scalar defaultDistance;

    void processIrSpots(const IRData* irSpots) {
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

                    if (clusterPoints[0].values[0].value < clusterPoints[1].values[0].value) {
                        leftPoint = clusterPoints[0].undivide();
                        rightPoint = clusterPoints[1].undivide();
                    } else {
                        leftPoint = clusterPoints[1].undivide();
                        rightPoint = clusterPoints[0].undivide();
                    }
                }
                break;
            default:
                valid = false;
                break;
        }
    }

    IrSpotClustering() : defaultDistance(300) {
        valid = false;
    }
};

class WiiMouse {
private:
    Xwiimote::Ptr wiimote;
    VirtualMouse vmouse;

    Vector3 smoothCoord;

    IRData irSpots[4];
    IrSpotClustering irSpotClustering;

    std::chrono::time_point<std::chrono::steady_clock> lastupdate;
public:
    Scalar getIrSpotDistance() const {
        return irSpotClustering.defaultDistance;
    }

    void setIrSpotDistance(int distance) {
        irSpotClustering.defaultDistance = distance;
    }

    bool hasValidLeftRight() const {
        return irSpotClustering.valid;
    }

    Vector3 getLeftPoint() const {
        return irSpotClustering.leftPoint.undivide();
    }

    Vector3 getRightPoint() const {
        return irSpotClustering.rightPoint.undivide();
    }

    IRData getIrSpot(int i) const {
        if ((i < 0) || (i >= 4)) {
            return INVALID_IR;
        }
        return irSpots[i];
    }

    void process() {
        std::chrono::time_point<std::chrono::steady_clock> now = 
            std::chrono::steady_clock::now();

        wiimote->poll();

        Vector3 accelVector = Vector3(wiimote->accelX, wiimote->accelY, wiimote->accelZ);
        if (accelVector.len() > 0) {
            accelVector = accelVector / accelVector.len();

            accelVector.redivide(10000);

            Vector3 newCoord(
                clamp(accelVector.values[0].value, -5000, 5000), 
                clamp(accelVector.values[1].value, -5000, 5000), 
                0
            );

            if (smoothCoord.len().value == 0L) {
                smoothCoord = newCoord.redivide(100);
            } else {
                smoothCoord = (
                    smoothCoord * Scalar(90, 100) + 
                    newCoord * Scalar(10, 100)
                ).redivide(100);
            }

            Vector3 c = smoothCoord.redivide(1);
            vmouse.move(c.values[0].value + 5000, c.values[1].value + 5000);
        }

        {
            IRData r;
            for (int i = 0; i < 4; i++) {
                r.point = Vector3(
                    wiimote->irdata[i].x, wiimote->irdata[i].y, 0
                );
                r.valid = xwii_event_ir_is_valid(
                    &(wiimote->irdata[i])
                ) && (r.point.len() > 0);
                irSpots[i] = r;
            }

            irSpotClustering.processIrSpots(irSpots);
        }

        lastupdate = now;
    }

    WiiMouse(Xwiimote::Ptr wiimote) : wiimote(wiimote), vmouse(10001) {
        lastupdate = std::chrono::steady_clock::now();

        std::fill(irSpots, irSpots + 4, INVALID_IR);
    }
};

bool interuptMainLoop = false;

void signalHandler(int signum) {
    interuptMainLoop = true;
}

int main() {
    XwiimoteMonitor monitor;

    monitor.poll();
    if (monitor.count() <= 0) {
        std::cout << "No Wiimote found. Please pair a new Wiimote now." << std::endl;
        while (monitor.count() <= 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            monitor.poll();
        }
    } 

    WiiMouse wmouse(monitor.get_device(0));
    ControlSocket csocket;
    std::cout << "Mouse driver started!" << std::endl;
    signal(SIGINT, signalHandler);

    char irMessageBuffer[1024];
    while (!interuptMainLoop) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        wmouse.process();

        // Send raw ir data
        for (int i = 0; i < 4; i++) {
            IRData d = wmouse.getIrSpot(i);
            snprintf(
                irMessageBuffer,
                1024,
                "ir:%i:%i:%i:%i\n",
                i,
                (int) d.valid,
                (int) d.point.values[0].undivide().value,
                (int) d.point.values[1].undivide().value
            );
            csocket.broadcastMessage(irMessageBuffer);
        }

        // Send clustered left/right data
        if (wmouse.hasValidLeftRight()) {
            snprintf(
                irMessageBuffer,
                1024,
                "lr:%i:%i:%i:%i\n",
                (int) wmouse.getLeftPoint().values[0].value,
                (int) wmouse.getLeftPoint().values[1].value,
                (int) wmouse.getRightPoint().values[0].value,
                (int) wmouse.getRightPoint().values[1].value
            );
            csocket.broadcastMessage(irMessageBuffer);
        } else {
            csocket.broadcastMessage("ir:invalid\n");
        }
        
    }

    std::cout << "Mouse driver stopped!" << std::endl;

    return 0;
}
