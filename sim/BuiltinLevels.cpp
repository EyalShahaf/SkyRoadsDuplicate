#include "sim/Level.hpp"

#include "core/Log.hpp"

namespace {

Level BuildPlaceholderLevel() {
  Level lv{};
  // Empty level - just a starting platform
  lv.segments[0].startZ = 0.0f;
  lv.segments[0].length = 10.0f;
  lv.segments[0].topY = 0.0f;
  lv.segments[0].width = 8.0f;
  lv.segments[0].xOffset = 0.0f;
  lv.segmentCount = 1;
  lv.obstacleCount = 0;
  lv.totalLength = 10.0f;
  return lv;
}

} // namespace

const Level &GetLevel1() {
  static Level lv = []() {
    Level l{};
    if (!LoadLevelFromFile(l, "levels/stage1_level1.json")) {
      LOG_ERROR("Failed to load Level 1 from JSON, using empty level.");
    }
    return l;
  }();
  return lv;
}

const Level &GetLevel2() {
  static Level lv = []() {
    Level l{};
    if (!LoadLevelFromFile(l, "levels/stage1_level2.json")) {
      LOG_ERROR("Failed to load Level 2 from JSON, using empty level.");
    }
    return l;
  }();
  return lv;
}

const Level &GetLevel3() {
  static Level lv = []() {
    Level l{};
    if (!LoadLevelFromFile(l, "levels/stage1_level3.json")) {
      LOG_ERROR("Failed to load Level 3 from JSON, using empty level.");
    }
    return l;
  }();
  return lv;
}

const Level &GetLevel4() {
  static Level lv = []() {
    Level l{};
    if (!LoadLevelFromFile(l, "levels/stage2_level1.json")) {
      LOG_ERROR("Failed to load Level 4 from JSON, using empty level.");
    }
    return l;
  }();
  return lv;
}

const Level &GetLevel5() {
  static Level lv = []() {
    Level l{};
    if (!LoadLevelFromFile(l, "levels/stage2_level2.json")) {
      LOG_ERROR("Failed to load Level 5 from JSON, using empty level.");
    }
    return l;
  }();
  return lv;
}

const Level &GetLevel6() {
  static Level lv = []() {
    Level l{};
    if (!LoadLevelFromFile(l, "levels/stage2_level3.json")) {
      LOG_ERROR("Failed to load Level 6 from JSON, using empty level.");
    }
    return l;
  }();
  return lv;
}

const Level &GetLevelByIndex(int levelIndex) {
  if (levelIndex == 1)
    return GetLevel1();
  if (levelIndex == 2)
    return GetLevel2();
  if (levelIndex == 3)
    return GetLevel3();
  if (levelIndex == 4)
    return GetLevel4();
  if (levelIndex == 5)
    return GetLevel5();
  if (levelIndex == 6)
    return GetLevel6();

  static const Level placeholder = BuildPlaceholderLevel();
  return placeholder;
}
