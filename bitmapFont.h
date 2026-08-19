#pragma once

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include "vecSys/frameBuffer.h"
#include "vecSys/vecRGBA.h"

namespace BitmapFont {
    using vecSys::FrameBuffer;
    using vecSys::vecRGBA;

    constexpr int GLYPH_W = 5;
    constexpr int GLYPH_H = 7;

    inline const uint8_t* glyphRows(char c) {
        static const uint8_t blank[7] = {0, 0, 0, 0, 0, 0, 0};
        switch (c) {
            case '0': { static const uint8_t g[7] = {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}; return g; }
            case '1': { static const uint8_t g[7] = {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}; return g; }
            case '2': { static const uint8_t g[7] = {0x0E, 0x11, 0x01, 0x06, 0x08, 0x10, 0x1F}; return g; }
            case '3': { static const uint8_t g[7] = {0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E}; return g; }
            case '4': { static const uint8_t g[7] = {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}; return g; }
            case '5': { static const uint8_t g[7] = {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E}; return g; }
            case '6': { static const uint8_t g[7] = {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E}; return g; }
            case '7': { static const uint8_t g[7] = {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}; return g; }
            case '8': { static const uint8_t g[7] = {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}; return g; }
            case '9': { static const uint8_t g[7] = {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C}; return g; }
            case '.': { static const uint8_t g[7] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C}; return g; }
            case ':': { static const uint8_t g[7] = {0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00}; return g; }
            case '%': { static const uint8_t g[7] = {0x19, 0x19, 0x02, 0x04, 0x08, 0x13, 0x13}; return g; }
            case '/': { static const uint8_t g[7] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x00, 0x00}; return g; }
            case '-': { static const uint8_t g[7] = {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00}; return g; }
            case 'F': { static const uint8_t g[7] = {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10}; return g; }
            case 'P': { static const uint8_t g[7] = {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10}; return g; }
            case 'S': { static const uint8_t g[7] = {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E}; return g; }
            case 'C': { static const uint8_t g[7] = {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E}; return g; }
            case 'U': { static const uint8_t g[7] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}; return g; }
            case 'M': { static const uint8_t g[7] = {0x11, 0x1B, 0x15, 0x11, 0x11, 0x11, 0x11}; return g; }
            case 'm': { static const uint8_t g[7] = {0x00, 0x00, 0x1A, 0x15, 0x15, 0x11, 0x11}; return g; }
            case 's': { static const uint8_t g[7] = {0x00, 0x00, 0x0E, 0x10, 0x0E, 0x01, 0x1E}; return g; }
            case 'O': { static const uint8_t g[7] = {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}; return g; }
            case 'N': { static const uint8_t g[7] = {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11}; return g; }
            case 'f': { static const uint8_t g[7] = {0x03, 0x04, 0x0E, 0x04, 0x04, 0x04, 0x04}; return g; }
            case 'c': { static const uint8_t g[7] = {0x00, 0x00, 0x0E, 0x10, 0x10, 0x11, 0x0E}; return g; }
            case 'p': { static const uint8_t g[7] = {0x00, 0x00, 0x1E, 0x11, 0x11, 0x1E, 0x10}; return g; }
            case 'a': { static const uint8_t g[7] = {0x00, 0x00, 0x0E, 0x01, 0x0F, 0x11, 0x0F}; return g; }
            default: return blank;
        }
    }

    inline void fillOpaqueRect(FrameBuffer& fb, int x1, int y1, int x2, int y2, const vecRGBA& color) {
        int xMin = std::max(0, x1);
        int xMax = std::min(fb.width, x2);
        int yMin = std::max(0, y1);
        int yMax = std::min(fb.height, y2);
        if (xMin >= xMax || yMin >= yMax) return;
        const vecRGBA c = color.clamped();
        for (int y = yMin; y < yMax; ++y) {
            const size_t row = static_cast<size_t>(y) * fb.width;
            for (int x = xMin; x < xMax; ++x) {
                fb.accumulationBuffer[row + x] = c;
            }
        }
    }

    inline void drawChar(FrameBuffer& fb, int x, int y, char c, const vecRGBA& color, int scale = 2) {
        const uint8_t* rows = glyphRows(c);
        for (int row = 0; row < GLYPH_H; ++row) {
            for (int col = 0; col < GLYPH_W; ++col) {
                if ((rows[row] >> (GLYPH_W - 1 - col)) & 1) {
                    fillOpaqueRect(fb, x + col * scale, y + row * scale,
                        x + (col + 1) * scale, y + (row + 1) * scale, color);
                }
            }
        }
    }

    inline int textWidth(const std::string& text, int scale = 2) {
        if (text.empty()) return 0;
        return static_cast<int>(text.size()) * (GLYPH_W + 1) * scale - scale;
    }

    inline int textHeight(int scale = 2) {
        return GLYPH_H * scale;
    }

    inline void drawText(FrameBuffer& fb, int x, int y, const std::string& text, const vecRGBA& color, int scale = 2) {
        int cursorX = x;
        for (char c : text) {
            drawChar(fb, cursorX, y, c, color, scale);
            cursorX += (GLYPH_W + 1) * scale;
        }
    }

    inline void drawPanelBottomRight(FrameBuffer& fb, int margin, const std::vector<std::string>& lines,
        int scale = 2, const vecRGBA& textColor = vecRGBA(0.92, 0.96, 0.92, 1.0)) {
        if (lines.empty()) return;

        int maxWidth = 0;
        for (const std::string& line : lines) {
            maxWidth = std::max(maxWidth, textWidth(line, scale));
        }

        const int lineHeight = textHeight(scale) + scale;
        const int panelW = maxWidth + margin * 2;
        const int panelH = static_cast<int>(lines.size()) * lineHeight - scale + margin * 2;
        const int panelX = fb.width - panelW - margin;
        const int panelY = fb.height - panelH - margin;

        fillOpaqueRect(fb, panelX, panelY, panelX + panelW, panelY + panelH, vecRGBA(0.05, 0.06, 0.07, 1.0));
        fillOpaqueRect(fb, panelX, panelY, panelX + panelW, panelY + 2, vecRGBA(0.35, 0.55, 0.35, 1.0));

        int textY = panelY + margin;
        for (const std::string& line : lines) {
            drawText(fb, panelX + margin, textY, line, textColor, scale);
            textY += lineHeight;
        }
    }
}
