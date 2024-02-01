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

class WMPTowedCircle : public WiiMouseProcessingModule {
private:
    bool validCircle;
    Vector3f circleCenter;
public:
    float radius, aspectRatio;

    virtual void process(const WiiMouseProcessingModule& prev) override;

    WMPTowedCircle() {
        radius = 0.005f;
        aspectRatio = 1024.0f / 768.0f;
    }
};
