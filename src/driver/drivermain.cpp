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
#include "floatlinalg.hpp"
#include "virtualmouse.hpp"
#include "controlsocket.hpp"
#include "device.hpp"
#include "settings.hpp"
#include "driveroptparse.hpp"
#include "driverextra.hpp"

#include "filterlayers/base.hpp"
#include "filterlayers/buttons.hpp"
#include "filterlayers/smoother.hpp"
#include "filterlayers/unrotate.hpp"
#include "filterlayers/predictive.hpp"
#include "filterlayers/clustering.hpp"



class WiiMouse {
private:
    Xwiimote::Ptr wiimote;
    VirtualMouse vmouse;

    Vector3 calmatX;
    Vector3 calmatY;

    Vector3f screenAreaTopLeft;
    Vector3f screenAreaBottomRight;

    Vector3 wiimoteMouseMatX;
    Vector3 wiimoteMouseMatY;

    std::chrono::time_point<std::chrono::steady_clock> lastupdate;

    WMPDummy processingStart;
    WMPClustering clustering;
    WMPButtonMapper buttonMapper;
    WMPSmoother smoother;
    WMPUnrotate unrotate;
    WMPPredictiveDualIrTracking predictiveDualIrTracking;
    WMPDummy processingEnd;

    std::vector<WiiMouseProcessingModule*> processorSequence;

    void computeMouseMat() {
        Vector3f screenAreaSize = screenAreaBottomRight - screenAreaTopLeft;

        wiimoteMouseMatX = calmatX * Scalar(screenAreaSize.values[0] / 10000L, 1000000);
        wiimoteMouseMatY = calmatY * Scalar(screenAreaSize.values[1] / 10000L, 1000000);

        wiimoteMouseMatX.values[2] += screenAreaTopLeft.values[0];
        wiimoteMouseMatY.values[2] += screenAreaTopLeft.values[1];

        wiimoteMouseMatX = wiimoteMouseMatX.redivide(100);
        wiimoteMouseMatY = wiimoteMouseMatY.redivide(100);
    }

    void internalSetScreenArea(
        const Scalar& left, const Scalar& top, const Scalar& right, const Scalar& bottom
    ) {
        screenAreaTopLeft = Vector3f(
            clamp(min(left, right).toFloat(), 0, 10000),
            clamp(min(top, bottom).toFloat(), 0, 10000),
            0L
        );
        screenAreaBottomRight = Vector3f(
            clamp(max(left, right).toFloat(), 0, 10000),
            clamp(max(top, bottom).toFloat(), 0, 10000),
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

    void getScreenArea(Vector3f& topLeft, Vector3f& bottomRight) const {
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

    bool hasValidLeftRight() const {
        return clustering.irSpotClustering.valid;
    }

    Vector3 getClusteringLeftPoint() const {
        return clustering.irSpotClustering.leftPoint.undivide();
    }

    Vector3 getClusteringRightPoint() const {
        return clustering.irSpotClustering.rightPoint.undivide();
    }

    float getClusteringDefaultDistance() const {
        return clustering.irSpotClustering.defaultDistance;
    }

    void setClusteringDefaultDistance(float distance) {
        clustering.irSpotClustering.defaultDistance = distance;
    }

    void setCalibrationMode(bool on) {
        clustering.enablePointCollapse = !on;
        smoother.enabled = !on;
    }

    void getSmoothingFactors(
        float& smoothingClicked, 
        float& smoothingReleased,
        float& clickFreeze
    ) const {
        smoothingClicked = smoother.positionMixFactorClicked;
        smoothingReleased = smoother.positionMixFactor;
        clickFreeze = smoother.clickReleaseFreezeDelay;
    }

    void setSmoothingFactors(
        float smoothingClicked, 
        float smoothingReleased,
        float clickFreeze
    ) {
        smoother.positionMixFactorClicked = smoothingClicked;
        smoother.positionMixFactor = smoothingReleased;
        smoother.clickReleaseFreezeDelay = clickFreeze;
    }

    Vector3 getFilteredLeftPoint() const {
        if (processingEnd.nValidIrSpots == 0) {
            return Vector3(0, 0, 0);
        }
        return processingEnd.trackingDots[0].toVector3(1);
    }

    Vector3 getFilteredRightPoint() const {
        if (processingEnd.nValidIrSpots == 0) {
            return Vector3(0, 0, 0);
        }
        if (processingEnd.nValidIrSpots == 1) {
            return getFilteredLeftPoint();
        }
        return processingEnd.trackingDots[1].toVector3(1);
    }

    std::map<WiimoteButtonMappingState, std::string> getButtonMap() const {
        return buttonMapper.getStringMappings();
    }

    void clearButtonMap() {
        buttonMapper.clearMapping();
    }

    void mapButton(WiimoteButton button, bool ir, const SupportedButton* evdevButton) {
        if (!evdevButton) {
            buttonMapper.clearButtonAssignments(button, ir);
        } else {
            buttonMapper.addMapping(button, ir, evdevButton->code);
        }
    }

    const WiimoteButtonStates& getButtonStates() const {
        return wiimote->buttonStates;
    }

    IRData getIrSpot(int i) const {
        if ((i < 0) || (i >= 4)) {
            return INVALID_IR;
        }
        return clustering.irData[i];
    }

    void process() {
        std::chrono::time_point<std::chrono::steady_clock> now = 
            std::chrono::steady_clock::now();

        wiimote->poll();

        Vector3 accelVector = Vector3(wiimote->accelX, wiimote->accelY, wiimote->accelZ);

        {
            processingStart.nValidIrSpots = 0;
            for (int i = 0; i < 4; i++) {
                Vector3f point(
                    wiimote->irdata[i].x, wiimote->irdata[i].y, 0
                );
                bool valid = xwii_event_ir_is_valid(
                    &(wiimote->irdata[i])
                ) && (point.len() > 0);

                if (valid) {
                    processingStart.trackingDots[processingStart.nValidIrSpots] = point;
                    processingStart.nValidIrSpots++;
                }
            }
        }

        processingStart.deltaT = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - lastupdate
        ).count();
        processingStart.accelVector = accelVector;
        {
            for (int bi = 0; bi < (int) WiimoteButton::COUNT; bi++) {
                processingStart.pressedButtons[bi] = NamespacedButtonState(
                    ButtonNamespace::WII, bi, wiimote->buttonStates.pressedButtons[bi]
                );
            }
        }
        processingStart.history[ProcessingOutputHistoryPoint::Cluster] = &processingStart;
        runProcessing();

        for (auto button : processingEnd.pressedButtons) {
            if (!button) {
                break;
            }
            if (button.ns == ButtonNamespace::VMOUSE) {
                vmouse.button(button.buttonId, button.state && mouseEnabled);
            }
        }
        if (mouseEnabled) {
            if (processingEnd.nValidIrSpots > 0) {
                Vector3f mid;
                for (int i = 0; i < processingEnd.nValidIrSpots; i++) {
                    mid = mid + processingEnd.trackingDots[i];
                }
                mid = mid / processingEnd.nValidIrSpots;
                mid.values[2] = 1.0f;

                const Vector3 mouseCoord = Vector3(
                    clamp(
                        mid.dot(wiimoteMouseMatX),
                        screenAreaTopLeft.values[0],
                        screenAreaBottomRight.values[0]
                    ),
                    clamp(
                        mid.dot(wiimoteMouseMatY),
                        screenAreaTopLeft.values[1],
                        screenAreaBottomRight.values[1]
                    ),
                    0L
                );

                vmouse.move(
                    mouseCoord.values[0].value,
                    mouseCoord.values[1].value
                );
            } 
        }

        lastupdate = now;
    }

    WiiMouse(Xwiimote::Ptr wiimote) : wiimote(wiimote), vmouse(10001) {
        mouseEnabled = true;
        lastupdate = std::chrono::steady_clock::now();

        calmatX = Vector3(Scalar(-10000, 1024), 0, 10000).redivide(100);
        calmatY = Vector3(0, Scalar(10000, 1024), 0).redivide(100);

        internalSetScreenArea(
            0, 0, 10000, 10000
        );

        buttonMapper.addMapping(WiimoteButton::A, true, BTN_LEFT);
        buttonMapper.addMapping(WiimoteButton::B, true, BTN_RIGHT);

        processorSequence.push_back(&processingStart);
        processorSequence.push_back(&clustering);
        processorSequence.push_back(&buttonMapper);
        processorSequence.push_back(&unrotate);
        processorSequence.push_back(&predictiveDualIrTracking);
        processorSequence.push_back(&smoother);
        processorSequence.push_back(&processingEnd);
    }
};

bool interuptMainLoop = false;

void signalHandler(int signum) {
    interuptMainLoop = true;
}

WiimoteButton configButtonNameToWiimote(const std::string& name) {
    static std::map<std::string, WiimoteButton> LOWERCASE_READABLE_NAMES;
    if (LOWERCASE_READABLE_NAMES.size() == 0) {
        for (auto& pair : WIIMOTE_BUTTON_READABLE_NAMES) {
            LOWERCASE_READABLE_NAMES[asciiLower(pair.second)] = pair.first;
        }
    }

    WiimoteButton wiiButton = rmap(
        WIIMOTE_BUTTON_NAMES, name, WiimoteButton::INVALID
    );
    if (wiiButton == WiimoteButton::INVALID) {
        try {
            wiiButton = LOWERCASE_READABLE_NAMES.at(name);
        }
        catch (std::out_of_range& e) {}
    }
    return wiiButton;
}

Vector3 getConfSmootingVector(WiiMouse& wmouse) {
    float smoothingClicked, smoothingReleased, clickFreeze;
    wmouse.getSmoothingFactors(
        smoothingClicked, smoothingReleased, clickFreeze
    );
    smoothingClicked = log10(smoothingClicked);
    smoothingReleased = log10(smoothingReleased);

    Scalar smoothingClickedS(smoothingClicked, 100);
    Scalar smoothingReleasedS(smoothingReleased, 100);
    Scalar clickFreezeS(clickFreeze, 100000);

    return Vector3(
        smoothingClickedS, smoothingReleasedS, clickFreezeS
    ).redivide(100000);
}

void applyDeviceConfigurations(WiiMouse& wmouse, Config& config) {
    static bool AppliedDefaults = false;
    if (!AppliedDefaults) {
        Vector3 defX, defY;
        wmouse.getCalibrationVectors(defX, defY);
        config.provideDefault("calmatx", vector3ToString(defX));
        config.provideDefault("calmaty", vector3ToString(defY));
        config.provideDefault(
            "screen_top_left",
            vector3ToString(Vector3(0, 0, 0))
        );
        config.provideDefault(
            "screen_bottom_right",
            vector3ToString(Vector3(10000, 10000, 0))
        );
        auto keyMappings = wmouse.getButtonMap();
        for (auto& mapping : keyMappings) {
            config.provideDefault(
                mapping.first.toConfigurationKey(),
                mapping.second
            );
        }
        config.provideDefault(
            "default_ir_distance",
            std::to_string((int64_t) (wmouse.getClusteringDefaultDistance() * 100))
        );

        config.provideDefault(
            "smoothing_clicked_released_delay",
            vector3ToString(getConfSmootingVector(wmouse))
        );
        AppliedDefaults = true;
    }

    wmouse.setCalibrationVectors(
        config.vectorOptions["calmatx"],
        config.vectorOptions["calmaty"]
    );
    wmouse.setScreenArea(
        config.vectorOptions["screen_top_left"].values[0],
        config.vectorOptions["screen_top_left"].values[1],
        config.vectorOptions["screen_bottom_right"].values[0],
        config.vectorOptions["screen_bottom_right"].values[1]
    );

    Vector3f smoothingVector = config.vectorOptions["smoothing_clicked_released_delay"];
    wmouse.setSmoothingFactors(
        pow(10, smoothingVector.values[0]),
        pow(10, smoothingVector.values[1]),
        smoothingVector.values[2]
    );

    try {
        int64_t defaultIrDistance = std::stoll(config.stringOptions["default_ir_distance"]);
        wmouse.setClusteringDefaultDistance(defaultIrDistance / 100.0f);
    }
    catch (std::exception& e) {}

    wmouse.clearButtonMap();
    static const std::vector<std::string> ONOFFSCREEN_SUFFIXES = {
        "",
        "_offscreen"
    };
    for (auto btnName : WIIMOTE_BUTTON_READABLE_NAMES) {
        for (auto suffix : ONOFFSCREEN_SUFFIXES) {
            std::string wiiConfName = asciiLower("button_" + btnName.second + suffix);
            auto foundConf = config.stringOptions.find(wiiConfName);
            if (foundConf == config.stringOptions.end()) {
                continue;
            }

            const bool ir = suffix == ONOFFSCREEN_SUFFIXES[0];

            const std::string& keyName = foundConf->second;
            const SupportedButton* btn = findButtonByName(keyName);
            if ((!btn) && (keyName != "")) {
                std::cerr 
                    << "Ignoring invalid mapped button: " 
                    << keyName << std::endl;
                continue;
            }

            wmouse.mapButton(btnName.first, ir, btn);
        }
    }

}

int main(int argc, char* argv[]) {
    signal(SIGINT, signalHandler);

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

    if (options.count("version")) {
        printVersion();
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

    while (!interuptMainLoop) {
        Xwiimote::Ptr wiimote;
        {
            XwiimoteMonitor monitor;
            monitor.poll();
            if (monitor.count() <= 0) {
                std::cout << "No Wiimote found. Please pair a new Wiimote now." << std::endl;
                while ((monitor.count() <= 0) && (!interuptMainLoop)) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    monitor.poll();
                }
            }
            if (interuptMainLoop) {
                break;
            }
            wiimote = monitor.get_device(0);
        }

        WiiMouse wmouse(wiimote);
        applyDeviceConfigurations(wmouse, config);

        std::cout << "Wiimote detected. (Re-)starting mouse driver" << std::endl;

        char irMessageBuffer[1024];
        WiimoteButtonStates buttonStates;
        while (!interuptMainLoop) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            try {
                wmouse.process();
            }
            catch (const DevDisappeared& e) {
                std::cout << "Wiimote disconnected." << std::endl;
                break;
            }
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
                            config.vectorOptions["calmatx"] = x;
                            config.vectorOptions["calmaty"] = y;
                            config.writeConfigFile();
                            return "OK";
                        }
                    }
                    if (command == "getscreenarea100") {
                        Vector3f topLeftF, bottomRightF;
                        wmouse.getScreenArea(topLeftF, bottomRightF);

                        Vector3 topLeft = topLeftF.toVector3(100);
                        Vector3 bottomRight = bottomRightF.toVector3(100);

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

                            Vector3f topLeftF, bottomRightF;
                            wmouse.getScreenArea(topLeftF, bottomRightF);
                            config.vectorOptions["screen_top_left"] 
                                = topLeftF.toVector3(1000);
                            config.vectorOptions["screen_bottom_right"] 
                                = bottomRightF.toVector3(1000);
                            config.writeConfigFile();
                            return "OK";
                        } else {
                            return "ERROR:Invalid parameter";
                        }
                    }
                    if (command == "keycount") {
                        std::stringstream ss;
                        ss << "OK:" << SUPPORTED_BUTTONS.size();
                        eventResultBuffer = ss.str();
                        return eventResultBuffer.c_str();
                    }
                    if (command == "keyget") {
                        if (parameters.size() != 1) {
                            return "ERROR:Single key index expected";
                        }
                        int index;
                        try {
                            index = std::stoi(parameters[0]);
                        }
                        catch (std::invalid_argument& e) {
                            return "ERROR:Invalid index";
                        }
                        catch (std::out_of_range& e) {
                            return "ERROR:Invalid index";
                        }

                        if ((index < 0) || (index >= (int) SUPPORTED_BUTTONS.size())) {
                            return "ERROR:Out of bounds";
                        }

                        auto key = SUPPORTED_BUTTONS[index];

                        std::stringstream ss;
                        ss << "OK:" 
                            << key.rawKeyName 
                            << ":" << (key.name ? key.name : "")
                            << ":" << key.category;
                        eventResultBuffer = ss.str();
                        return eventResultBuffer.c_str();
                    }
                    if (command == "keymapget") {
                        auto keymap = wmouse.getButtonMap();
                        std::stringstream ss;
                        ss << "OK:";
                        for (auto& mapping : keymap) {
                            ss << mapping.first.toProtocolString()
                                << ":" << mapping.second << ":";
                        }
                        eventResultBuffer = ss.str();
                        eventResultBuffer.pop_back(); // remove trailing ':'
                        return eventResultBuffer.c_str();
                    }
                    if (command == "bindkey") {
                        if (parameters.size() != 3) {
                            return "ERROR:Invalid parameter count";
                        }

                        WiimoteButton wiiButton = configButtonNameToWiimote(
                            parameters[0]
                        );
                        if (wiiButton == WiimoteButton::INVALID) {
                            return "ERROR:Invalid wii button";
                        }

                        bool ir;
                        if (parameters[1] == "0") {
                            ir = false;
                        } else if (parameters[1] == "1") {
                            ir = true;
                        } else {
                            return "ERROR:Invalid ir value";
                        }

                        std::string keyName = trim(parameters[2]);
                        const SupportedButton* key = findButtonByName(keyName);
                        if ((!key) && (keyName != "")) {
                            return "ERROR:Invalid key binding";
                        }

                        wmouse.mapButton(wiiButton, ir, key);

                        const WiimoteButtonMappingState state = {wiiButton, ir};
                        config.stringOptions[state.toConfigurationKey()] = keyName;
                        config.writeConfigFile();
                        return "OK";
                    }
                    if (command == "irdist100") {
                        if (parameters.size() != 1) {
                            return "ERROR:Invalid parameter count";
                        }
                        int64_t distance;
                        try {
                            distance = std::stoll(parameters[0]);
                        }
                        catch (std::exception& e) {
                            return "ERROR:Invalid parameter";
                        }
                        if (distance < 0) {
                            return "ERROR:Invalid parameter";
                        }
                        wmouse.setClusteringDefaultDistance(distance / 100.0f);
                        config.stringOptions["default_ir_distance"] = parameters[0];
                        config.writeConfigFile();
                        return "OK";
                    }
                    if (command == "calibration") {
                        if (parameters.size() != 1) {
                            return "ERROR:Invalid parameter count";
                        }
                        wmouse.setCalibrationMode(parameters[0] == "on");
                    }
                    if (command == "getsmoothing100") {
                        Vector3 sVec = getConfSmootingVector(wmouse);

                        std::stringstream ss;
                        ss << "OK:" 
                            << sVec.values[0].redivide(100).value
                            << ":" << sVec.values[1].redivide(100).value
                            << ":" << sVec.values[2].redivide(100000).value;
                        eventResultBuffer = ss.str();
                        return eventResultBuffer.c_str();
                    }
                    if (command == "setsmoothing100") {
                        if (parameters.size() != 3) {
                            return "ERROR:Invalid parameter count";
                        }
                        Scalar smoothingClicked, smoothingReleased, clickFreeze;
                        try {
                            smoothingClicked = Scalar(
                                std::stoll(parameters[0]), 100
                            );
                            smoothingReleased = Scalar(
                                std::stoll(parameters[1]), 100
                            );
                            clickFreeze = Scalar(
                                std::stoll(parameters[2]), 100000
                            );
                        }
                        catch (std::exception& e) {
                            return "ERROR:Invalid parameter";
                        }

                        if (clickFreeze < 0) {
                            return "ERROR:Click freeze negative";
                        }
                        if ((smoothingClicked > 0) || (smoothingReleased > 0)) {
                            return "ERROR:Log smoothing factors larger than 0";
                        }

                        wmouse.setSmoothingFactors(
                            pow(10.0f, smoothingClicked.toFloat()),
                            pow(10.0f, smoothingReleased.toFloat()),
                            clickFreeze.toFloat()
                        );
                        config.vectorOptions[
                            "smoothing_clicked_released_delay"
                        ] = Vector3(
                            smoothingClicked, smoothingReleased, clickFreeze
                        ).redivide(100000);
                        config.writeConfigFile();
                        return "OK";
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
                const Vector3 left = wmouse.getClusteringLeftPoint();
                const Vector3 right = wmouse.getClusteringRightPoint();
                snprintf(
                    irMessageBuffer,
                    1024,
                    "lr:%i:%i:%i:%i\n",
                    (int) left.values[0].value,
                    (int) left.values[1].value,
                    (int) right.values[0].value,
                    (int) right.values[1].value
                );
                csocket.broadcastMessage(irMessageBuffer);

                const Vector3 fleft = wmouse.getFilteredLeftPoint();
                const Vector3 fright = wmouse.getFilteredRightPoint();
                snprintf(
                    irMessageBuffer,
                    1024,
                    "flr:%i:%i:%i:%i\n",
                    (int) fleft.values[0].value,
                    (int) fleft.values[1].value,
                    (int) fright.values[0].value,
                    (int) fright.values[1].value
                );
                csocket.broadcastMessage(irMessageBuffer);
            } else {
                csocket.broadcastMessage("lr:invalid\n");
                csocket.broadcastMessage("flr:invalid\n");
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
    }

    std::cout << "Mouse driver stopped!" << std::endl;

    return 0;
}
