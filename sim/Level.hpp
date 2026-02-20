#pragma once

#include <raylib.h>

// A platform segment the player can stand on.
struct LevelSegment {
    float startZ = 0.0f;
    float length = 10.0f;
    float topY = 0.0f;          // top surface Y
    float width = 8.0f;         // X width (centered on 0)
    float xOffset = 0.0f;       // lateral shift of segment center
};

// An obstacle on a segment that kills on contact.
struct LevelObstacle {
    float z = 0.0f;
    float x = 0.0f;
    float y = 0.0f;             // base Y
    float sizeX = 1.0f;
    float sizeY = 1.5f;
    float sizeZ = 1.0f;
    int colorIndex = 0;         // 0-2 for palette deco colors
};

// Fixed-capacity level data. No heap, fully constexpr-friendly.
constexpr int kMaxSegments = 64;
constexpr int kMaxObstacles = 64;

struct Level {
    LevelSegment segments[kMaxSegments]{};
    int segmentCount = 0;
    LevelObstacle obstacles[kMaxObstacles]{};
    int obstacleCount = 0;
    float totalLength = 0.0f;   // Z extent of entire level
};

// Returns a pointer to the built-in level (static data, no alloc).
const Level& GetLevel1();
const Level& GetLevel2();
const Level& GetLevel3();
const Level& GetLevel4();

// Get level by index (1-30). Returns placeholder for unimplemented levels.
const Level& GetLevelByIndex(int levelIndex);

// Stage/Level conversion helpers (10 stages, 3 levels each = 30 total)
int GetStageFromLevelIndex(int levelIndex);  // Returns 1-10
int GetLevelInStageFromLevelIndex(int levelIndex);  // Returns 1-3
int GetLevelIndexFromStageAndLevel(int stage, int levelInStage);  // Returns 1-30

// Check if a level index is implemented (currently 1-4 are implemented)
bool IsLevelImplemented(int levelIndex);

// Check if player is over any segment. Returns segment index or -1.
int FindSegmentUnder(const Level& level, float playerZ, float playerX, float playerHalfW);

// Check if player AABB overlaps any obstacle.
bool CheckObstacleCollision(const Level& level, Vector3 playerPos, float halfW, float halfH, float halfD);
