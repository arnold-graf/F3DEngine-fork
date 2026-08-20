#pragma once

namespace DisplayConfig {
    // Internal CPU render resolution. The SDL window can be any size; presentation upscales this.
    inline constexpr int defaultWidth = 800;
    inline constexpr int defaultHeight = 500;

    // Fit the render image into the client area, preserving render aspect ratio (letterbox/pillarbox).
    inline void fitPresentationRect(
        int clientWidth, int clientHeight,
        int renderWidth, int renderHeight,
        int& outX, int& outY, int& outWidth, int& outHeight) {

        int targetWidth = clientWidth;
        int targetHeight = (targetWidth * renderHeight) / renderWidth;

        if (targetHeight > clientHeight) {
            targetHeight = clientHeight;
            targetWidth = (targetHeight * renderWidth) / renderHeight;
        }

        outX = (clientWidth - targetWidth) / 2;
        outY = (clientHeight - targetHeight) / 2;
        outWidth = targetWidth;
        outHeight = targetHeight;
    }
}
