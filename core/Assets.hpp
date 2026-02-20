#pragma once

#include <raylib.h>

// Lightweight asset path helpers.
// Resolves paths relative to the executable's working directory,
// which CMake sets to the project root via the run scripts.
//
// Usage:
//   Model ship = LoadModel(AssetPath("models/ship.obj"));
//   Texture2D tex = LoadTexture(AssetPath("textures/track.png"));
//   Sound sfx = LoadSound(AssetPath("sounds/jump.wav"));
//   Font font = LoadFontEx(AssetPath("fonts/hud.ttf"), 32, nullptr, 0);

namespace assets {

// Returns "assets/<relative>" as a stack-allocated path (max 512 chars).
// Thread-safe: uses a local buffer, no heap allocation.
const char* Path(const char* relative);

// Convenience: check if an asset file exists before loading.
bool Exists(const char* relative);

}  // namespace assets
