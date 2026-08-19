#include "globals.h"

namespace RayCast {
    vector<line2> viewRays(Player& plr, int numRays, double fovRadians, double viewDistance);

    double halfFovH, minRay, maxRay, rayStep, rayDistance, viewDistance, lineWidth, verticalShift, horizonCenterY, upperViewLimit, lowerViewLimit;
    vector<line2> rays = viewRays(pl, numRays, fovH, rayDistance);
    FrameBuffer frameBuffer;

    inline void clampScreenY(double& y) {
        y = std::clamp(y, 0.0, static_cast<double>(windowHeight));
    }

    inline int cappedVerticalScans(double ySpan) {
        if (ySpan <= lineWidth) return 1;
        return std::min(static_cast<int>(ySpan / lineWidth), windowHeight);
    }

    inline void ensureFramebuffer() {
        if (frameBuffer.width != windowWidth || frameBuffer.height != windowHeight) {
            frameBuffer.init(windowWidth, windowHeight);
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

    void presentFramebuffer() {
        static GLuint presentTexture = 0;
        if (presentTexture == 0) {
            glGenTextures(1, &presentTexture);
            glBindTexture(GL_TEXTURE_2D, presentTexture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }

        std::vector<unsigned char> rgba(static_cast<size_t>(frameBuffer.width) * frameBuffer.height * 4);
        for (int y = 0; y < frameBuffer.height; ++y) {
            for (int x = 0; x < frameBuffer.width; ++x) {
                const vecRGBA color = frameBuffer.accumulationBuffer[y * frameBuffer.width + x].clamped();
                const size_t i = (static_cast<size_t>(y) * frameBuffer.width + x) * 4;
                rgba[i]     = static_cast<unsigned char>(color.r * 255.99);
                rgba[i + 1] = static_cast<unsigned char>(color.g * 255.99);
                rgba[i + 2] = static_cast<unsigned char>(color.b * 255.99);
                rgba[i + 3] = 255;
            }
        }

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, presentTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, frameBuffer.width, frameBuffer.height, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, windowWidth, windowHeight, 0, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex2f(static_cast<float>(windowWidth), 0.0f);
        glTexCoord2f(1.0f, 1.0f); glVertex2f(static_cast<float>(windowWidth), static_cast<float>(windowHeight));
        glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, static_cast<float>(windowHeight));
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

    void drawBaseWorld() {
        minRay = pl.yaw - (0.5 * fovH);
        rayStep = fovH / numRays;
        viewDistance = 1000;
        lineWidth = static_cast<double>(windowWidth) / numRays;

        rays = viewRays(pl, numRays, fovH, viewDistance);

        verticalShift = pl.pitch * (windowHeight / (pi * 0.25));
        horizonCenterY = (0.5 * windowHeight) - verticalShift + pl.verticalOffset;

        ensureFramebuffer();
        frameBuffer.clear(vecRGBA::Black());

        const unsigned int threadCount = std::max(1u, std::thread::hardware_concurrency());
        const Texture& skyTexture = textures["./textures/night_sky.ppm"];
        const Texture& floorTexture = textures["./textures/coarse_dirt.ppm"];

        vector<std::thread> threads;

        if (horizonCenterY > 0.0) {
            double yawRatio = (pl.yaw + pi) / (2 * pi);
            int yScans = static_cast<int>(std::ceil((horizonCenterY - 1) / lineWidth));

            for (unsigned int t = 0; t < threadCount; ++t) {
                threads.emplace_back([&, t] {
                    for (int y = yScans - 1 - static_cast<int>(t); y >= 0; y -= static_cast<int>(threadCount)) {
                        for (int x = 0; x < numRays; ++x) {
                            int textY = skyTexture.height - 1 - y;
                            int textureX = static_cast<int>(skyTexture.width * yawRatio + x);
                            vecRGBA color = skyTexture.get(textureX, textY);
                            float x1 = x * lineWidth, x2 = (x + 1) * lineWidth;
                            float y1 = horizonCenterY - (y * lineWidth), y2 = horizonCenterY - ((y - 1) * lineWidth);
                            fillOpaqueRect(x1, y1, x2, y2, color);
                        }
                    }
                });
            }

            for (auto& thread : threads) thread.join();
            threads.clear();
        }

        if (horizonCenterY < windowHeight) {
            int yScans = static_cast<int>(std::ceil((windowHeight - horizonCenterY - 1) / lineWidth));

            for (unsigned int t = 0; t < threadCount; ++t) {
                threads.emplace_back([&, t] {
                    for (int y = static_cast<int>(t); y < yScans; y += static_cast<int>(threadCount)) {
                        double horizonRatio = halfWindowHeight / ((y + 1) * lineWidth);
                        for (int x = 0; x < numRays; ++x) {
                            double rayAngle = minRay + (x * rayStep);
                            double floorRayLength = (Player::BASE_CAMERA_HEIGHT + pl.verticalOffset) * horizonRatio;
                            floorRayLength /= std::cos(std::abs(rayAngle - pl.yaw));
                            double brightness = (viewDistance - floorRayLength) / viewDistance;

                            double worldX = pl.position.x + floorRayLength * std::cos(rayAngle);
                            double worldY = pl.position.y + floorRayLength * std::sin(rayAngle);
                            vecRGBA color = applyLighting(
                                floorTexture.get(worldX, worldY),
                                brightness, {worldX, worldY}, 0.0, lvl.lightList);

                            float x1 = x * lineWidth, x2 = (x + 1) * lineWidth;
                            float y1 = horizonCenterY + (y * lineWidth), y2 = y1 + lineWidth;
                            fillOpaqueRect(x1, y1, x2, y2, color);
                        }
                    }
                });
            }

            for (auto& thread : threads) thread.join();
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
        double zDistance = cameraZ - floorZ;
        vec2 sectorCenter = sect->outline.centerPoint();

        for (int y = 0; y < yScans; ++y) {
            double screenY = yStart + (y + 0.5) * yScanLineSize;
            double screenYFromHorizon = screenY - horizonCenterY;

            if (std::abs(screenYFromHorizon) < 1e-5) continue;

            double rayLength = (zDistance * halfWindowHeight) / screenYFromHorizon;
            rayLength /= cosineFactor;

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

            fillOpaqueRect(x1, y1, x2, y2, color);
        }
    }

    void underSideFill(Sector* sect, double rayAngle, double cosineFactor, int rayIndex, double yStart, double yEnd) {
        clampScreenY(yStart);
        clampScreenY(yEnd);
        if (yStart <= yEnd) return;

        const Texture* fillTexture = &textures[sect->bottomTextureFile];
        double ySpan = yStart - yEnd;
        int yScans = cappedVerticalScans(ySpan);
        double yScanLineSize = ySpan / yScans;

        double cameraZ = Player::BASE_CAMERA_HEIGHT + pl.verticalOffset;
        double underSideZ = sect->floatingHeight;
        double zDistance = underSideZ - cameraZ;
        vec2 sectorCenter = sect->outline.centerPoint();

        for (int y = 0; y < yScans; ++y) {
            double screenY = yStart - (y * yScanLineSize);
            double screenYFromHorizon = horizonCenterY - screenY;

            if (std::abs(screenYFromHorizon) < 1e-5) continue;

            double rayLength = (zDistance * halfWindowHeight) / screenYFromHorizon;
            rayLength /= cosineFactor;

            vec2 worldPos = {pl.position.x + rayLength * std::cos(rayAngle), pl.position.y + rayLength * std::sin(rayAngle)};
            vec2 worldCoord = worldPos;
            worldCoord.orbit(sectorCenter, -sect->rotation);
            worldCoord -= sectorCenter;

            double brightness = (viewDistance - rayLength) / viewDistance;
            vecRGBA color = applyLighting(
                fillTexture->get(worldCoord.x, worldCoord.y),
                brightness, worldPos, underSideZ, lvl.lightList);

            float x1 = rayIndex * lineWidth, x2 = x1 + lineWidth;
            float y1 = screenY, y2 = y1 - yScanLineSize;

            if ((y1 < 0 && y2 < 0) || (y1 >= windowHeight && y2 >= windowHeight)) continue;
            if (y2 < 0) y2 = 0;
            if (y1 > windowHeight) y1 = windowHeight;

            fillOpaqueRect(x1, y1, x2, y2, color);
        }
    }

    void basicColumnFill(const Texture* fillTexture, double x1, double x2, double yTop, double yBottom, int textureXIndex, double brightness, const vec2& worldPos, double elevationBottom, double elevationTop, double textureScaleY = 1.0, bool invertTexureY = false, double textureBaseSize = Sector::defaultWallHeight) {
        int yScans = static_cast<int>(textureBaseSize * textureScaleY);
        if (yScans < 1) yScans = 1;
        double ySpan = yBottom - yTop;
        if (ySpan <= 0.0) return;
        double yScanlineHeight = ySpan / yScans;

        for (int y = 0; y < yScans; ++y) {
            int textureYIndex = invertTexureY ? y : fillTexture->height - y;
            double t = (y + 0.5) / yScans;
            double elevation = elevationBottom + t * (elevationTop - elevationBottom);
            vecRGBA color = applyLighting(
                fillTexture->get(textureXIndex, textureYIndex),
                brightness, worldPos, elevation, lvl.lightList);
            float y1 = yTop + (y * yScanlineHeight), y2 = y1 + yScanlineHeight;
            if ((y1 < 0 && y2 < 0) || (y1 >= windowHeight && y2 >= windowHeight)) continue;
            if (y1 < 0) y1 = 0;
            if (y2 >= windowHeight) y2 = windowHeight;
            if (y2 <= y1) continue;
            fillOpaqueRect(x1, y1, x2, y2, color);
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
                            workingWall.wallHeight / workingSector.defaultWallHeight);
                    }
                    floorFill(&workingSector, rayAngle, cosineFactor, x,
                        wallInfo.midWallBottomY, static_cast<double>(windowHeight) - 1.0);
                } else {
                    basicColumnFill(&textures[workingWall.textureFile], x1, x2,
                        wallInfo.baseWallTopY, wallInfo.basewWallBottomY,
                        wallInfo.textureXIndex, brightness, wallWorldPos,
                        workingSector.floatingHeight, floorElevation,
                        workingSector.baseHeight / workingSector.defaultWallHeight);
                    underSideFill(&workingSector, rayAngle, cosineFactor, x,
                        wallInfo.basewWallBottomY, 0.0);
                }
            } else if (obj.isEntryRay) {
                if (workingWall.isVisibleWall) {
                    basicColumnFill(&textures[workingWall.textureFile], x1, x2,
                        wallInfo.midWallTopY, wallInfo.midWallBottomY,
                        wallInfo.textureXIndex, brightness, wallWorldPos,
                        floorElevation, midWallTopElevation,
                        workingWall.wallHeight / workingSector.defaultWallHeight);
                }

                basicColumnFill(&textures[workingSector.bottomTextureFile], x1, x2,
                    wallInfo.baseWallTopY, wallInfo.basewWallBottomY,
                    wallInfo.textureXIndex, brightness, wallWorldPos,
                    workingSector.floatingHeight, floorElevation,
                    workingSector.baseHeight / workingSector.defaultWallHeight);

                if (obj.pairPartnerIndex >= 0 && obj.pairPartnerIndex < static_cast<int>(view.size())) {
                    const rayPack& pair = view[obj.pairPartnerIndex];
                    if (!std::isfinite(pair.collInfo.rayLength)) continue;

                    sectorFillMetadata pairWallInfo = sectorDataCalc(&workingSector, &workingWall,
                        pair.collInfo, pair.collInfo.rayLength * cosineFactor);
                    if (pairWallInfo.basewWallBottomY < horizonCenterY) {
                        underSideFill(&workingSector, rayAngle, cosineFactor, x,
                            pairWallInfo.basewWallBottomY, wallInfo.basewWallBottomY);
                    }
                }
            } else {
                if (workingWall.isVisibleWall) {
                    basicColumnFill(&textures[workingWall.textureFile], x1, x2,
                        wallInfo.midWallTopY, wallInfo.midWallBottomY,
                        wallInfo.textureXIndex, brightness, wallWorldPos,
                        floorElevation, midWallTopElevation,
                        workingWall.wallHeight / workingSector.defaultWallHeight);
                }

                if (obj.pairPartnerIndex >= 0 && obj.pairPartnerIndex < static_cast<int>(view.size())) {
                    const rayPack& pair = view[obj.pairPartnerIndex];
                    if (!std::isfinite(pair.collInfo.rayLength)) continue;

                    sectorFillMetadata pairWallInfo = sectorDataCalc(&workingSector, &workingWall,
                        pair.collInfo, pair.collInfo.rayLength * cosineFactor);
                    if (pairWallInfo.midWallBottomY > horizonCenterY) {
                        floorFill(&workingSector, rayAngle, cosineFactor, x,
                            wallInfo.baseWallTopY, pairWallInfo.baseWallTopY);
                    }
                }
            }
        }
    }

    void rayCast() {
        minRay = pl.yaw - (0.5 * fovH);
        maxRay = minRay + fovH;
        halfFovH = (0.5 * fovH);
        rayStep = fovH / numRays;
        rayDistance = 5000;
        viewDistance = 1000;
        lineWidth = static_cast<double>(windowWidth) / numRays;

        rays = viewRays(pl, numRays, fovH, rayDistance);

        verticalShift = pl.pitch * (windowHeight / (pi * 0.25));
        horizonCenterY = (0.5 * windowHeight) - verticalShift + pl.verticalOffset;

        ensureFramebuffer();

        const unsigned int threadCount = std::max(1u, std::thread::hardware_concurrency());
        vector<std::thread> threads;

        for (unsigned int t = 0; t < threadCount; ++t) {
            threads.emplace_back([&, t] {
                RayViewWorkspace workspace;
                for (int x = static_cast<int>(t); x < static_cast<int>(rays.size()); x += static_cast<int>(threadCount)) {
                    const line2& ray = rays[x];
                    const double rayAngle = minRay + rayStep * x;
                    const double cosineFactor = std::cos(std::abs(rayAngle - pl.yaw));
                    renderRayColumn(x, ray, rayAngle, cosineFactor, workspace);
                }
            });
        }

        for (auto& thread : threads) thread.join();
    }
};
