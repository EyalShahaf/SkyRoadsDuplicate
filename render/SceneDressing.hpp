#pragma once

#include <raylib.h>

struct LevelPalette;
struct Vector3;

namespace render {

// One-time init (idempotent — safe to call every frame).
void InitSceneDressing();

// Draw mountain silhouettes behind the level.
void RenderMountains(const LevelPalette &pal, const Vector3 &playerRenderPos);

// Draw floating decorative cubes with glow halos.
void RenderDecoCubes(const LevelPalette &pal, const Vector3 &playerRenderPos,
                     float simTime);

// Draw ambient floating particles.
void RenderAmbientDots(const LevelPalette &pal, const Vector3 &playerRenderPos,
                       float simTime);

} // namespace render
