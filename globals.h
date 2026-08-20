#pragma once

// --- STL Includes ---

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <cmath>
#include <vector>
#include <map>
#include <unordered_map>
#include <string>
#include <iostream>
#include <algorithm>
#include <format>
#include <thread>
#include <chrono>
#include <omp.h>
#include <mutex>
#include <fstream>
#include <numbers>

// --- VecSys Includes ---

#include "vecSys/base.h"
#include "vecSys/vec2.h"
#include "vecSys/shape2.h"
#include "vecSys/line2.h"
#include "vecSys/vecRGBA.h"
#include "vecSys/frameBuffer.h"
#include "displayConfig.h"

using namespace vecSys;

// --- Generic GLobals ---

using std::vector;
using std::pair;
using std::map;
using std::unordered_map;
using std::string;

int xyIndextoscalarIndex(const int& xval, const int& yval, int maxX = 256) { return (yval * maxX) + xval; }

// CPU render resolution (fixed from DisplayConfig unless snapshot overrides via applyRenderSize).
int windowWidth = DisplayConfig::defaultWidth;
int windowHeight = DisplayConfig::defaultHeight;
int halfWindowWidth = windowWidth / 2;
int halfWindowHeight = windowHeight / 2;
int numRays = windowWidth / 4;
int veritcalRays = (windowWidth / 16) * 9;

// SDL client area and scaled presentation rect within it.
int clientWidth = DisplayConfig::defaultWidth;
int clientHeight = DisplayConfig::defaultHeight;
int halfClientWidth = clientWidth / 2;
int halfClientHeight = clientHeight / 2;
int presentX = 0;
int presentY = 0;
int presentWidth = clientWidth;
int presentHeight = clientHeight;

inline void applyRenderSize(int width, int height) {
    windowWidth = width;
    windowHeight = height;
    halfWindowWidth = windowWidth / 2;
    halfWindowHeight = windowHeight / 2;
    numRays = windowWidth / 4;
    veritcalRays = (windowWidth / 16) * 9;
}

inline void applyClientSize(int width, int height) {
    clientWidth = width;
    clientHeight = height;
    halfClientWidth = clientWidth / 2;
    halfClientHeight = clientHeight / 2;
    DisplayConfig::fitPresentationRect(
        clientWidth, clientHeight, windowWidth, windowHeight,
        presentX, presentY, presentWidth, presentHeight);
}
bool keyFlags[256] = {false};
double fovH = 75*pi/180.0;
double frameDeltaSeconds = 1.0 / 120.0;
double frameScale = 1.0; // 1.0 at 120 FPS; movement/physics multiply by this
double movementRatio = 1.0;

enum class GAMESTATES {TITLE_SCREEN, GAME_PLAY, GAME_OVER, MENU_OPEN, PAUSED, LEVEL_EDITOR, OPTIONS_MENU};
GAMESTATES currentGameState = GAMESTATES::GAME_PLAY;

// --- Game Engine Includes ---

#include "Player.h"
#include "texture.h"
#include "sector.h"
#include "level.h"

// --- Complex Globals ---

Level lvl;
Player pl = {{450, 400}, 0.25 * pi, 0.0, 2.0, 400, 0, false};
vec2 mouseMovement;

vector<std::string> textureFiles;
unordered_map<std::string, Texture> textures;
