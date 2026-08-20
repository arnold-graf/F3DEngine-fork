#pragma once

#include <algorithm>
#include <cmath>

#include "vecSys/base.h"

namespace FloorProjection {
    // Match sectorDataCalc wall/floor vertical placement (includes pitched-horizon virtualY).
    inline double projectSurfaceScreenY(
        double horizonCenterY,
        double halfWindowHeight,
        double cameraHeight,
        double trueRayDistance,
        double surfaceHeight,
        double cosineFactor) {

        const double adjustedRayLength = std::max(trueRayDistance * cosineFactor, 1e-6);
        const double distanceFactor = halfWindowHeight / adjustedRayLength;
        const double virtualY = horizonCenterY + cameraHeight / (adjustedRayLength * halfWindowHeight);
        return virtualY + (cameraHeight - surfaceHeight) * distanceFactor;
    }

    inline double trueDistanceFromScreenY(
        double horizonCenterY,
        double halfWindowHeight,
        double cameraHeight,
        double screenY,
        double surfaceHeight,
        double cosineFactor) {

        const double screenOffset = screenY - horizonCenterY;
        if (std::abs(screenOffset) < 1e-5) {
            return infinity;
        }

        const double heightDelta = cameraHeight - surfaceHeight;
        const double adjustedRayLength = (cameraHeight / halfWindowHeight + heightDelta * halfWindowHeight) / screenOffset;
        if (adjustedRayLength <= 0.0) {
            return infinity;
        }

        return adjustedRayLength / std::max(cosineFactor, 1e-6);
    }
}
