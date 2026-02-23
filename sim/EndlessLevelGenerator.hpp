#pragma once

#include "sim/Level.hpp"
#include <cstdint>

// Endless Mode level generator that procedurally generates level chunks
// as the player progresses. Difficulty increases over time.

struct EndlessLevelGenerator {
  uint32_t rngState = 1u;
  float currentZ = 0.0f;  // Current end of generated level
  float difficultyT = 0.0f;  // Current difficulty (0.0 to 1.0+)
  Level level{};  // The dynamically generated level
  
  // Generate initial level chunk
  void Initialize(uint32_t seed);
  
  // Extend the level as player progresses
  // Should be called periodically to generate new chunks ahead of player
  void ExtendLevel(float playerZ, float difficulty);
  
  // Get the current level (for use in game)
  const Level& GetLevel() const { return level; }
  
private:
  void GenerateChunk(float startZ, float difficulty);
  void AddSegment(float startZ, float length, float topY, float width, float xOffset);
  void AddObstacle(float z, float x, float y, float sizeX, float sizeY, float sizeZ, ObstacleShape shape);
  float NextFloat01();
  float NextFloat(float min, float max);
  int NextInt(int min, int max);
};
