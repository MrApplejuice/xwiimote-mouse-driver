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

#include "../driverextra.hpp"
#include "../device.hpp"
#include "../stringtools.hpp"


struct WiimoteButtonMappingState {
    WiimoteButton button;
    bool irVisible;

    bool operator==(const WiimoteButtonMappingState& other) const {
        return (button == other.button) && (irVisible == other.irVisible);
    }

    std::string toProtocolString() const {
        std::string result = WIIMOTE_BUTTON_NAMES[button];
        if (irVisible) {
            result += ":1";
        } else {
            result += ":0";
        }
        return result;
    }

    std::string toConfigurationKey() const {
        std::string result = "button_";
        result += asciiLower(WIIMOTE_BUTTON_READABLE_NAMES[button]);
        if (!irVisible) {
            result += "_offscreen";
        }
        return result;
    }
};


static bool operator<(
    const WiimoteButtonMappingState& a,
    const WiimoteButtonMappingState& b
) {
    if (a.button < b.button) {
        return true;
    }
    if (a.button > b.button) {
        return false;
    }
    return a.irVisible < b.irVisible;
}


class WMPButtonMapper : public WiiMouseProcessingModule {
private:
    std::map<WiimoteButtonMappingState, std::vector<int>> wiiToEvdevMap;
public:
    void clearButtonAssignments(WiimoteButton wiiButton, bool ir) {
        WiimoteButtonMappingState key = {wiiButton, ir};
        wiiToEvdevMap.erase(key);
    }

    void clearMapping() {
        wiiToEvdevMap.clear();
    }

    void addMapping(WiimoteButton wiiButton, bool ir, int evdevButton) {
        WiimoteButtonMappingState key = {wiiButton, ir};
        auto& v = wiiToEvdevMap[key];
        if (std::find(v.begin(), v.end(), evdevButton) != v.end()) {
            return;
        }
        v.push_back(evdevButton);
    }

    std::map<WiimoteButtonMappingState, std::string> getStringMappings() const {
        std::map<WiimoteButtonMappingState, std::string> result;
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
            if (!button || (button.ns != ButtonNamespace::WII)) {
                break;
            }
            WiimoteButtonMappingState key = {
                (WiimoteButton) button.buttonId, prev.nValidIrSpots > 0
            };
            if (button.ns == ButtonNamespace::WII) {
                if (!button.state) {
                    continue;
                }
                auto mapping = wiiToEvdevMap.find(key);
                if (mapping == wiiToEvdevMap.end()) {
                    continue;
                }
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
