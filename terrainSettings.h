#pragma once

#include <string>
#include <vector>

#include "vecSys/vec2.h"

struct Mountain {
    double centerX = 0.0;
    double centerY = 0.0;
    double radius = 100.0;
    double peakHeight = 200.0;
    double detailScale = 0.02;
    unsigned int seed = 0;
};

struct TerrainSettings {
    bool enabled = false;
    unsigned int seed = 1337;
    double noiseScale = 0.004;
    double heightAmplitude = 40.0;
    double baseHeight = 12.0;
    double stepSize = 2.5;
    int fbmOctaves = 5;
    double fbmPersistence = 0.5;
    double fbmLacunarity = 2.0;
    std::string textureFile = "./textures/coarse_dirt.ppm";
    int groundSectorIndex = 0;
    std::vector<Mountain> mountains;
};
