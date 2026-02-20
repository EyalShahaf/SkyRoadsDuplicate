#pragma once

#include <cstdint>

struct Game;

void InitRenderer();
void CleanupRenderer();
void RenderFrame(Game& game, float alpha, float renderDt);
void RegenerateSpaceObjects(uint32_t seed);
