#pragma once

#include <chrono>
#include <cmath>
#include <format>
#include <string>
#include <thread>
#include <vector>

#if defined(__APPLE__) || defined(__linux__)
#include <sys/resource.h>
#include <sys/time.h>
#endif

#include "bitmapFont.h"
#include "globals.h"

struct PerfMonitor {
    bool overlayVisible = false;
    int fpsCap = 120;
    bool fpsCapEnabled = true;

    double fpsSmoothed = 0.0;
    double frameMsSmoothed = 0.0;
    double cpuPercentSmoothed = 0.0;
    double cpuPerCoreSmoothed = 0.0;

    std::chrono::steady_clock::time_point lastFrameStart {};
    std::chrono::steady_clock::time_point lastCpuSampleTime {};
    double lastCpuSeconds = 0.0;
    bool hasFrameSample = false;
    bool hasCpuSample = false;

    static double processCpuSeconds() {
#if defined(__APPLE__) || defined(__linux__)
        rusage usage {};
        if (getrusage(RUSAGE_SELF, &usage) != 0) {
            return 0.0;
        }
        return static_cast<double>(usage.ru_utime.tv_sec) + usage.ru_utime.tv_usec / 1e6
             + static_cast<double>(usage.ru_stime.tv_sec) + usage.ru_stime.tv_usec / 1e6;
#else
        return 0.0;
#endif
    }

    static double secondsSince(const std::chrono::steady_clock::time_point& start,
                               const std::chrono::steady_clock::time_point& end) {
        return std::chrono::duration<double>(end - start).count();
    }

    void onFrameStart(const std::chrono::steady_clock::time_point& frameStart) {
        if (hasFrameSample) {
            const double frameSeconds = secondsSince(lastFrameStart, frameStart);
            if (frameSeconds > 0.0) {
                const double instantFps = 1.0 / frameSeconds;
                const double instantMs = frameSeconds * 1000.0;
                const double alpha = 0.12;
                fpsSmoothed = fpsSmoothed <= 0.0 ? instantFps : fpsSmoothed * (1.0 - alpha) + instantFps * alpha;
                frameMsSmoothed = frameMsSmoothed <= 0.0 ? instantMs : frameMsSmoothed * (1.0 - alpha) + instantMs * alpha;
            }
        }

        lastFrameStart = frameStart;

        const double cpuNow = processCpuSeconds();
        if (hasCpuSample) {
            const double cpuDelta = cpuNow - lastCpuSeconds;
            const double wallDelta = secondsSince(lastCpuSampleTime, frameStart);
            if (wallDelta > 0.0) {
                const double instantCpu = (cpuDelta / wallDelta) * 100.0;
                const unsigned int cores = std::max(1u, std::thread::hardware_concurrency());
                const double instantPerCore = instantCpu / static_cast<double>(cores);
                const double alpha = 0.15;
                cpuPercentSmoothed = cpuPercentSmoothed <= 0.0 ? instantCpu
                    : cpuPercentSmoothed * (1.0 - alpha) + instantCpu * alpha;
                cpuPerCoreSmoothed = cpuPerCoreSmoothed <= 0.0 ? instantPerCore
                    : cpuPerCoreSmoothed * (1.0 - alpha) + instantPerCore * alpha;
            }
        }

        lastCpuSeconds = cpuNow;
        lastCpuSampleTime = frameStart;
        hasFrameSample = true;
        hasCpuSample = true;
    }

    void waitForFpsCap(const std::chrono::steady_clock::time_point& frameStart) const {
        if (!fpsCapEnabled || fpsCap <= 0) return;
        const auto targetFrame = std::chrono::duration<double>(1.0 / static_cast<double>(fpsCap));
        const auto targetEnd = frameStart + std::chrono::duration_cast<std::chrono::steady_clock::duration>(targetFrame);
        const auto now = std::chrono::steady_clock::now();
        if (now < targetEnd) {
            std::this_thread::sleep_for(targetEnd - now);
        }
    }

    void draw(vecSys::FrameBuffer& framebuffer) const {
        if (!overlayVisible) return;

        const std::string fpsLine = std::format("FPS {:4.1f}", fpsSmoothed > 0.0 ? fpsSmoothed : 0.0);
        const std::string frameLine = std::format("frm {:5.2f}ms", frameMsSmoothed > 0.0 ? frameMsSmoothed : 0.0);
        const std::string cpuLine = std::format("CPU {:4.0f}%", cpuPercentSmoothed > 0.0 ? cpuPercentSmoothed : 0.0);
        const std::string coreLine = std::format("cor {:4.1f}%/c", cpuPerCoreSmoothed > 0.0 ? cpuPerCoreSmoothed : 0.0);
        const std::string capLine = fpsCapEnabled
            ? std::format("cap {:3d}", fpsCap)
            : std::string("cap OFF");

        BitmapFont::drawPanelBottomRight(framebuffer, 8, {fpsLine, frameLine, cpuLine, coreLine, capLine});
    }

    void cycleFpsCap() {
        if (fpsCapEnabled && fpsCap == 60) {
            fpsCap = 120;
        } else if (fpsCapEnabled && fpsCap == 120) {
            fpsCapEnabled = false;
        } else {
            fpsCapEnabled = true;
            fpsCap = 60;
        }
    }

    void toggleOverlay() {
        overlayVisible = !overlayVisible;
    }
};

inline PerfMonitor perfMonitor;
