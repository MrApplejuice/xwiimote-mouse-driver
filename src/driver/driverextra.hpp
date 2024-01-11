#pragma once

#include "intlinalg.hpp"

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

