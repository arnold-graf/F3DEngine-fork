#include "globals.h"
#include "floorProjection.h"
#include "voxelTerrain.h"

namespace RayCast {
    vector<line2> viewRays(Player& plr, int numRays, double fovRadians, double viewDistance);

    double halfFovH, minRay, maxRay, rayStep, rayDistance, viewDistance, lineWidth, verticalShift, horizonCenterY, upperViewLimit, lowerViewLimit;
    vector<line2> rays = viewRays(pl, numRays, fovH, rayDistance);
    FrameBuffer frameBuffer;
    vector<double> depthBuffer;

    inline void ensureFramebuffer() {
        if (frameBuffer.width != windowWidth || frameBuffer.height != windowHeight) {
            frameBuffer.init(windowWidth, windowHeight);
        }
    }

    inline void ensureDepthBuffer() {
        ensureFramebuffer();
        const size_t pixelCount = static_cast<size_t>(frameBuffer.width) * frameBuffer.height;
        if (depthBuffer.size() != pixelCount) {
            depthBuffer.resize(pixelCount);
        }
    }

    inline void clearDepthBuffer() {
        ensureDepthBuffer();
        std::fill(depthBuffer.begin(), depthBuffer.end(), infinity);
    }

    inline void writeDepthRect(int xMin, int xMax, int yMin, int yMax, const vecRGBA& color, double depth) {
        if (xMin >= xMax || yMin >= yMax) return;
        const vecRGBA shaded = color.clamped();
        for (int y = yMin; y < yMax; ++y) {
            const size_t row = static_cast<size_t>(y) * frameBuffer.width;
            for (int x = xMin; x < xMax; ++x) {
                const size_t index = row + static_cast<size_t>(x);
                if (depth <= depthBuffer[index]) {
                    depthBuffer[index] = depth;
                    frameBuffer.accumulationBuffer[index] = shaded;
                }
            }
        }
    }

    inline void fillOpaqueRect(float x1, float y1, float x2, float y2, const vecRGBA& color) {
        int xMin = std::max(0, static_cast<int>(std::floor(x1)));
        int xMax = std::min(frameBuffer.width, static_cast<int>(std::ceil(x2)));
        int yMin = std::max(0, static_cast<int>(std::floor(y1)));
        int yMax = std::min(frameBuffer.height, static_cast<int>(std::ceil(y2)));
        if (xMin >= xMax || yMin >= yMax) return;

        const vecRGBA c = color.clamped();
        for (int y = yMin; y < yMax; ++y) {
            const size_t row = static_cast<size_t>(y) * frameBuffer.width;
            for (int x = xMin; x < xMax; ++x) {
                frameBuffer.accumulationBuffer[row + x] = c;
            }
        }
    }

    inline void fillOpaqueRectDepth(float x1, float y1, float x2, float y2, const vecRGBA& color, double depth) {
        const int xMin = std::max(0, static_cast<int>(std::floor(x1)));
        const int xMax = std::min(frameBuffer.width, static_cast<int>(std::ceil(x2)));
        const int yMin = std::max(0, static_cast<int>(std::floor(y1)));
        const int yMax = std::min(frameBuffer.height, static_cast<int>(std::ceil(y2)));
        writeDepthRect(xMin, xMax, yMin, yMax, color, depth);
    }

    inline void clampScreenY(double& y) {
        y = std::clamp(y, 0.0, static_cast<double>(windowHeight));
    }

    inline int cappedVerticalScans(double ySpan) {
        if (ySpan <= lineWidth) return 1;
        return std::min(static_cast<int>(ySpan / lineWidth), windowHeight);
    }

    void presentFramebuffer() {
        static GLuint presentTexture = 0;
        static std::vector<unsigned char> rgba;

        const int w = frameBuffer.width;
        const int h = frameBuffer.height;
        const size_t pixelCount = static_cast<size_t>(w) * h;
        if (rgba.size() != pixelCount * 4) {
            rgba.resize(pixelCount * 4);
        }

        if (presentTexture == 0) {
            glGenTextures(1, &presentTexture);
            glBindTexture(GL_TEXTURE_2D, presentTexture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }

        #pragma omp parallel for schedule(static)
        for (int i = 0; i < w * h; ++i) {
            const vecRGBA& color = frameBuffer.accumulationBuffer[i];
            const size_t o = static_cast<size_t>(i) * 4;
            rgba[o]     = static_cast<unsigned char>(std::min(255.0, color.r * 255.99));
            rgba[o + 1] = static_cast<unsigned char>(std::min(255.0, color.g * 255.99));
            rgba[o + 2] = static_cast<unsigned char>(std::min(255.0, color.b * 255.99));
            rgba[o + 3] = 255;
        }

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, presentTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, clientWidth, clientHeight, 0, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(static_cast<float>(presentX), static_cast<float>(presentY));
        glTexCoord2f(1.0f, 0.0f); glVertex2f(static_cast<float>(presentX + presentWidth), static_cast<float>(presentY));
        glTexCoord2f(1.0f, 1.0f); glVertex2f(static_cast<float>(presentX + presentWidth), static_cast<float>(presentY + presentHeight));
        glTexCoord2f(0.0f, 1.0f); glVertex2f(static_cast<float>(presentX), static_cast<float>(presentY + presentHeight));
        glEnd();
        glDisable(GL_TEXTURE_2D);
    }

    vector<line2> viewRays(Player& plr, int numRays = 256, double fovRadians = pi * 0.5f, double viewDistance = 1000.0f) {
        pair<double, double> minMaxyaw = plr.viewAngles(fovRadians);
        double yawPerRay = fovRadians / numRays;
        vector<line2> rays;
        rays.reserve(numRays);
        for (int i = 0; i < numRays; ++i) {
            double rayyaw = minMaxyaw.first + (i * yawPerRay);
            vec2 rayDeff = vec2::fromAngle(rayyaw) * viewDistance;
            line2 ray = {plr.position, plr.position + rayDeff};
            rays.push_back(ray);
        }
        return rays;
    }

    void prepareFrame() {
        minRay = pl.yaw - (0.5 * fovH);
        maxRay = minRay + fovH;
        halfFovH = 0.5 * fovH;
        rayStep = fovH / numRays;
        rayDistance = 5000;
        viewDistance = 1000;
        lineWidth = static_cast<double>(windowWidth) / numRays;
        rays = viewRays(pl, numRays, fovH, rayDistance);
        verticalShift = pl.pitch * (windowHeight / (pi * 0.25));
        horizonCenterY = (0.5 * windowHeight) - verticalShift + pl.verticalOffset;
        ensureFramebuffer();
    }

    void drawBaseWorld() {
        prepareFrame();
        frameBuffer.clear(vecRGBA::Black());

        const Texture& skyTexture = textures["./textures/night_sky.ppm"];
        const Texture& floorTexture = textures["./textures/coarse_dirt.ppm"];

        if (horizonCenterY > 0.0) {
            const double yawRatio = (pl.yaw + pi) / (2 * pi);
            const int yScans = static_cast<int>(std::ceil((horizonCenterY - 1) / lineWidth));

            #pragma omp parallel for schedule(static)
            for (int y = yScans - 1; y >= 0; --y) {
                for (int x = 0; x < numRays; ++x) {
                    const int textY = skyTexture.height - 1 - y;
                    const int textureX = static_cast<int>(skyTexture.width * yawRatio + x);
                    const vecRGBA color = skyTexture.get(textureX, textY);
                    const float x1 = x * lineWidth, x2 = (x + 1) * lineWidth;
                    const float y1 = horizonCenterY - (y * lineWidth), y2 = horizonCenterY - ((y - 1) * lineWidth);
                    fillOpaqueRect(x1, y1, x2, y2, color);
                }
            }
        }

        if (lvl.terrain.enabled) {
            clearDepthBuffer();

            const Texture& terrainTexture = textures[lvl.terrain.textureFile];

            #pragma omp parallel for schedule(static)
            for (int x = 0; x < numRays; ++x) {
                const double rayAngle = minRay + (x * rayStep);
                const double cosineFactor = std::cos(std::abs(rayAngle - pl.yaw));
                VoxelTerrain::renderColumn(
                    x, rayAngle, cosineFactor,
                    horizonCenterY, halfWindowHeight, viewDistance, lineWidth, windowHeight,
                    lvl.terrain, terrainTexture, frameBuffer, depthBuffer, lvl.lightList);
            }
        } else if (horizonCenterY < windowHeight) {
            clearDepthBuffer();

            const double groundHeight = 0.0;
            const double cameraHeight = pl.cameraHeight;

            #pragma omp parallel for schedule(static)
            for (int x = 0; x < numRays; ++x) {
                const double rayAngle = minRay + (x * rayStep);
                const double cosineFactor = std::cos(std::abs(rayAngle - pl.yaw));
                const float x1 = static_cast<float>(x * lineWidth);
                const float x2 = x1 + static_cast<float>(lineWidth);

                double previousScreenY = static_cast<double>(windowHeight);
                for (double distance = lineWidth; distance < viewDistance; distance += lineWidth) {
                    const double worldX = pl.position.x + std::cos(rayAngle) * distance;
                    const double worldY = pl.position.y + std::sin(rayAngle) * distance;
                    const double screenY = FloorProjection::projectSurfaceScreenY(
                        horizonCenterY, halfWindowHeight, cameraHeight,
                        distance, groundHeight, cosineFactor);

                    if (screenY < horizonCenterY || screenY >= previousScreenY) {
                        continue;
                    }

                    const double brightness = std::clamp((viewDistance - distance) / viewDistance, 0.0, 1.0);
                    const vecRGBA color = applyLighting(
                        floorTexture.get(worldX, worldY),
                        brightness, {worldX, worldY}, groundHeight, lvl.lightList);

                    fillOpaqueRectDepth(x1, static_cast<float>(screenY), x2, static_cast<float>(previousScreenY), color, distance);
                    previousScreenY = screenY;
                }
            }
        }
    }

    void floorFill(Sector* sect, double rayAngle, double cosineFactor, int rayIndex, double yStart, double yEnd) {
        clampScreenY(yStart);
        clampScreenY(yEnd);
        if (yEnd <= yStart) return;

        const Texture* fillTexture = &textures[sect->floorTextureFile];
        double ySpan = yEnd - yStart;
        int yScans = cappedVerticalScans(ySpan);
        double yScanLineSize = ySpan / yScans;

        double cameraZ = Player::BASE_CAMERA_HEIGHT + pl.verticalOffset;
        double floorZ = sect->baseHeight + sect->floatingHeight;
        (void)cameraZ;
        vec2 sectorCenter = sect->outline.centerPoint();

        for (int y = 0; y < yScans; ++y) {
            const double screenY = yStart + (y + 0.5) * yScanLineSize;
            const double rayLength = FloorProjection::trueDistanceFromScreenY(
                horizonCenterY, halfWindowHeight, pl.cameraHeight,
                screenY, floorZ, cosineFactor);

            if (!std::isfinite(rayLength)) continue;

            vec2 worldPos = {pl.position.x + rayLength * std::cos(rayAngle), pl.position.y + rayLength * std::sin(rayAngle)};
            vec2 worldCoord = worldPos;
            worldCoord.orbit(sectorCenter, -sect->rotation);
            worldCoord -= sectorCenter;

            double brightness = (viewDistance - rayLength) / viewDistance;
            vecRGBA color = applyLighting(
                fillTexture->get(worldCoord.x, worldCoord.y),
                brightness, worldPos, floorZ, lvl.lightList);

            float x1 = rayIndex * lineWidth, x2 = x1 + lineWidth;
            float y1 = yStart + y * yScanLineSize, y2 = y1 + yScanLineSize;

            if ((y1 < 0 && y2 < 0) || (y1 >= windowHeight && y2 >= windowHeight)) continue;
            if (y1 < 0) y1 = 0;
            if (y2 > windowHeight) y2 = windowHeight;

            fillOpaqueRectDepth(x1, y1, x2, y2, color, rayLength);
        }
    }

    void underSideFill(Sector* sect, double rayAngle, double cosineFactor, int rayIndex, double yStart, double yEnd) {
        clampScreenY(yStart);
        clampScreenY(yEnd);

        const double yHigh = std::max(yStart, yEnd);
        const double yLow = std::min(yStart, yEnd);
        if (yHigh <= yLow + 1e-5) return;

        const Texture* fillTexture = &textures[sect->bottomTextureFile];
        const double ySpan = yHigh - yLow;
        const int yScans = cappedVerticalScans(ySpan);
        const double yScanLineSize = ySpan / yScans;

        const double cameraZ = Player::BASE_CAMERA_HEIGHT + pl.verticalOffset;
        const double underSideZ = sect->floatingHeight;
        if (cameraZ >= underSideZ - 1e-5) return;
        (void)cameraZ;
        const vec2 sectorCenter = sect->outline.centerPoint();

        for (int y = 0; y < yScans; ++y) {
            const double screenY = yHigh - (y + 0.5) * yScanLineSize;
            const double rayLength = FloorProjection::trueDistanceFromScreenY(
                horizonCenterY, halfWindowHeight, pl.cameraHeight,
                screenY, underSideZ, cosineFactor);

            if (!std::isfinite(rayLength)) continue;

            vec2 worldPos = {pl.position.x + rayLength * std::cos(rayAngle), pl.position.y + rayLength * std::sin(rayAngle)};
            vec2 worldCoord = worldPos;
            worldCoord.orbit(sectorCenter, -sect->rotation);
            worldCoord -= sectorCenter;

            const double brightness = (viewDistance - rayLength) / viewDistance;
            const vecRGBA color = applyLighting(
                fillTexture->get(worldCoord.x, worldCoord.y),
                brightness, worldPos, underSideZ, lvl.lightList);

            const float x1 = rayIndex * lineWidth, x2 = x1 + lineWidth;
            float y1 = screenY + yScanLineSize * 0.5f;
            float y2 = screenY - yScanLineSize * 0.5f;

            if ((y1 < 0 && y2 < 0) || (y1 >= windowHeight && y2 >= windowHeight)) continue;
            if (y2 < 0) y2 = 0;
            if (y1 > windowHeight) y1 = windowHeight;

            fillOpaqueRectDepth(x1, y2, x2, y1, color, rayLength);
        }
    }

    inline bool cameraBelowSector(const Sector& sect) {
        return pl.cameraHeight < sect.floatingHeight - 1e-5;
    }

    void basicColumnFill(const Texture* fillTexture, double x1, double x2, double yTop, double yBottom, int textureXIndex, double brightness, const vec2& worldPos, double elevationBottom, double elevationTop, double surfaceDepth, double textureScaleY = 1.0, bool invertTexureY = false, double textureBaseSize = Sector::defaultWallHeight) {
        (void)textureScaleY;
        (void)textureBaseSize;

        const int yMin = std::max(0, static_cast<int>(std::floor(yTop)));
        const int yMax = std::min(frameBuffer.height, static_cast<int>(std::ceil(yBottom)));
        const int xMin = std::max(0, static_cast<int>(std::floor(x1)));
        const int xMax = std::min(frameBuffer.width, static_cast<int>(std::ceil(x2)));
        if (xMin >= xMax || yMin >= yMax) return;

        const double ySpan = yBottom - yTop;
        if (ySpan <= 0.0) return;

        const int texX = (textureXIndex % static_cast<int>(fillTexture->width) + static_cast<int>(fillTexture->width)) % static_cast<int>(fillTexture->width);
        const int texHeight = static_cast<int>(fillTexture->height);
        const double invYSpan = 1.0 / ySpan;
        const vecRGBA lightBottom = computeLighting(worldPos, elevationBottom, lvl.lightList);
        const vecRGBA lightTop = computeLighting(worldPos, elevationTop, lvl.lightList);

        for (int y = yMin; y < yMax; ++y) {
            const double t = std::clamp(((y + 0.5) - yTop) * invYSpan, 0.0, 1.0);
            const int texY = invertTexureY
                ? static_cast<int>(t * (texHeight - 1))
                : texHeight - 1 - static_cast<int>(t * (texHeight - 1));
            const vecRGBA light = lightBottom * (1.0 - t) + lightTop * t;
            const vecRGBA color = (fillTexture->get(texX, texY) * brightness * light).clamped();
            fillOpaqueRectDepth(static_cast<float>(xMin), static_cast<float>(y),
                static_cast<float>(xMax), static_cast<float>(y + 1), color, surfaceDepth);
        }
    }

    struct sectorFillMetadata { double basewWallBottomY, baseWallTopY, midWallBottomY, midWallTopY, brightness; int textureXIndex; };

    sectorFillMetadata sectorDataCalc(const Sector* workingSector, const Sector::Wall* workingWall, const line2::collisionInfo& info, const double& rayLength) {
        double relativeYcomparedToHorizon = pl.cameraHeight / (rayLength * halfWindowHeight);
        double virtualY = horizonCenterY + relativeYcomparedToHorizon;
        double distanceFactor = halfWindowHeight / rayLength;
        double basewWallBottomY = virtualY + ((Player::BASE_CAMERA_HEIGHT + pl.verticalOffset - workingSector->floatingHeight) * distanceFactor);
        double baseWallTopY = basewWallBottomY - (workingSector->baseHeight * distanceFactor);
        double midWallBottomY = baseWallTopY;
        double midWallTopY = midWallBottomY - (workingWall->wallHeight * distanceFactor);

        double brightness = (viewDistance - rayLength) / viewDistance;
        int textureXIndex = static_cast<int>(info.fromP1.length());

        return {basewWallBottomY, baseWallTopY, midWallBottomY, midWallTopY, brightness, textureXIndex};
    }

    struct rayPack {
        bool isSector = false, isBillboard = false, isEntryRay = false, isDangling = false;
        int parentIndex = -1, subIndex = -1, pairPartnerIndex = -1;
        double distance3d = infinity, verticalOffset = -infinity;
        line2::collisionInfo collInfo;
    };

    struct RayViewWorkspace {
        struct SectorHit {
            int sectorIndex = -1;
            double maxZ = 0.0;
            vector<pair<int, line2::collisionInfo>> walls;
        };

        vector<SectorHit> sectorHits;
        vector<rayPack> view;

        void reset(size_t sectorCount) {
            if (sectorHits.size() < sectorCount) {
                sectorHits.resize(sectorCount);
                for (auto& sectorHit : sectorHits) {
                    sectorHit.walls.reserve(12);
                }
            }
            for (auto& sectorHit : sectorHits) {
                sectorHit.walls.clear();
            }
            view.clear();
            view.reserve(sectorCount * 4);
        }
    };

    void rayView(const line2& ray, RayViewWorkspace& workspace) {
        const size_t packCount = lvl.sectorPack.size();
        workspace.reset(packCount);

        for (size_t packIndex = 0; packIndex < packCount; ++packIndex) {
            const Level::CastingPackage& pack = lvl.sectorPack[packIndex];
            RayViewWorkspace::SectorHit& sectorHit = workspace.sectorHits[packIndex];
            sectorHit.sectorIndex = pack.index_parentList;
            sectorHit.maxZ = pack.maxZ;

            for (const auto& subgroup : pack.lines) {
                line2::collisionInfo info = ray.findIntersection(subgroup.line, subgroup.index_parentList);
                if (info.collisionFound) {
                    sectorHit.walls.push_back({subgroup.index_parentList, info});
                }
            }

            if (sectorHit.walls.empty()) continue;

            std::sort(sectorHit.walls.begin(), sectorHit.walls.end(),
                [](const pair<int, line2::collisionInfo>& a, const pair<int, line2::collisionInfo>& b) {
                    return a.second.rayLength > b.second.rayLength;
                });

            const Sector& sector = lvl.sectorList[sectorHit.sectorIndex];
            bool playerInSector = sector.outline.isInside(pl.position);
            double verticalOffset = sectorHit.maxZ - pl.cameraHeight;
            int wallCount = static_cast<int>(sectorHit.walls.size());

            for (int i = 0; i < wallCount; ++i) {
                const auto& [wallIndex, info] = sectorHit.walls[i];
                rayPack r;
                r.isSector = true;
                r.parentIndex = sectorHit.sectorIndex;
                r.subIndex = wallIndex;
                r.distance3d = std::hypot(info.rayLength, verticalOffset);
                r.verticalOffset = verticalOffset;
                r.collInfo = info;
                r.pairPartnerIndex = -1;

                if (playerInSector) {
                    if (wallCount == 1) {
                        r.isDangling = true;
                        r.isEntryRay = false;
                    } else if (wallCount % 2 == 1 && i == wallCount - 1) {
                        r.isDangling = true;
                        r.isEntryRay = false;
                    } else {
                        r.isEntryRay = (i % 2 == 1);
                        r.isDangling = false;
                    }
                } else {
                    r.isDangling = false;
                    if (wallCount == 1) {
                        r.isEntryRay = true;
                    } else if (wallCount % 2 == 0) {
                        r.isEntryRay = (i % 2 == 1);
                    } else {
                        r.isEntryRay = true;
                    }
                }

                workspace.view.push_back(r);
            }
        }

        std::sort(workspace.view.begin(), workspace.view.end(),
            [](const rayPack& a, const rayPack& b) {
                return a.distance3d > b.distance3d;
            });

        for (size_t i = 0; i < workspace.view.size(); ++i) {
            rayPack& r = workspace.view[i];
            if (!r.isEntryRay && !r.isDangling && r.isSector && i + 1 < workspace.view.size()) {
                auto it = std::find_if(workspace.view.begin() + i + 1, workspace.view.end(),
                    [&](const rayPack& p) {
                        return p.parentIndex == r.parentIndex;
                    });

                if (it != workspace.view.end()) {
                    int pairIndex = static_cast<int>(std::distance(workspace.view.begin(), it));
                    r.pairPartnerIndex = pairIndex;
                    workspace.view[pairIndex].pairPartnerIndex = static_cast<int>(i);
                } else {
                    r.pairPartnerIndex = -1;
                }
            }
        }
    }

    void renderRayColumn(int x, const line2& ray, double rayAngle, double cosineFactor, RayViewWorkspace& workspace) {
        const double x1 = x * lineWidth, x2 = x1 + lineWidth;
        rayView(ray, workspace);
        const vector<rayPack>& view = workspace.view;

        for (const rayPack& obj : view) {
            if (!obj.isSector || obj.parentIndex < 0 || obj.parentIndex >= static_cast<int>(lvl.sectorList.size())) continue;

            Sector& workingSector = lvl.sectorList[obj.parentIndex];
            if (obj.subIndex < 0 || obj.subIndex >= static_cast<int>(workingSector.walls.size())) continue;

            Sector::Wall& workingWall = workingSector.walls[obj.subIndex];
            if (!std::isfinite(obj.collInfo.rayLength) || obj.collInfo.rayLength <= 0.001) continue;

            double adjustedDistance = obj.collInfo.rayLength * cosineFactor;
            sectorFillMetadata wallInfo = sectorDataCalc(&workingSector, &workingWall, obj.collInfo, adjustedDistance);
            double brightness = std::clamp((viewDistance - obj.collInfo.rayLength) / viewDistance, 0.0, 1.0);
            const vec2& wallWorldPos = obj.collInfo.collisionPoint;
            const double floorElevation = workingSector.floatingHeight + workingSector.baseHeight;
            const double midWallTopElevation = floorElevation + workingWall.wallHeight;

            if (obj.isDangling) {
                if (wallInfo.midWallBottomY > horizonCenterY) {
                    if (workingWall.isVisibleWall) {
                        basicColumnFill(&textures[workingWall.textureFile], x1, x2,
                            wallInfo.midWallTopY, wallInfo.midWallBottomY,
                            wallInfo.textureXIndex, brightness, wallWorldPos,
                            floorElevation, midWallTopElevation,
                            obj.collInfo.rayLength,
                            workingWall.wallHeight / workingSector.defaultWallHeight);
                    }
                    if (!VoxelTerrain::replacesSectorFloor(obj.parentIndex, lvl.terrain)) {
                        floorFill(&workingSector, rayAngle, cosineFactor, x,
                            wallInfo.midWallBottomY, static_cast<double>(windowHeight) - 1.0);
                    }
                } else {
                    basicColumnFill(&textures[workingWall.textureFile], x1, x2,
                        wallInfo.baseWallTopY, wallInfo.basewWallBottomY,
                        wallInfo.textureXIndex, brightness, wallWorldPos,
                        workingSector.floatingHeight, floorElevation,
                        obj.collInfo.rayLength,
                        workingSector.baseHeight / workingSector.defaultWallHeight);
                    if (cameraBelowSector(workingSector) && wallInfo.basewWallBottomY < horizonCenterY) {
                        underSideFill(&workingSector, rayAngle, cosineFactor, x,
                            wallInfo.basewWallBottomY, 0.0);
                    }
                }
            } else if (obj.isEntryRay) {
                if (workingWall.isVisibleWall) {
                    basicColumnFill(&textures[workingWall.textureFile], x1, x2,
                        wallInfo.midWallTopY, wallInfo.midWallBottomY,
                        wallInfo.textureXIndex, brightness, wallWorldPos,
                        floorElevation, midWallTopElevation,
                        obj.collInfo.rayLength,
                        workingWall.wallHeight / workingSector.defaultWallHeight);
                }

                basicColumnFill(&textures[workingSector.bottomTextureFile], x1, x2,
                    wallInfo.baseWallTopY, wallInfo.basewWallBottomY,
                    wallInfo.textureXIndex, brightness, wallWorldPos,
                    workingSector.floatingHeight, floorElevation,
                    obj.collInfo.rayLength,
                    workingSector.baseHeight / workingSector.defaultWallHeight);

                if (cameraBelowSector(workingSector) && wallInfo.basewWallBottomY < horizonCenterY) {
                    if (obj.pairPartnerIndex >= 0 && obj.pairPartnerIndex < static_cast<int>(view.size())) {
                        const rayPack& pair = view[obj.pairPartnerIndex];
                        if (std::isfinite(pair.collInfo.rayLength)) {
                            const Sector::Wall& pairWall = workingSector.walls[pair.subIndex];
                            const sectorFillMetadata pairWallInfo = sectorDataCalc(&workingSector, &pairWall,
                                pair.collInfo, pair.collInfo.rayLength * cosineFactor);
                            underSideFill(&workingSector, rayAngle, cosineFactor, x,
                                pairWallInfo.basewWallBottomY, wallInfo.basewWallBottomY);
                        }
                    } else {
                        underSideFill(&workingSector, rayAngle, cosineFactor, x,
                            wallInfo.basewWallBottomY, 0.0);
                    }
                }
            } else {
                if (workingWall.isVisibleWall) {
                    basicColumnFill(&textures[workingWall.textureFile], x1, x2,
                        wallInfo.midWallTopY, wallInfo.midWallBottomY,
                        wallInfo.textureXIndex, brightness, wallWorldPos,
                        floorElevation, midWallTopElevation,
                        obj.collInfo.rayLength,
                        workingWall.wallHeight / workingSector.defaultWallHeight);
                }

                if (obj.pairPartnerIndex >= 0 && obj.pairPartnerIndex < static_cast<int>(view.size())) {
                    const rayPack& pair = view[obj.pairPartnerIndex];
                    if (!std::isfinite(pair.collInfo.rayLength)) continue;

                    sectorFillMetadata pairWallInfo = sectorDataCalc(&workingSector, &workingWall,
                        pair.collInfo, pair.collInfo.rayLength * cosineFactor);
                    if (pairWallInfo.midWallBottomY > horizonCenterY
                            && !VoxelTerrain::replacesSectorFloor(obj.parentIndex, lvl.terrain)) {
                        floorFill(&workingSector, rayAngle, cosineFactor, x,
                            wallInfo.baseWallTopY, pairWallInfo.baseWallTopY);
                    }
                }
            }
        }
    }

    void rayCast() {
        #pragma omp parallel
        {
            RayViewWorkspace workspace;
            #pragma omp for schedule(static)
            for (int x = 0; x < static_cast<int>(rays.size()); ++x) {
                const line2& ray = rays[x];
                const double rayAngle = minRay + rayStep * x;
                const double cosineFactor = std::cos(std::abs(rayAngle - pl.yaw));
                renderRayColumn(x, ray, rayAngle, cosineFactor, workspace);
            }
        }
    }
};
