#include "sim/Level.hpp"

#include <cmath>

int FindSegmentUnder(const Level &level, const float playerZ,
                     const float playerX, const float playerHalfW) {
  for (int i = 0; i < level.segmentCount; ++i) {
    const auto &s = level.segments[i];
    const float endZ = s.startZ + s.length;
    if (playerZ < s.startZ || playerZ > endZ)
      continue;
    const float segLeft = s.xOffset - s.width * 0.5f;
    const float segRight = s.xOffset + s.width * 0.5f;
    if (playerX + playerHalfW < segLeft || playerX - playerHalfW > segRight)
      continue;
    return i;
  }
  return -1;
}

bool CheckObstacleCollision(const Level &level, const Vector3 playerPos,
                            const float halfW, const float halfH,
                            const float halfD) {
  for (int i = 0; i < level.obstacleCount; ++i) {
    const auto &o = level.obstacles[i];
    // AABB overlap test.
    const float oMinX = o.x - o.sizeX * 0.5f;
    const float oMaxX = o.x + o.sizeX * 0.5f;
    const float oMinY = o.y;
    const float oMaxY = o.y + o.sizeY;
    const float oMinZ = o.z - o.sizeZ * 0.5f;
    const float oMaxZ = o.z + o.sizeZ * 0.5f;

    const float pMinX = playerPos.x - halfW;
    const float pMaxX = playerPos.x + halfW;
    const float pMinY = playerPos.y - halfH;
    const float pMaxY = playerPos.y + halfH;
    const float pMinZ = playerPos.z - halfD;
    const float pMaxZ = playerPos.z + halfD;

    if (pMaxX > oMinX && pMinX < oMaxX && pMaxY > oMinY && pMinY < oMaxY &&
        pMaxZ > oMinZ && pMinZ < oMaxZ) {
      return true;
    }
  }
  return false;
}

bool CheckFinishZoneCrossing(const Level &level, const float playerZ) {
  // If no finish zone (placeholder levels), fall back to totalLength check
  if (level.finish.style == FinishStyle::None) {
    return playerZ > level.totalLength;
  }
  // Player completes when crossing finish.endZ
  return playerZ > level.finish.endZ;
}

float GetSpawnZ(const Level &level) {
  // If no start zone (placeholder levels), use default spawn position
  if (level.start.style == StartStyle::None) {
    return 2.0f; // Default spawn Z
  }
  // Return spawn Z from start zone
  return level.start.spawnZ;
}
