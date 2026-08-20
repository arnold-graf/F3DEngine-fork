#include <fstream>
#include <chrono>
#include <string>

#include "globals.h"
#include "glCallBacks.h"
#include "IOUtils/saveAndLoad.h"
#include "perfOverlay.h"
#include "voxelTerrain.h"

namespace {
    void loadGameLevel(const std::string& levelPath) {
        lvl = loadLevelFromFile(levelPath);
        lvl.update();
        VoxelTerrain::initialize(lvl.terrain);
    }

    bool writeSnapshotBmp(const std::string& path) {
        if (RayCast::frameBuffer.width <= 0 || RayCast::frameBuffer.height <= 0) {
            std::cerr << "snapshot: framebuffer not initialized\n";
            return false;
        }
        try {
            RayCast::frameBuffer.writeToBMP(path);
            return true;
        } catch (const std::exception& ex) {
            std::cerr << "snapshot: " << ex.what() << "\n";
            return false;
        }
    }

    int runSnapshot(const std::string& outputPath, const std::string& levelPath) {
        windowWidth = 1024;
        windowHeight = 576;
        halfWindowWidth = windowWidth / 2;
        halfWindowHeight = windowHeight / 2;
        numRays = windowWidth / 4;
        veritcalRays = (windowWidth / 16) * 9;

        loadTextures("./gameDef/textures.json");
        loadGameLevel(levelPath);
        updateLevelLights(lvl.lightList);

        // Snapshot mode has no game loop; settle the player onto the floor first.
        for (int i = 0; i < 240; ++i) {
            pl.physicUpdate();
        }

        RayCast::drawBaseWorld();
        RayCast::rayCast();

        if (!writeSnapshotBmp(outputPath)) {
            return 1;
        }

        std::cout << "Wrote snapshot: " << outputPath << "\n";
        return 0;
    }
}

int main(int argc, char* argv[]) {
    std::string levelPath = "./gameDef/savedLevel.json";
    std::string snapshotPath;
    bool snapshotMode = false;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--snapshot" && i + 1 < argc) {
            snapshotMode = true;
            snapshotPath = argv[++i];
        } else if (arg == "--level" && i + 1 < argc) {
            levelPath = argv[++i];
        } else if (arg == "--fps-cap" && i + 1 < argc) {
            perfMonitor.fpsCap = std::max(0, std::stoi(argv[++i]));
            perfMonitor.fpsCapEnabled = perfMonitor.fpsCap > 0;
        } else if (arg == "--no-fps-cap") {
            perfMonitor.fpsCapEnabled = false;
        }
    }

    if (snapshotMode) {
        return runSnapshot(snapshotPath, levelPath);
    }

    // --- Game setup ---
    loadTextures("./gameDef/textures.json");
    loadGameLevel(levelPath);

    // --- SDL + OpenGL Setup ---
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_Window* window = SDL_CreateWindow("Test Project",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        windowWidth, windowHeight,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    if (!window) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        std::cerr << "SDL_GL_CreateContext Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_GL_SetSwapInterval(0);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    onReshape(windowWidth, windowHeight);

    SDL_ShowCursor(SDL_DISABLE);
    SDL_WarpMouseInWindow(window, halfWindowWidth, halfWindowHeight);

    bool running = true;
    SDL_Event event;
    auto lastFrameTime = std::chrono::steady_clock::now();

    while (running) {
        const auto frameStart = std::chrono::steady_clock::now();
        frameDeltaSeconds = std::chrono::duration<double>(frameStart - lastFrameTime).count();
        lastFrameTime = frameStart;
        if (frameDeltaSeconds <= 0.0 || frameDeltaSeconds > 0.25) {
            frameDeltaSeconds = 1.0 / 120.0;
        }
        frameScale = frameDeltaSeconds * 120.0;

        perfMonitor.onFrameStart(frameStart);

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = false;
                    break;

                case SDL_KEYDOWN:
                    if (!event.key.repeat)
                        keyDown(event.key.keysym.sym, window);
                    break;

                case SDL_KEYUP:
                    keyUp(event.key.keysym.sym);
                    break;

                case SDL_MOUSEBUTTONDOWN:
                    if (event.button.button == SDL_BUTTON_LEFT)
                        mouseButtonDown(window);
                    break;

                case SDL_MOUSEMOTION:
                    mouseMotion(event.motion.x, event.motion.y, window);
                    break;

                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                        onReshape(event.window.data1, event.window.data2);
                    }
                    break;
            }
        }

        display(window);
        perfMonitor.waitForFpsCap(frameStart);
    }

    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}