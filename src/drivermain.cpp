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
#include <cmath>

#include <xwiimote.h>

#include "base.hpp"
#include "intlinalg.hpp"
#include "virtualmouse.hpp"
#include "controlsocket.hpp"
#include "device.hpp"
#include "settings.hpp"
#include "driveroptparse.hpp"

constexpr int64_t clamp(int64_t v, int64_t min, int64_t max) {
    return (v < min) ? min : ((v > max) ? max : v);
}

Scalar clamp(Scalar v, Scalar min, Scalar max) {
    return (v < min) ? min : ((v > max) ? max : v);
}

Vector3 clamp(Vector3 v, Scalar min, Scalar max) {
    return Vector3(
        clamp(v.values[0], min, max),
        clamp(v.values[1], min, max),
        clamp(v.values[2], min, max)
    );
}

Scalar min(Scalar a, Scalar b) {
    return (a < b) ? a : b;
}

Scalar max(Scalar a, Scalar b) {
    return (a > b) ? a : b;
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

class WiiMouseProcessingModule {
public:
    static const int MAX_BUTTONS = 128;
protected:
    void copyFromPrev(const WiiMouseProcessingModule& prev) {
        deltaT = prev.deltaT;
        std::copy(
            prev.pressedButtons, 
            prev.pressedButtons + MAX_BUTTONS, 
            pressedButtons
        );
        nValidIrSpots = prev.nValidIrSpots;
        std::copy(
            prev.trackingDots, 
            prev.trackingDots + 4, 
            trackingDots
        );
        accelVector = prev.accelVector;
    }
public:
    int deltaT; // milli seconds

    int pressedButtons[MAX_BUTTONS];

    int nValidIrSpots;
    Vector3 trackingDots[4];
    Vector3 accelVector;

    virtual void process(const WiiMouseProcessingModule& prev) = 0;
};

class WMPDummy : public WiiMouseProcessingModule {
public:
    virtual void process(const WiiMouseProcessingModule& prev) override {
        copyFromPrev(prev);
    }
};

class WMPSmoother : public WiiMouseProcessingModule {
private:
    int deltaTRemainder;

    Scalar positionMixFactor; // influence factor after 1 second
    Scalar accelMixFactor;  // influence factor after 1 second

    Scalar positionMixFactor10ms;
    Scalar accelMixFactor10ms;

    bool hasAccel;
    Vector3 lastAccel;

    bool hasPosition;
    Vector3 lastPositions[4];

    void update10msFactors() {
        {
            double f = (double) positionMixFactor.value / (double) positionMixFactor10ms.divisor;
            f = pow(f, 0.01);
            positionMixFactor10ms = Scalar((int64_t) (f * 1000000), 1000000);
        }

        {
            double f = (double) accelMixFactor.value / (double) accelMixFactor10ms.divisor;
            f = pow(f, 0.01);
            accelMixFactor10ms = Scalar((int64_t) (f * 1000000), 1000000);
        }
    }
public:
    Scalar getPositionMixFactor() const {
        return positionMixFactor;
    }

    Scalar getAccelMixFactor() const {
        return accelMixFactor;
    }

    void setPositionMixFactor(const Scalar& f) {
        positionMixFactor = f;
        update10msFactors();
    }

    void setAccelMixFactor(const Scalar& f) {
        accelMixFactor = f;
        update10msFactors();
    }

    virtual void process(const WiiMouseProcessingModule& prev) override {
        copyFromPrev(prev);

        if (prev.nValidIrSpots == 0) {
            hasPosition = false;
        }

        deltaTRemainder += prev.deltaT;
        while (deltaTRemainder > 10) {
            deltaTRemainder -= 10;

            if (hasPosition) {
                for (int i = 0; i < 4; i++) {
                    trackingDots[i] = lastPositions[i] = (
                        (trackingDots[i] * (-positionMixFactor10ms + 1)).redivide(100) + 
                        (lastPositions[i] * positionMixFactor10ms).redivide(100)
                    );
                }
            }

            if (hasAccel) {
                accelVector = lastAccel = (
                    (accelVector * (-accelMixFactor10ms + 1)).redivide(100) + 
                    (lastAccel * accelMixFactor10ms).redivide(100)
                );
            }
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

    WMPSmoother() : 
        positionMixFactor(1),
        accelMixFactor(1) 
    {
        deltaTRemainder = 0;
        hasAccel = false;
        hasPosition = false;
    }
};

class WiiMouse {
private:
    Xwiimote::Ptr wiimote;
    VirtualMouse vmouse;

    IRData irSpots[4];
    IrSpotClustering irSpotClustering;

    Vector3 calmatX;
    Vector3 calmatY;

    Vector3 screenAreaTopLeft;
    Vector3 screenAreaBottomRight;

    Vector3 wiimoteMouseMatX;
    Vector3 wiimoteMouseMatY;

    std::chrono::time_point<std::chrono::steady_clock> lastupdate;

    WMPDummy processingStart;
    WMPSmoother smoother;
    WMPDummy processingEnd;

    std::vector<WiiMouseProcessingModule*> processorSequence;

    void computeMouseMat() {
        Vector3 screenAreaSize = screenAreaBottomRight - screenAreaTopLeft;

        wiimoteMouseMatX = calmatX * (screenAreaSize.values[0] / 10000L);
        wiimoteMouseMatY = calmatY * (screenAreaSize.values[1] / 10000L);

        wiimoteMouseMatX.values[2] += screenAreaTopLeft.values[0];
        wiimoteMouseMatY.values[2] += screenAreaTopLeft.values[1];

        wiimoteMouseMatX = wiimoteMouseMatX.redivide(100);
        wiimoteMouseMatY = wiimoteMouseMatY.redivide(100);
    }

    void internalSetScreenArea(const Scalar& left, const Scalar& top, const Scalar& right, const Scalar& bottom) {
        screenAreaTopLeft = Vector3(
            clamp(min(left, right), 0, 10000),
            clamp(min(top, bottom), 0, 10000),
            0L
        );
        screenAreaBottomRight = Vector3(
            clamp(max(left, right), 0, 10000),
            clamp(max(top, bottom), 0, 10000),
            0L
        );
        computeMouseMat();
    }

    void runProcessing() {
        WiiMouseProcessingModule* prev = nullptr;
        for (WiiMouseProcessingModule* module : processorSequence) {
            if (prev) {
                module->process(*prev);
            }
            prev = module;
        }
    }
public:
    bool mouseEnabled;

    void setScreenArea(const Scalar& left, const Scalar& top, const Scalar& right, const Scalar& bottom) {
        internalSetScreenArea(left, top, right, bottom);
        std::cout << "Screen area set to " << screenAreaTopLeft << " and " << screenAreaBottomRight << std::endl;
    }

    void getScreenArea(Vector3& topLeft, Vector3& bottomRight) const {
        topLeft = screenAreaTopLeft;
        bottomRight = screenAreaBottomRight;
    }

    void getCalibrationVectors(Vector3& x, Vector3& y) const {
        x = calmatX;
        y = calmatY;
    }

    void setCalibrationVectors(const Vector3& x, const Vector3& y) {
        calmatX = x.redivide(100);
        calmatY = y.redivide(100);
        computeMouseMat();
        std::cout << "Calibration vectors set to " << calmatX << " and " << calmatY << std::endl;
    }

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

    const WiimoteButtonStates& getButtonStates() const {
        return wiimote->buttonStates;
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

        processingStart.accelVector = accelVector;
        processingStart.nValidIrSpots = irSpotClustering.valid ? 2 : 0;
        processingStart.trackingDots[0] = irSpotClustering.leftPoint;
        processingStart.trackingDots[1] = irSpotClustering.rightPoint;
        processingStart.deltaT = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - lastupdate
        ).count();
        runProcessing();

        if (mouseEnabled) {
            if (processingEnd.nValidIrSpots > 0) {
                Vector3 mid = Vector3();
                for (int i = 0; i < processingEnd.nValidIrSpots; i++) {
                    mid = mid + processingEnd.trackingDots[i];
                }
                mid = mid.undivide() / processingEnd.nValidIrSpots;
                mid.values[2] = 1;

                const Vector3 mouseCoord = Vector3(
                    clamp(
                        mid.dot(wiimoteMouseMatX),
                        screenAreaTopLeft.values[0],
                        screenAreaBottomRight.values[0]
                    ).undivide(),
                    clamp(
                        mid.dot(wiimoteMouseMatY),
                        screenAreaTopLeft.values[1],
                        screenAreaBottomRight.values[1]
                    ).undivide(),
                    0L
                );

                vmouse.move(
                    mouseCoord.values[0].value,
                    mouseCoord.values[1].value
                );
                vmouse.setButtonPressed(0, wiimote->buttonStates.a);
                vmouse.setButtonPressed(2, wiimote->buttonStates.b);
            } else {
                vmouse.setButtonPressed(0, false);
                vmouse.setButtonPressed(2, false);
            }
        }

        lastupdate = now;
    }

    WiiMouse(Xwiimote::Ptr wiimote) : wiimote(wiimote), vmouse(10001) {
        mouseEnabled = true;
        lastupdate = std::chrono::steady_clock::now();

        std::fill(irSpots, irSpots + 4, INVALID_IR);
        calmatX = Vector3(Scalar(-10000, 1024), 0, 10000).redivide(100);
        calmatY = Vector3(0, Scalar(10000, 1024), 0).redivide(100);

        internalSetScreenArea(
            0, 0, 10000, 10000
        );

        smoother.setPositionMixFactor(Scalar(90, 100));
        smoother.setAccelMixFactor(Scalar(75, 100));

        processorSequence.push_back(&processingStart);
        processorSequence.push_back(&smoother);
        processorSequence.push_back(&processingEnd);
    }
};

bool interuptMainLoop = false;

void signalHandler(int signum) {
    interuptMainLoop = true;
}

int main(int argc, char* argv[]) {
    OptionsMap options;
    try {
        options = parseOptions(argc, argv);
    }
    catch (const InvalidOptionException& e) {
        std::cerr << e.what() << std::endl;
        printHelp();
        return 1;
    }

    if (options.count("help")) {
        printHelp();
        return 0;
    }

    std::string configFilePath = options.defaultString("config-file", DEFAULT_CONFIG_PATH);
    std::cout << "Config file: " << configFilePath << std::endl;

    Config config(configFilePath);
    config.parseConfigFile();

    config.provideDefault("socket_address", DEFAULT_SOCKET_ADDR);
    std::string socketAddr = options.defaultString("socket-path", config.stringOptions["socket_address"]);

    std::shared_ptr<ControlSocket> socketref;
    try {
        socketref = std::shared_ptr<ControlSocket>(new ControlSocket(socketAddr));
    }
    catch (const SocketFailed& e) {
        std::cerr << "Failed to create socket: " << e.what() << std::endl;
        return 1;
    }
    ControlSocket& csocket = *socketref;
    std::cout << "Socket address: " << socketAddr << std::endl;

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
    {
        Vector3 defX, defY;
        wmouse.getCalibrationVectors(defX, defY);
        config.provideDefault("calmatX", vector3ToString(defX));
        config.provideDefault("calmatY", vector3ToString(defY));
        config.provideDefault(
            "screen_top_left",
            vector3ToString(Vector3(0, 0, 0))
        );
        config.provideDefault(
            "screen_bottom_right",
            vector3ToString(Vector3(10000, 10000, 0))
        );
    }
    wmouse.setCalibrationVectors(
        config.vectorOptions["calmatX"],
        config.vectorOptions["calmatY"]
    );
    wmouse.setScreenArea(
        config.vectorOptions["screen_top_left"].values[0],
        config.vectorOptions["screen_top_left"].values[1],
        config.vectorOptions["screen_bottom_right"].values[0],
        config.vectorOptions["screen_bottom_right"].values[1]
    );
    std::cout << "Mouse driver started!" << std::endl;
    signal(SIGINT, signalHandler);

    char irMessageBuffer[1024];
    WiimoteButtonStates buttonStates;
    while (!interuptMainLoop) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        wmouse.process();
        std::string eventResultBuffer;
        csocket.processEvents(
            [&wmouse, &config, &eventResultBuffer](const std::string& command, const std::vector<std::string>& parameters) {
                //std::cout << "Command: " << command << std::endl;
                //for (const std::string& p : parameters) {
                //    std::cout << "Parameter: " << p << std::endl;
                //}

                if (command == "mouse") {
                    if (parameters.size() == 1) {
                        if (parameters[0] == "on") {
                            wmouse.mouseEnabled = true;
                            return "OK";
                        } else if (parameters[0] == "off") {
                            wmouse.mouseEnabled = false;
                            return "OK";
                        }
                        return "ERROR:Invalid parameter";
                    }
                }
                if (command == "cal100") {
                    if (parameters.size() == 6) {
                        Vector3 x, y;
                        try {
                            x = Vector3(
                                Scalar(std::stoll(parameters[0]), 100),
                                Scalar(std::stoll(parameters[1]), 100),
                                Scalar(std::stoll(parameters[2]), 100)
                            );
                            y = Vector3(
                                Scalar(std::stoll(parameters[3]), 100),
                                Scalar(std::stoll(parameters[4]), 100),
                                Scalar(std::stoll(parameters[5]), 100)
                            );
                        } 
                        catch (std::invalid_argument& e) {
                            return "ERROR:Invalid parameter";
                        }
                        catch (std::out_of_range& e) {
                            return "ERROR:Invalid parameter";
                        }
                        wmouse.setCalibrationVectors(x, y);
                        config.vectorOptions["calmatX"] = x;
                        config.vectorOptions["calmatY"] = y;
                        config.writeConfigFile();
                        return "OK";
                    }
                }
                if (command == "getscreenarea100") {
                    Vector3 topLeft, bottomRight;
                    wmouse.getScreenArea(topLeft, bottomRight);
                    topLeft = topLeft.redivide(100);
                    bottomRight = bottomRight.redivide(100);

                    std::stringstream ss;
                    ss << "OK" 
                        << ":" << topLeft.values[0].value 
                        << ":" << topLeft.values[1].value 
                        << ":" << bottomRight.values[0].value
                        << ":" << bottomRight.values[1].value;
                    eventResultBuffer = ss.str();
                    return eventResultBuffer.c_str();
                }
                if (command == "screenarea100") {
                    if (parameters.size() == 4) {
                        Scalar top, left, bottom, right;
                        try {
                            left = Scalar(std::stoll(parameters[0]), 100);
                            top = Scalar(std::stoll(parameters[1]), 100);
                            right = Scalar(std::stoll(parameters[2]), 100);
                            bottom = Scalar(std::stoll(parameters[3]), 100);
                        } 
                        catch (std::invalid_argument& e) {
                            return "ERROR:Invalid parameter";
                        }
                        catch (std::out_of_range& e) {
                            return "ERROR:Invalid parameter";
                        } 
                        wmouse.setScreenArea(left, top, right, bottom);

                        Vector3 topLeft, bottomRight;
                        wmouse.getScreenArea(topLeft, bottomRight);
                        config.vectorOptions["screen_top_left"] = topLeft;
                        config.vectorOptions["screen_bottom_right"] = bottomRight;
                        config.writeConfigFile();
                        return "OK";
                    } else {
                        return "ERROR:Invalid parameter";
                    }
                }
                return "ERROR:Invalid command";
            }
        );

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
            csocket.broadcastMessage("lr:invalid\n");
        }

        if (buttonStates != wmouse.getButtonStates()) {
            buttonStates = wmouse.getButtonStates();
            snprintf(
                irMessageBuffer,
                1024,
                "b:%s\n",
                buttonStates.toMsgState().c_str()
            );
            csocket.broadcastMessage(irMessageBuffer);
        }
    }

    std::cout << "Mouse driver stopped!" << std::endl;

    return 0;
}
