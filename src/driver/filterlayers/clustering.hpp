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

    Scalar defaultDistance;

    void processIrSpots(const IRData* irSpots);
    IrSpotClustering();
};

