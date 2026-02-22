#pragma once

#include <raylib.h>

struct Game;
struct LevelPalette;

namespace render {

// Draw a scrolling background texture with a grid overlay.
// `textureIndex` selects from the loaded background textures (-1 = grid only).
void DrawBackgroundWithGrid(int textureIndex, float scrollOffset,
                            const LevelPalette &pal, int width, int height,
                            float alpha = 1.0f);

// Draw the full retro cockpit HUD (bottom-third panel) during gameplay.
void RenderCockpitHUD(const Game &game, const LevelPalette &pal,
                      float planarSpeed);

} // namespace render
