#include "render/HudWidgets.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "core/Config.hpp"
#include "game/Game.hpp"
#include "render/Palette.hpp"
#include "render/RenderUtils.hpp"

// External texture state owned by Render.cpp
namespace render {
extern bool g_backgroundTexturesLoaded;
extern Texture2D g_backgroundTextures[4];
} // namespace render

namespace render {

// ─── Internal helpers
// ─────────────────────────────────────────────────────────

static void DrawBeveledRectangle(int x, int y, int width, int height,
                                 Color base, int bevel = 2) {
  DrawRectangle(x, y, width, height, base);

  const Color hi =
      Color{static_cast<unsigned char>(std::min(255, base.r + 40)),
            static_cast<unsigned char>(std::min(255, base.g + 40)),
            static_cast<unsigned char>(std::min(255, base.b + 40)), 255};
  const Color sh =
      Color{static_cast<unsigned char>(std::max(0, base.r - 40)),
            static_cast<unsigned char>(std::max(0, base.g - 40)),
            static_cast<unsigned char>(std::max(0, base.b - 40)), 255};

  DrawRectangle(x, y, width, bevel, hi);
  DrawRectangle(x, y, bevel, height, hi);
  DrawRectangle(x, y + height - bevel, width, bevel, sh);
  DrawRectangle(x + width - bevel, y, bevel, height, sh);
}

static void DrawLEDLight(int cx, int cy, int radius, Color color, bool active) {
  if (!active) {
    DrawCircle(cx, cy, radius,
               Color{static_cast<unsigned char>(color.r / 4),
                     static_cast<unsigned char>(color.g / 4),
                     static_cast<unsigned char>(color.b / 4), 255});
    return;
  }
  for (int i = 3; i >= 0; --i) {
    const float alpha = 0.3f / (static_cast<float>(i) + 1.0f);
    DrawCircle(cx, cy, radius + static_cast<float>(i) * 2.0f,
               Color{color.r, color.g, color.b,
                     static_cast<unsigned char>(255 * alpha)});
  }
  DrawCircle(cx, cy, radius, color);
  DrawCircle(cx, cy, radius * 0.5f, Color{255, 255, 255, 200});
}

// 7-segment patterns: top, upper-right, lower-right, bottom, lower-left,
// upper-left, middle
static constexpr uint8_t k7Seg[10] = {
    0b1111110, // 0
    0b0110000, // 1
    0b1101101, // 2
    0b1111001, // 3
    0b0110011, // 4
    0b1011011, // 5
    0b1011111, // 6
    0b1110000, // 7
    0b1111111, // 8
    0b1111011  // 9
};

// Draw all active segments for one digit at (x,y) with given color.
// Extracted helper eliminates the duplicated glow+main draw logic.
static void DrawSegments(uint8_t pattern, int x, int y, int digitSize,
                         int thickness, Color color) {
  const int half = digitSize / 2;
  const int cx = x + half;
  const int cy = y + half;
  const int len = half;

  if (pattern & 0b1000000)
    DrawRectangle(cx - len / 2, y, len, thickness, color); // top
  if (pattern & 0b0100000)
    DrawRectangle(cx + len / 2, y + thickness, thickness, len,
                  color); // upper-right
  if (pattern & 0b0010000)
    DrawRectangle(cx + len / 2, cy, thickness, len, color); // lower-right
  if (pattern & 0b0001000)
    DrawRectangle(cx - len / 2, y + digitSize - thickness, len, thickness,
                  color); // bottom
  if (pattern & 0b0000100)
    DrawRectangle(cx - len / 2 - thickness, cy, thickness, len,
                  color); // lower-left
  if (pattern & 0b0000010)
    DrawRectangle(cx - len / 2 - thickness, y + thickness, thickness, len,
                  color); // upper-left
  if (pattern & 0b0000001)
    DrawRectangle(cx - len / 2, cy - thickness / 2, len, thickness,
                  color); // middle
}

static void Draw7SegmentDigit(int x, int y, char digit, int size, Color color,
                              Color glowColor) {
  if (digit < '0' || digit > '9')
    return;
  const uint8_t pattern = k7Seg[digit - '0'];
  const int thickness = std::max(2, size / 8);

  // Glow passes (offsets 0,1,2 with diminishing alpha)
  for (int glow = 0; glow < 3; ++glow) {
    const Color gc = Color{glowColor.r, glowColor.g, glowColor.b,
                           static_cast<unsigned char>(80 / (glow + 1))};
    DrawSegments(pattern, x + glow, y + glow, size, thickness, gc);
  }
  // Main pass
  DrawSegments(pattern, x, y, size, thickness, color);
}

static void Draw7SegmentNumber(int x, int y, const char *number, int digitSize,
                               int spacing, Color color, Color glowColor) {
  int curX = x;
  for (int i = 0; number[i] != '\0'; ++i) {
    if (number[i] >= '0' && number[i] <= '9') {
      Draw7SegmentDigit(curX, y, number[i], digitSize, color, glowColor);
      curX += digitSize + spacing;
    } else if (number[i] == ' ') {
      curX += digitSize / 2;
    }
  }
}

// Segmented circular gauge with radial fill and tick marks.
static void DrawSegmentedGauge(int cx, int cy, float radius, int segCount,
                               float fill, Color fillColor, Color emptyColor) {
  const float fillAngle = fill * 360.0f;

  // Filled pie slice
  if (fill > 0.0f) {
    const int tris = static_cast<int>(fillAngle / 2.0f) + 1;
    for (int i = 0; i < tris; ++i) {
      // Pre-compute angles once per triangle pair — avoids redundant trig
      const float a1 = fillAngle * static_cast<float>(i) /
                       static_cast<float>(tris) * DEG2RAD;
      const float a2 = fillAngle * static_cast<float>(i + 1) /
                       static_cast<float>(tris) * DEG2RAD;
      DrawTriangle(
          Vector2{static_cast<float>(cx), static_cast<float>(cy)},
          Vector2{cx + std::cos(a1) * radius, cy - std::sin(a1) * radius},
          Vector2{cx + std::cos(a2) * radius, cy - std::sin(a2) * radius},
          fillColor);
    }
  }

  // Tick marks
  const float innerR = radius * 0.75f;
  const float angleStep = 360.0f / static_cast<float>(segCount);
  for (int i = 0; i < segCount; ++i) {
    const float angle = static_cast<float>(i) * angleStep * DEG2RAD;
    const float cosA = std::cos(angle);
    const float sinA = std::sin(angle);
    const float thickness = (i % 6 == 0) ? 2.5f : 1.5f;
    const Color col = (static_cast<float>(i) * angleStep < fillAngle)
                          ? fillColor
                          : emptyColor;
    DrawLineEx(Vector2{cx + cosA * innerR, cy - sinA * innerR},
               Vector2{cx + cosA * radius, cy - sinA * radius}, thickness, col);
  }

  DrawCircleLines(cx, cy, radius, Color{100, 100, 150, 255});
  DrawCircleLines(cx, cy, innerR, Color{100, 100, 150, 255});
}

// Draw a grid pattern covering (0,0)→(width,height).
static void DrawGridOverlay(const LevelPalette &pal, int width, int height,
                            float alpha) {
  const int gridSpacing = 40;
  const Color gridColor = Fade(pal.gridLine, 0.4f * alpha);
  for (int x = 0; x <= width; x += gridSpacing)
    DrawLine(x, 0, x, height, gridColor);
  for (int y = 0; y <= height; y += gridSpacing)
    DrawLine(0, y, width, y, gridColor);

  const int diagSpacing = 60;
  const Color diagColor = Fade(pal.gridLine, 0.15f * alpha);
  for (int i = -height; i < width + height; i += diagSpacing) {
    DrawLine(i, 0, i + height, height, diagColor);
    DrawLine(i, 0, i - height, height, diagColor);
  }
}

// ─── Public functions
// ─────────────────────────────────────────────────────────

void DrawBackgroundWithGrid(int textureIndex, float scrollOffset,
                            const LevelPalette &pal, int width, int height,
                            float alpha) {
  if (!g_backgroundTexturesLoaded || textureIndex < 0 || textureIndex >= 4 ||
      g_backgroundTextures[textureIndex].id == 0) {
    DrawGridOverlay(pal, width, height, alpha);
    return;
  }

  const Texture2D &tex = g_backgroundTextures[textureIndex];
  const float texScale =
      static_cast<float>(height) / static_cast<float>(tex.height);
  const float texW = static_cast<float>(tex.width) * texScale;
  const float scrollX = std::fmod(scrollOffset, texW);

  for (int x = -1; x <= 1; ++x) {
    const float drawX = scrollX + static_cast<float>(x) * texW;
    DrawTexturePro(
        tex,
        Rectangle{0.0f, 0.0f, static_cast<float>(tex.width),
                  static_cast<float>(tex.height)},
        Rectangle{drawX, 0.0f, texW, static_cast<float>(height)},
        Vector2{0.0f, 0.0f}, 0.0f,
        Color{255, 255, 255, static_cast<unsigned char>(255 * alpha)});
  }

  DrawGridOverlay(pal, width, height, alpha);
}

void RenderCockpitHUD(const Game &game, const LevelPalette &pal,
                      float planarSpeed) {
  const int hudStartY = cfg::kScreenHeight * 2 / 3;
  const int hudHeight = cfg::kScreenHeight - hudStartY;
  const int centerX = cfg::kScreenWidth / 2;

  // Console base
  DrawRectangleGradientV(0, hudStartY, cfg::kScreenWidth, hudHeight,
                         Color{50, 60, 70, 255}, Color{40, 50, 60, 255});
  DrawRectangleLinesEx(Rectangle{0.0f, static_cast<float>(hudStartY),
                                 static_cast<float>(cfg::kScreenWidth),
                                 static_cast<float>(hudHeight)},
                       2.0f, Color{30, 35, 40, 255});

  // ── Left panel: GRAV-C METER ──────────────────────────────────────────────
  const int leftPanelX = 40;
  const int leftPanelY = hudStartY + 20;
  const int panelW = 180;
  const int panelH = 100;
  const int displayW = panelW - 20;
  const int displayH = 50;

  DrawBeveledRectangle(leftPanelX, leftPanelY, panelW, panelH,
                       Color{30, 35, 40, 255}, 3);

  const float gravValue = std::abs(game.player.velocity.y) * 10.0f;
  DrawLEDLight(leftPanelX + 15, leftPanelY - 8, 5, Color{0, 255, 0, 255},
               gravValue > 1.0f);

  const int displayX = leftPanelX + 10;
  const int displayY = leftPanelY + 15;
  DrawRectangle(displayX, displayY, displayW, displayH, Color{0, 0, 0, 255});
  DrawRectangleLinesEx(
      Rectangle{static_cast<float>(displayX), static_cast<float>(displayY),
                static_cast<float>(displayW), static_cast<float>(displayH)},
      2.0f, Color{40, 40, 40, 255});

  char gravText[32];
  std::snprintf(gravText, sizeof(gravText), "%.0f", gravValue);
  const int digitSize = 32;
  const int numberW = static_cast<int>(std::strlen(gravText)) * (digitSize + 4);
  const int numberX = displayX + (displayW - numberW) / 2;
  const int numberY = displayY + (displayH - digitSize) / 2;
  Draw7SegmentNumber(numberX, numberY, gravText, digitSize, 4,
                     Color{0, 255, 100, 255}, Color{0, 255, 150, 255});
  DrawText("GRAV-C METER", leftPanelX + 20, leftPanelY + 75, 12, pal.uiText);

  // ── Right panel: JUMP-O MASTER ────────────────────────────────────────────
  const int rightPanelX = cfg::kScreenWidth - panelW - 40;
  const int rightPanelY = leftPanelY;

  DrawBeveledRectangle(rightPanelX, rightPanelY, panelW, panelH,
                       Color{30, 35, 40, 255}, 3);

  const bool jumpLightOn =
      game.player.dashTimer > 0.0f || !game.player.grounded;
  DrawLEDLight(rightPanelX + 15, rightPanelY - 8, 5, Color{0, 255, 0, 255},
               jumpLightOn);

  const int rDisplayX = rightPanelX + 10;
  const int rDisplayY = rightPanelY + 15;
  DrawRectangle(rDisplayX, rDisplayY, displayW, displayH, Color{0, 0, 0, 255});
  DrawRectangleLinesEx(
      Rectangle{static_cast<float>(rDisplayX), static_cast<float>(rDisplayY),
                static_cast<float>(displayW), static_cast<float>(displayH)},
      2.0f, Color{40, 40, 40, 255});

  const char *jumpStatus = "IDLE";
  if (game.player.dashTimer > 0.0f)
    jumpStatus = "DASH";
  else if (!game.player.grounded)
    jumpStatus = "JUMPING";
  else if (game.player.jumpBufferTimer > 0.0f)
    jumpStatus = "READY";

  const int statusW = MeasureText(jumpStatus, 20);
  const int statusX = rDisplayX + (displayW - statusW) / 2;
  for (int i = 0; i < 3; ++i) {
    DrawText(jumpStatus, statusX + i, rDisplayY + 12 + i, 20,
             Color{0, 255, 100, static_cast<unsigned char>(60 / (i + 1))});
  }
  DrawText(jumpStatus, statusX, rDisplayY + 12, 20, Color{0, 255, 100, 255});
  DrawText("JUMP-O MASTER", rightPanelX + 15, rightPanelY + 75, 12, pal.uiText);

  // ── Central gauge: O2 / FUEL / SPEED ─────────────────────────────────────
  const int gaugeCX = centerX;
  const int gaugeCY = hudStartY + 60;
  const float gaugeR = 55.0f;
  const int segCount = 24;

  DrawCircleLines(gaugeCX, gaugeCY, gaugeR, Color{100, 100, 150, 255});
  DrawCircleLines(gaugeCX, gaugeCY, gaugeR * 0.7f, Color{100, 100, 150, 255});

  const float speedNorm = Clamp01(planarSpeed / cfg::kThrottleSpeedMax);
  const float fuelNorm =
      1.0f - (game.player.dashCooldownTimer / cfg::kDashCooldown);
  const float o2Norm = Clamp01(game.runTime / 300.0f);

  const Color fillCol = Color{200, 100, 255, 255};
  const Color emptyCol = Color{40, 40, 80, 255};

  DrawSegmentedGauge(gaugeCX, gaugeCY, gaugeR, segCount, speedNorm, fillCol,
                     emptyCol);
  DrawSegmentedGauge(gaugeCX, gaugeCY, gaugeR * 0.82f, segCount, fuelNorm,
                     fillCol, emptyCol);
  DrawSegmentedGauge(gaugeCX, gaugeCY, gaugeR * 0.64f, segCount, o2Norm,
                     fillCol, emptyCol);

  DrawText("O2", gaugeCX - 10, gaugeCY - 18, 14, pal.uiText);
  DrawText("FUEL", gaugeCX - 18, gaugeCY - 3, 12, pal.uiText);
  DrawText("SPEED", gaugeCX - 20, gaugeCY + 12, 12, pal.uiText);

  // ── Throttle bar ──────────────────────────────────────────────────────────
  const int throttleBarX = centerX - 150;
  const int throttleBarY = hudStartY + 140;
  const int throttleBarW = 300;
  const int throttleBarH = 20;

  DrawRectangle(throttleBarX, throttleBarY, throttleBarW, throttleBarH,
                Color{20, 25, 30, 255});
  DrawRectangleLinesEx(Rectangle{static_cast<float>(throttleBarX),
                                 static_cast<float>(throttleBarY),
                                 static_cast<float>(throttleBarW),
                                 static_cast<float>(throttleBarH)},
                       2.0f, Color{80, 90, 100, 255});

  const int fillW = static_cast<int>(game.throttle * throttleBarW);
  if (fillW > 0) {
    DrawRectangle(throttleBarX, throttleBarY, fillW, throttleBarH,
                  Color{200, 100, 255, 255});
    DrawRectangle(throttleBarX, throttleBarY, fillW, throttleBarH / 3,
                  Color{255, 150, 255, 255});
    DrawRectangleLinesEx(Rectangle{static_cast<float>(throttleBarX),
                                   static_cast<float>(throttleBarY),
                                   static_cast<float>(fillW),
                                   static_cast<float>(throttleBarH)},
                         1.0f, Color{255, 200, 255, 255});
  }
  DrawText("THROTTLE", centerX - 35, throttleBarY - 18, 14, pal.uiText);

  // ── Side-panel decorative strips ─────────────────────────────────────────
  DrawRectangle(0, hudStartY, 20, hudHeight, Color{40, 45, 50, 255});
  DrawRectangle(cfg::kScreenWidth - 20, hudStartY, 20, hudHeight,
                Color{40, 45, 50, 255});
  for (int i = 0; i < 3; ++i) {
    DrawRectangle(10 + i * 6, hudStartY + 10, 4, 4, Color{100, 120, 140, 255});
    DrawRectangle(cfg::kScreenWidth - 14 - i * 6, hudStartY + 10, 4, 4,
                  Color{100, 120, 140, 255});
  }
}

} // namespace render
