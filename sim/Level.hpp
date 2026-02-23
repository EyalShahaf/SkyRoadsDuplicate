#pragma once

#include <raylib.h>

// A platform segment the player can stand on.
struct LevelSegment {
  float startZ = 0.0f;
  float length = 10.0f;
  float topY = 0.0f;         // top surface Y
  float width = 8.0f;        // X width (centered on 0)
  float xOffset = 0.0f;      // lateral shift of segment center
  int variantIndex = -1;     // -1 for auto, 0-7 for different visual styles
  float heightScale = -1.0f; // -1.0f for auto, otherwise visual multiplier
  int colorTint = -1;        // -1 for auto, 0-2 for color variation
};

// Obstacle shape variants
enum class ObstacleShape : int {
  Unset = -1,   // Procedurally assigned
  Cube = 0,     // Standard rectangular cube
  Cylinder = 1, // Pill-shaped (rounded)
  Pyramid = 2,  // Triangular pyramid
  Spike = 3,    // Tall thin spike
  Wall = 4,     // Wide and flat
  Sphere = 5    // Round sphere
};

// An obstacle on a segment that kills on contact.
struct LevelObstacle {
  float z = 0.0f;
  float x = 0.0f;
  float y = 0.0f; // base Y
  float sizeX = 1.0f;
  float sizeY = 1.5f;
  float sizeZ = 1.0f;
  int colorIndex = -1; // -1 for auto, 0-2 for palette deco colors
  ObstacleShape shape = ObstacleShape::Unset;
  float rotation = -999.0f; // -999.0f for auto, otherwise deg around Y
};

// Finish line style variants
enum class FinishStyle : int {
  None = 0,            // No finish line (placeholder levels)
  NeonGate = 1,        // Level 1: Simple neon gate, wide forgiving lane
  SegmentedPylons = 2, // Level 2: Segmented pylons + mild lateral offset
  PrecisionCorridor =
      3,              // Level 3: Tighter precision corridor + animated chevrons
  MultiRingPortal = 4 // Level 4: Multi-ring/portal-style finale
};

// Start line style variants (mirroring finish styles)
enum class StartStyle : int {
  None = 0,              // No start line (placeholder levels)
  NeonGate = 1,          // Level 1: Clean/simple intro gate, wide lane
  IndustrialPylons = 2,  // Level 2: Denser industrial pylons, offset accents
  PrecisionCorridor = 3, // Level 3: Precision-focused corridor-like start
  RingedLaunch = 4       // Level 4: Portal-esque or ringed launch frame
};

// Finish line zone data
struct FinishZone {
  float startZ = 0.0f; // Start of finish zone
  float endZ = 0.0f; // End of finish zone (player completes when crossing this)
  FinishStyle style = FinishStyle::None;
  float width = 8.0f;   // Finish zone width
  float xOffset = 0.0f; // Lateral offset of finish zone center
  float topY = 0.0f;    // Top Y of finish zone
  // Per-style parameters
  float ringCount = 3.0f;     // For multi-ring style
  float glowIntensity = 1.0f; // Glow intensity multiplier
  bool hasRunway = true;      // Whether to draw runway markers
};

// Start line zone data
struct StartZone {
  float spawnZ = 0.0f;     // Player spawn Z position
  float gateZ = 0.0f;      // Start gate Z position (visual marker)
  float zoneDepth = 10.0f; // Depth of start zone (for rendering)
  StartStyle style = StartStyle::None;
  float width = 8.0f;   // Start zone width
  float xOffset = 0.0f; // Lateral offset of start zone center
  float topY = 0.0f;    // Top Y of start zone
  // Per-style parameters
  float pylonSpacing = 2.0f;  // Spacing between pylons
  float glowIntensity = 1.0f; // Glow intensity multiplier
  int stripeCount = 5;        // Number of lane stripes
  float ringCount = 3.0f;     // For ringed launch style
};

// Fixed-capacity level data. No heap, fully constexpr-friendly.
constexpr int kMaxSegments = 64;
constexpr int kMaxObstacles = 64;

struct Level {
  LevelSegment segments[kMaxSegments]{};
  int segmentCount = 0;
  LevelObstacle obstacles[kMaxObstacles]{};
  int obstacleCount = 0;
  float totalLength = 0.0f; // Z extent of entire level
  FinishZone finish{}; // Finish line zone (zero-initialized = no finish for
                       // placeholders)
  StartZone
      start{}; // Start line zone (zero-initialized = no start for placeholders)
};

// Returns a pointer to the built-in level (static data, no alloc).
const Level &GetLevel1();
const Level &GetLevel2();
const Level &GetLevel3();
const Level &GetLevel4();
const Level &GetLevel5();
const Level &GetLevel6();

// Get level by index (1-30). Returns placeholder for unimplemented levels.
const Level &GetLevelByIndex(int levelIndex);

// Stage/Level conversion helpers (10 stages, 3 levels each = 30 total)
int GetStageFromLevelIndex(int levelIndex);                      // Returns 1-10
int GetLevelInStageFromLevelIndex(int levelIndex);               // Returns 1-3
int GetLevelIndexFromStageAndLevel(int stage, int levelInStage); // Returns 1-30

// Load level from a JSON file.
bool LoadLevelFromFile(Level &level, const char *relativePath);

// Check if a level index is implemented (currently 1-4 are implemented)
bool IsLevelImplemented(int levelIndex);

// Check if player is over any segment. Returns segment index or -1.
int FindSegmentUnder(const Level &level, float playerZ, float playerX,
                     float playerHalfW);

// Check if player AABB overlaps any obstacle.
bool CheckObstacleCollision(const Level &level, Vector3 playerPos, float halfW,
                            float halfH, float halfD);

// Check if player has crossed the finish zone. Returns true when player Z
// passes finish.endZ.
bool CheckFinishZoneCrossing(const Level &level, float playerZ);

// Get player spawn position from start zone. Returns spawnZ if start zone
// exists, otherwise default.
float GetSpawnZ(const Level &level);

// Assign deterministic visual variants to segments and obstacles based on their
// properties. This ensures all players see the same visual variety
// (deterministic, not random).
void AssignVariants(Level &level);

// Dynamic test to validate all levels in assets/levels are loadable.
// Returns true if all files in the directory were successfully parsed.
bool TestAllLevelsAccessibility();
