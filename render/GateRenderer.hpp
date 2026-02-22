#pragma once

#include <raylib.h>

struct Level;
struct LevelPalette;

namespace render {

void RenderFinishLine(const Level &level, const Vector3 &playerRenderPos,
                      const LevelPalette &pal, float simTime);

void RenderStartLine(const Level &level, const Vector3 &playerRenderPos,
                     const LevelPalette &pal, float simTime);

} // namespace render
