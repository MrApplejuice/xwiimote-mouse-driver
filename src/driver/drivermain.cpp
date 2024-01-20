/*
This file is part of xwiimote-mouse-driver.

Foobar is free software: you can redistribute it and/or modify it under 
the terms of the GNU General Public License as published by the Free 
Software Foundation, either version 3 of the License, or (at your option) 
any later version.

Foobar is distributed in the hope that it will be useful, but WITHOUT 
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with 
Foobar. If not, see <https://www.gnu.org/licenses/>. 
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
#include "virtualmouse.hpp"
#include "controlsocket.hpp"
#include "device.hpp"
#include "settings.hpp"
#include "driveroptparse.hpp"
#include "driverextra.hpp"

class WiiMouseProcessingModule {
public:
    static const int MAX_BUTTONS = 32;
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

    NamespacedButtonState pressedButtons[MAX_BUTTONS];

    int nValidIrSpots;
    Vector3 trackingDots[4];
    Vector3 accelVector;

    bool isButtonPressed(ButtonNamespace ns, int buttonId) const {
        const auto searchFor = NamespacedButtonState(ns, buttonId, false);
        for (int i = 0; i < MAX_BUTTONS; i++) {
            if (!pressedButtons[i]) {
                break;
            }
            if (pressedButtons[i].matches(searchFor)) {
                return pressedButtons[i].state;
            }
        }
        return false;
    }

    virtual void process(const WiiMouseProcessingModule& prev) = 0;
};

class WMPDummy : public WiiMouseProcessingModule {
public:
    virtual void process(const WiiMouseProcessingModule& prev) override {
        copyFromPrev(prev);
    }
};

class WMPButtonMapper : public WiiMouseProcessingModule {
private:
    std::map<WiimoteButton, std::vector<int>> wiiToEvdevMap;
public:
    void clearButtonAssignments(WiimoteButton wiiButton) {
        wiiToEvdevMap.erase(wiiButton);
    }

    void clearMapping() {
        wiiToEvdevMap.clear();
    }

    void addMapping(WiimoteButton wiiButton, int evdevButton) {
        auto& v = wiiToEvdevMap[wiiButton];
        if (std::find(v.begin(), v.end(), evdevButton) != v.end()) {
            return;
        }
        v.push_back(evdevButton);
    }

    std::map<WiimoteButton, std::string> getStringMappings() const {
        std::map<WiimoteButton, std::string> result;
        for (auto& mapping : wiiToEvdevMap) {
            for (int evdevButton : mapping.second) {
                const SupportedButton* btn = findButtonByCode(evdevButton);
                if (btn) {
                    result[mapping.first] = btn->rawKeyName;
                    break;                    
                }
            }
        }
        return result;
    }

    virtual void process(const WiiMouseProcessingModule& prev) override {
        std::vector<int> lastPressedButtons;
        for (int i = 0; i < MAX_BUTTONS; i++) {
            if (!pressedButtons[i]) {
                break;
            }
            if ((pressedButtons[i].ns == ButtonNamespace::VMOUSE) && pressedButtons[i].state) {
                lastPressedButtons.push_back(pressedButtons[i].buttonId);
            }
        }

        copyFromPrev(prev);

        int assignedButtons = 0;
        for (auto& button : prev.pressedButtons) {
            if (assignedButtons >= MAX_BUTTONS) {
                break;
            }
            if (!button) {
                break;
            }
            if (button.ns == ButtonNamespace::WII) {
                if (!button.state) {
                    continue;
                }
                auto mapping = wiiToEvdevMap.find((WiimoteButton) button.buttonId);
                if (mapping != wiiToEvdevMap.end()) {
                    for (int evdevButton : mapping->second) {
                        if (assignedButtons >= MAX_BUTTONS) {
                            break;
                        }
                        pressedButtons[assignedButtons++] = NamespacedButtonState(
                            ButtonNamespace::VMOUSE, evdevButton, button.state
                        );
                        lastPressedButtons.erase(
                            std::remove(
                                lastPressedButtons.begin(),
                                lastPressedButtons.end(),
                                evdevButton
                            ),
                            lastPressedButtons.end()
                        );
                    }
                }
            }
        }

        for (int evdevButton : lastPressedButtons) {
            if (assignedButtons >= MAX_BUTTONS) {
                break;
            }
            pressedButtons[assignedButtons++] = NamespacedButtonState(
                ButtonNamespace::VMOUSE, evdevButton, false
            );
        }

        if (assignedButtons < MAX_BUTTONS) {
            pressedButtons[assignedButtons++] = NamespacedButtonState::NONE;
        }
    }
};

class WMPSmoother : public WiiMouseProcessingModule {
private:
    bool hasAccel;
    Vector3 lastAccel;

    bool hasPosition;
    Vector3 lastPositions[4];

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

        float posMix, accelMix;

        const bool buttonIsPressed = isButtonPressed(ButtonNamespace::VMOUSE, 0) 
            || isButtonPressed(ButtonNamespace::VMOUSE, 1) 
            || isButtonPressed(ButtonNamespace::VMOUSE, 2);
        
        clickReleaseTimer = maxf(clickReleaseTimer - deltaT / 1000.0f, 0.0f);
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
                    (trackingDots[i] * Scalar(1.0f - posMix, 1000000)).redivide(100) + 
                    (lastPositions[i] * Scalar(posMix, 1000000)).redivide(100)
                );
            }
        }

        if (hasAccel && enabled) {
            accelVector = lastAccel = (
                (accelVector * Scalar(1.0f - accelMix, 1000000)).redivide(100) +
                (lastAccel * Scalar(accelMix, 1000000)).redivide(100)
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

        accelMixFactor = 0.2f;
        accelMixFactorClicked = 0.2f;

        positionMixFactor = 1e-20f;
        positionMixFactorClicked = 0.1f;
        clickReleaseBlendDelay = 1.0f;
        clickReleaseFreezeDelay = 0.25f;
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
    WMPButtonMapper buttonMapper;
    WMPSmoother smoother;
    WMPDummy processingEnd;

    std::vector<WiiMouseProcessingModule*> processorSequence;

    void computeMouseMat() {
        Vector3 screenAreaSize = screenAreaBottomRight - screenAreaTopLeft;

        wiimoteMouseMatX = calmatX * Scalar(screenAreaSize.values[0] / 10000L, 1000000);
        wiimoteMouseMatY = calmatY * Scalar(screenAreaSize.values[1] / 10000L, 1000000);

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

    std::map<WiimoteButton, std::string> getButtonMap() const {
        return buttonMapper.getStringMappings();
    }

    void clearWiiButtonAssignments(const std::string& wiiButton) {
        WiimoteButton foundWiiButton = WiimoteButton::INVALID;
        foundWiiButton = rmap(WIIMOTE_BUTTON_READABLE_NAMES, wiiButton, WiimoteButton::INVALID);
        if (foundWiiButton == WiimoteButton::INVALID) {
            foundWiiButton = rmap(WIIMOTE_BUTTON_NAMES, wiiButton, WiimoteButton::INVALID);
        }
        if (foundWiiButton == WiimoteButton::INVALID) {
            return;
        }
        buttonMapper.clearButtonAssignments(foundWiiButton);
    }

    void clearButtonMap() {
        buttonMapper.clearMapping();
    }

    bool mapButton(const std::string& wiiButton, const std::string& evdevButton) {
        WiimoteButton foundWiiButton = WiimoteButton::INVALID;
        foundWiiButton = rmap(WIIMOTE_BUTTON_READABLE_NAMES, wiiButton, WiimoteButton::INVALID);
        if (foundWiiButton == WiimoteButton::INVALID) {
            foundWiiButton = rmap(WIIMOTE_BUTTON_NAMES, wiiButton, WiimoteButton::INVALID);
        }
        if (foundWiiButton == WiimoteButton::INVALID) {
            return false;
        }

        auto evdevButtonSearch = findButtonByName(evdevButton);
        if (!evdevButtonSearch) {
            return false;
        }

        buttonMapper.addMapping(foundWiiButton, evdevButtonSearch->code);
        return true;
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

        processingStart.deltaT = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - lastupdate
        ).count();
        processingStart.accelVector = accelVector;
        processingStart.nValidIrSpots = irSpotClustering.valid ? 2 : 0;
        processingStart.trackingDots[0] = irSpotClustering.leftPoint;
        processingStart.trackingDots[1] = irSpotClustering.rightPoint;
        {
            for (int bi = 0; bi < (int) WiimoteButton::COUNT; bi++) {
                processingStart.pressedButtons[bi] = NamespacedButtonState(
                    ButtonNamespace::WII, bi, wiimote->buttonStates.pressedButtons[bi]
                );
            }
        }
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

        buttonMapper.addMapping(WiimoteButton::A, BTN_LEFT);
        buttonMapper.addMapping(WiimoteButton::B, BTN_RIGHT);

        processorSequence.push_back(&processingStart);
        processorSequence.push_back(&buttonMapper);
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

        auto keyMappings = wmouse.getButtonMap();
        for (auto& mapping : keyMappings) {
            config.provideDefault(
                "button_" + asciiLower(WIIMOTE_BUTTON_READABLE_NAMES[mapping.first]),
                mapping.second
            );
        }
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
    wmouse.clearButtonMap();
    for (auto btnName : WIIMOTE_BUTTON_READABLE_NAMES) {
        std::string key = asciiLower("button_" + btnName.second);
        if (config.stringOptions.find(key) != config.stringOptions.end()) {
            if (!wmouse.mapButton(btnName.second, config.stringOptions[key])) {
                std::cerr << "Ignoring invalid button map: " 
                    << btnName.second 
                    << " to " 
                    << config.stringOptions[key] << std::endl;
            }
        }
    }

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
                        ss << WIIMOTE_BUTTON_NAMES[mapping.first] 
                            << ":" << mapping.second << ":";
                    }
                    eventResultBuffer = ss.str();
                    eventResultBuffer.pop_back(); // remove trailing ':'
                    return eventResultBuffer.c_str();
                }
                if (command == "bindkey") {
                    if (parameters.size() != 2) {
                        return "ERROR:Invalid parameter count";
                    }
                    std::string keyName = trim(parameters[1]);

                    // Test if valid
                    if (parameters[1] == "" || (!wmouse.mapButton(parameters[0], keyName))) {
                        return "ERROR:Invalid button";
                    }
                    // Now execute the button assignment
                    wmouse.clearWiiButtonAssignments(parameters[0]);
                    if (keyName != "") {
                        wmouse.mapButton(parameters[0], keyName);
                    }

                    for (auto btn : WIIMOTE_BUTTON_READABLE_NAMES) {
                        config.stringOptions.erase("button_" + asciiLower(btn.second));
                    }
                    for (auto& mapping : wmouse.getButtonMap()) {
                        config.stringOptions[
                            "button_" + asciiLower(WIIMOTE_BUTTON_READABLE_NAMES[mapping.first])
                        ] = mapping.second;
                    }
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
