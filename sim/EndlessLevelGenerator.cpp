#include "sim/EndlessLevelGenerator.hpp"
#include "core/Rng.hpp"
#include "core/Config.hpp"
#include <algorithm>
#include <cmath>

namespace {
  constexpr float kChunkGenerationDistance = 50.0f;  // Generate chunks this far ahead
  constexpr float kMinSegmentLength = 8.0f;
  constexpr float kMaxSegmentLength = 20.0f;
  constexpr float kMinGapLength = 2.0f;
  constexpr float kMaxGapLength = 8.0f;
  constexpr float kSegmentWidthMin = 6.0f;
  constexpr float kSegmentWidthMax = 10.0f;
  constexpr float kSafeStartZone = 30.0f;  // Empty zone at start (no obstacles)
  constexpr float kMinObstacleSpacing = 3.0f;  // Minimum distance between obstacles
}

void EndlessLevelGenerator::Initialize(uint32_t seed) {
  rngState = (seed == 0) ? 1u : seed;
  currentZ = 0.0f;
  difficultyT = 0.0f;
  lastObstacleZ = -999.0f;  // Reset obstacle tracking
  
  // Clear level
  level = Level{};
  level.segmentCount = 0;
  level.obstacleCount = 0;
  
  // Create start zone
  level.start.spawnZ = 0.0f;
  level.start.gateZ = -5.0f;
  level.start.zoneDepth = 10.0f;
  level.start.style = StartStyle::NeonGate;
  level.start.width = 8.0f;
  level.start.xOffset = 0.0f;
  level.start.topY = 0.0f;
  
  // Generate initial chunk
  GenerateChunk(0.0f, 0.0f);
  
  // Assign visual variants to initial segments
  AssignVariants(level);
}

void EndlessLevelGenerator::ExtendLevel(float playerZ, float difficulty) {
  difficultyT = difficulty;
  
  // Generate new chunks if player is getting close to the end
  while (currentZ < playerZ + kChunkGenerationDistance) {
    GenerateChunk(currentZ, difficultyT);
  }
  
  // Update total length
  level.totalLength = currentZ;
}

void EndlessLevelGenerator::GenerateChunk(float startZ, float difficulty) {
  const float chunkLength = NextFloat(30.0f, 60.0f);
  float currentZ = startZ;
  
  // Determine number of segments in this chunk based on difficulty
  const int segmentCount = NextInt(3, 8);
  
  for (int i = 0; i < segmentCount && currentZ < startZ + chunkLength; ++i) {
    // Decide if this should be a gap or a segment
    const float gapProbability = 0.1f + difficulty * 0.3f;  // More gaps as difficulty increases
    const bool isGap = NextFloat01() < gapProbability && i > 0;  // Don't start with a gap
    
    if (isGap) {
      // Add a gap
      const float gapLength = NextFloat(kMinGapLength, kMaxGapLength + difficulty * 4.0f);
      currentZ += gapLength;
    } else {
      // Add a segment
      const float segmentLength = NextFloat(kMinSegmentLength, kMaxSegmentLength);
      const float segmentWidth = NextFloat(
        kSegmentWidthMin, 
        kSegmentWidthMax - difficulty * 2.0f  // Narrower segments as difficulty increases
      );
      
      // Lateral offset becomes more common with difficulty
      const float xOffset = (NextFloat01() < difficulty * 0.4f) 
        ? NextFloat(-3.0f, 3.0f) 
        : 0.0f;
      
      // Height variation
      const float topY = (NextFloat01() < 0.3f) ? NextFloat(-1.0f, 1.0f) : 0.0f;
      
      AddSegment(currentZ, segmentLength, topY, segmentWidth, xOffset);
      
      // Add obstacles based on difficulty (but not in safe start zone)
      // Reduced obstacle density for better spacing
      const float obstacleDensity = 0.08f + difficulty * 0.3f;  // Slightly reduced from before
      const int obstacleCount = (int)(segmentLength * obstacleDensity * NextFloat(0.6f, 1.2f));
      
      // Try to place obstacles with proper spacing
      int placedCount = 0;
      int attempts = 0;
      const int maxAttempts = obstacleCount * 3;  // Allow multiple attempts per obstacle
      
      while (placedCount < obstacleCount && attempts < maxAttempts) {
        attempts++;
        
        // Try a random position in the segment
        const float candidateZ = currentZ + NextFloat(1.0f, segmentLength - 1.0f);
        
        // Skip if in safe start zone
        if (candidateZ < kSafeStartZone) {
          continue;
        }
        
        // Check if this position is far enough from the last obstacle
        if (candidateZ >= lastObstacleZ + kMinObstacleSpacing) {
          const float obstacleX = NextFloat(-segmentWidth * 0.4f, segmentWidth * 0.4f) + xOffset;
          const float obstacleY = topY;
          
          // Obstacle size varies
          const float sizeX = NextFloat(0.8f, 1.5f);
          const float sizeY = NextFloat(1.2f, 2.5f);
          const float sizeZ = NextFloat(0.8f, 1.5f);
          
          // Random obstacle shape
          ObstacleShape shape = ObstacleShape::Unset;
          const float shapeRand = NextFloat01();
          if (shapeRand < 0.3f) shape = ObstacleShape::Cube;
          else if (shapeRand < 0.5f) shape = ObstacleShape::Cylinder;
          else if (shapeRand < 0.65f) shape = ObstacleShape::Pyramid;
          else if (shapeRand < 0.8f) shape = ObstacleShape::Spike;
          else if (shapeRand < 0.9f) shape = ObstacleShape::Wall;
          else shape = ObstacleShape::Sphere;
          
          AddObstacle(candidateZ, obstacleX, obstacleY, sizeX, sizeY, sizeZ, shape);
          lastObstacleZ = candidateZ;
          placedCount++;
        }
      }
      
      currentZ += segmentLength;
    }
  }
  
  currentZ = std::max(currentZ, startZ + chunkLength);
  this->currentZ = currentZ;
}

void EndlessLevelGenerator::AddSegment(float startZ, float length, float topY, float width, float xOffset) {
  if (level.segmentCount >= kMaxSegments) {
    return;  // Level is full
  }
  
  auto& seg = level.segments[level.segmentCount++];
  seg.startZ = startZ;
  seg.length = length;
  seg.topY = topY;
  seg.width = width;
  seg.xOffset = xOffset;
  seg.variantIndex = -1;  // Auto-assign
  seg.heightScale = -1.0f;  // Auto-assign
  seg.colorTint = -1;  // Auto-assign
}

void EndlessLevelGenerator::AddObstacle(float z, float x, float y, float sizeX, float sizeY, float sizeZ, ObstacleShape shape) {
  if (level.obstacleCount >= kMaxObstacles) {
    return;  // Level is full
  }
  
  auto& obs = level.obstacles[level.obstacleCount++];
  obs.z = z;
  obs.x = x;
  obs.y = y;
  obs.sizeX = sizeX;
  obs.sizeY = sizeY;
  obs.sizeZ = sizeZ;
  obs.shape = shape;
  obs.colorIndex = -1;  // Auto-assign
  obs.rotation = -999.0f;  // Auto-assign
}

float EndlessLevelGenerator::NextFloat01() {
  return core::NextFloat01(rngState);
}

float EndlessLevelGenerator::NextFloat(float min, float max) {
  return min + (max - min) * NextFloat01();
}

int EndlessLevelGenerator::NextInt(int min, int max) {
  return min + (int)(NextFloat01() * (max - min + 1));
}
