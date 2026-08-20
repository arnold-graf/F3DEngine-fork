#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <random>
#include <vector>

#include "globals.h"
#include "light.h"
#include "terrainSettings.h"
#include "floorProjection.h"

struct PerlinNoise {
    std::array<int, 512> permutation {};

    void seed(unsigned int value) {
        std::array<int, 256> source {};
        std::iota(source.begin(), source.end(), 0);
        std::mt19937 random(value);
        std::shuffle(source.begin(), source.end(), random);
        for (int index = 0; index < 256; ++index) {
            permutation[index] = source[index];
            permutation[index + 256] = source[index];
        }
    }

    static double fade(double value) {
        return value * value * value * (value * (value * 6.0 - 15.0) + 10.0);
    }

    static double linearInterpolate(double start, double end, double amount) {
        return start + amount * (end - start);
    }

    static double gradient2(int hash, double x, double y) {
        const int direction = hash & 7;
        const double u = direction < 4 ? x : y;
        const double v = direction < 4 ? y : x;
        return ((direction & 1) ? -u : u) + ((direction & 2) ? -2.0 * v : 2.0 * v);
    }

    double sample(double x, double y) const {
        const int x0 = static_cast<int>(std::floor(x)) & 255;
        const int y0 = static_cast<int>(std::floor(y)) & 255;
        const int x1 = (x0 + 1) & 255;
        const int y1 = (y0 + 1) & 255;

        const double localX = x - std::floor(x);
        const double localY = y - std::floor(y);

        const double fadeX = fade(localX);
        const double fadeY = fade(localY);

        const int hash00 = permutation[x0] + y0;
        const int hash10 = permutation[x1] + y0;
        const int hash01 = permutation[x0] + y1;
        const int hash11 = permutation[x1] + y1;

        const double lower = linearInterpolate(
            gradient2(permutation[hash00], localX, localY),
            gradient2(permutation[hash10], localX - 1.0, localY),
            fadeX);
        const double upper = linearInterpolate(
            gradient2(permutation[hash01], localX, localY - 1.0),
            gradient2(permutation[hash11], localX - 1.0, localY - 1.0),
            fadeX);

        return linearInterpolate(lower, upper, fadeY);
    }
};

namespace VoxelTerrain {
    inline PerlinNoise noise;
    inline PerlinNoise mountainNoise;
    inline bool ready = false;

    inline void initialize(const TerrainSettings& settings) {
        if (!settings.enabled) {
            ready = false;
            return;
        }
        noise.seed(settings.seed);
        mountainNoise.seed(settings.seed ^ 0x9E3779B9u);
        ready = true;
    }

    inline double smoothFalloff(double distance, double radius) {
        if (distance >= radius) return 0.0;
        const double blend = 1.0 - distance / radius;
        return blend * blend * (3.0 - 2.0 * blend);
    }

    inline double fractalBrownianMotion(double x, double y, const TerrainSettings& settings) {
        double total = 0.0;
        double amplitude = 1.0;
        double frequency = 1.0;
        double normalization = 0.0;

        for (int octave = 0; octave < settings.fbmOctaves; ++octave) {
            total += noise.sample(x * frequency, y * frequency) * amplitude;
            normalization += amplitude;
            amplitude *= settings.fbmPersistence;
            frequency *= settings.fbmLacunarity;
        }

        return normalization > 0.0 ? total / normalization : 0.0;
    }

    inline double sampleBaseHeight(double worldX, double worldY, const TerrainSettings& settings) {
        const double noiseValue = fractalBrownianMotion(
            worldX * settings.noiseScale,
            worldY * settings.noiseScale,
            settings);
        return settings.baseHeight + noiseValue * settings.heightAmplitude;
    }

    inline double sampleMountainHeight(
        double worldX,
        double worldY,
        double baseHeight,
        const Mountain& mountain,
        const TerrainSettings& settings) {

        const double offsetX = worldX - mountain.centerX;
        const double offsetY = worldY - mountain.centerY;
        const double distance = std::sqrt(offsetX * offsetX + offsetY * offsetY);
        const double falloff = smoothFalloff(distance, mountain.radius);
        if (falloff <= 0.0) {
            return baseHeight;
        }

        const unsigned int detailSeed = mountain.seed != 0 ? mountain.seed : settings.seed + 17'713u;
        const double detailX = (worldX + static_cast<double>(detailSeed)) * mountain.detailScale;
        const double detailY = (worldY + mountain.centerX * 0.37) * mountain.detailScale;

        double detailAmplitude = 1.0;
        double detailFrequency = 1.0;
        double detailTotal = 0.0;
        double detailNormalization = 0.0;
        constexpr int detailOctaves = 4;

        for (int octave = 0; octave < detailOctaves; ++octave) {
            detailTotal += mountainNoise.sample(detailX * detailFrequency, detailY * detailFrequency) * detailAmplitude;
            detailNormalization += detailAmplitude;
            detailAmplitude *= 0.5;
            detailFrequency *= 2.1;
        }

        const double detail = detailNormalization > 0.0 ? detailTotal / detailNormalization : 0.0;
        const double rockyLift = detail * mountain.peakHeight * 0.18 * falloff;
        return baseHeight + mountain.peakHeight * falloff + rockyLift;
    }

    inline double sampleHeight(double worldX, double worldY, const TerrainSettings& settings) {
        const double baseHeight = sampleBaseHeight(worldX, worldY, settings);
        double height = baseHeight;
        for (const Mountain& mountain : settings.mountains) {
            height = std::max(height, sampleMountainHeight(worldX, worldY, baseHeight, mountain, settings));
        }
        return height;
    }

    inline double sampleHeightAt(const vec2& worldPosition, const TerrainSettings& settings) {
        return sampleHeight(worldPosition.x, worldPosition.y, settings);
    }

    inline bool replacesSectorFloor(int sectorIndex, const TerrainSettings& settings) {
        return settings.enabled && sectorIndex == settings.groundSectorIndex;
    }

    inline void renderColumn(
        int columnIndex,
        double rayAngle,
        double cosineFactor,
        double horizonCenterY,
        double halfWindowHeight,
        double viewDistance,
        double lineWidth,
        int windowHeight,
        const TerrainSettings& settings,
        const Texture& texture,
        FrameBuffer& framebuffer,
        std::vector<double>& depthBuffer,
        const std::vector<Light>& lights) {

        const double cameraHeight = pl.cameraHeight;
        const double rayDirectionX = std::cos(rayAngle);
        const double rayDirectionY = std::sin(rayAngle);

        double previousScreenY = static_cast<double>(windowHeight);
        const int xMin = std::max(0, static_cast<int>(std::floor(columnIndex * lineWidth)));
        const int xMax = std::min(framebuffer.width, static_cast<int>(std::ceil((columnIndex + 1) * lineWidth)));
        if (xMin >= xMax) return;

        const size_t rowWidth = static_cast<size_t>(framebuffer.width);

        for (double distance = settings.stepSize; distance < viewDistance; distance += settings.stepSize) {
            const double worldX = pl.position.x + rayDirectionX * distance;
            const double worldY = pl.position.y + rayDirectionY * distance;
            const double terrainHeight = sampleHeight(worldX, worldY, settings);

            const double screenY = FloorProjection::projectSurfaceScreenY(
                horizonCenterY, halfWindowHeight, cameraHeight,
                distance, terrainHeight, cosineFactor);

            // Tall hills project above the horizon; only skip samples that do not
            // advance the visible surface (same rule as wall columns).
            if (screenY >= previousScreenY) {
                continue;
            }

            const double brightness = std::clamp((viewDistance - distance) / viewDistance, 0.0, 1.0);
            const vecRGBA color = applyLighting(
                texture.get(worldX, worldY),
                brightness,
                {worldX, worldY},
                terrainHeight,
                lights).clamped();

            const int yMin = std::max(0, static_cast<int>(std::floor(screenY)));
            const int yMax = std::min(framebuffer.height, static_cast<int>(std::ceil(previousScreenY)));
            const vecRGBA shaded = color.clamped();
            for (int y = yMin; y < yMax; ++y) {
                const size_t row = static_cast<size_t>(y) * rowWidth;
                for (int x = xMin; x < xMax; ++x) {
                    const size_t index = row + static_cast<size_t>(x);
                    if (distance <= depthBuffer[index]) {
                        depthBuffer[index] = distance;
                        framebuffer.accumulationBuffer[index] = shaded;
                    }
                }
            }

            previousScreenY = screenY;
        }
    }
}
