#pragma once

#include <cstdint>
#include <raylib.h>

struct LevelPalette;

namespace render {

// Initialise / re-seed the space object pool.
// Called from RenderFrame when the run seed changes.
// Also exposed via Render.hpp under the old name for backward compatibility.
void RegenerateSpaceObjects(uint32_t seed);

// Advance orbital and drift animation.
void UpdateSpaceObjects(float dt, const Vector3 &playerPos);

// Draw all space objects (stars, planets, asteroids) in the current 3D mode.
void RenderSpaceObjects(const Camera3D &camera, const LevelPalette &pal,
                        float simTime);

} // namespace render
