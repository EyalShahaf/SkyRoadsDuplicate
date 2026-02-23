#include "sim/Level.hpp"

#include "core/Assets.hpp"
#include "core/Log.hpp"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace {

// Helper to load enums from JSON (supporting both numbers and strings)
FinishStyle GetFinishStyle(const json &j, const char *key,
                           FinishStyle defaultVal) {
  if (!j.contains(key))
    return defaultVal;
  const auto &val = j[key];
  if (val.is_number())
    return static_cast<FinishStyle>(val.get<int>());
  if (val.is_string()) {
    std::string s = val.get<std::string>();
    if (s == "None")
      return FinishStyle::None;
    if (s == "NeonGate")
      return FinishStyle::NeonGate;
    if (s == "SegmentedPylons")
      return FinishStyle::SegmentedPylons;
    if (s == "PrecisionCorridor")
      return FinishStyle::PrecisionCorridor;
    if (s == "MultiRingPortal")
      return FinishStyle::MultiRingPortal;
  }
  return defaultVal;
}

StartStyle GetStartStyle(const json &j, const char *key,
                         StartStyle defaultVal) {
  if (!j.contains(key))
    return defaultVal;
  const auto &val = j[key];
  if (val.is_number())
    return static_cast<StartStyle>(val.get<int>());
  if (val.is_string()) {
    std::string s = val.get<std::string>();
    if (s == "None")
      return StartStyle::None;
    if (s == "NeonGate")
      return StartStyle::NeonGate;
    if (s == "IndustrialPylons")
      return StartStyle::IndustrialPylons;
    if (s == "PrecisionCorridor")
      return StartStyle::PrecisionCorridor;
    if (s == "RingedLaunch")
      return StartStyle::RingedLaunch;
  }
  return defaultVal;
}

ObstacleShape GetObstacleShape(const json &j, const char *key,
                               ObstacleShape defaultVal) {
  if (!j.contains(key))
    return defaultVal;
  const auto &val = j[key];
  if (val.is_number())
    return static_cast<ObstacleShape>(val.get<int>());
  if (val.is_string()) {
    std::string s = val.get<std::string>();
    if (s == "Cube")
      return ObstacleShape::Cube;
    if (s == "Cylinder")
      return ObstacleShape::Cylinder;
    if (s == "Pyramid")
      return ObstacleShape::Pyramid;
    if (s == "Spike")
      return ObstacleShape::Spike;
    if (s == "Wall")
      return ObstacleShape::Wall;
    if (s == "Sphere")
      return ObstacleShape::Sphere;
  }
  return defaultVal;
}

} // namespace

bool LoadLevelFromFile(Level &level, const char *relativePath) {
  level = {}; // Reset

  std::string fullPath = assets::Path(relativePath);
  std::ifstream f(fullPath);
  if (!f.is_open()) {
    LOG_ERROR("Failed to open level file: {}", fullPath);
    return false;
  }

  try {
    json data = json::parse(f);

    // Parse segments
    if (data.contains("segments") && data["segments"].is_array()) {
      for (const auto &s_json : data["segments"]) {
        if (level.segmentCount >= kMaxSegments)
          break;
        auto &s = level.segments[level.segmentCount++];
        s.startZ = s_json.value("startZ", 0.0f);
        s.length = s_json.value("length", 10.0f);
        s.topY = s_json.value("topY", 0.0f);
        s.width = s_json.value("width", 8.0f);
        s.xOffset = s_json.value("xOffset", 0.0f);
        s.variantIndex = s_json.value("variantIndex", -1);
        s.heightScale = s_json.value("heightScale", -1.0f);
        s.colorTint = s_json.value("colorTint", -1);
      }
    }

    // Parse obstacles
    if (data.contains("obstacles") && data["obstacles"].is_array()) {
      for (const auto &o_json : data["obstacles"]) {
        if (level.obstacleCount >= kMaxObstacles)
          break;
        auto &o = level.obstacles[level.obstacleCount++];
        o.z = o_json.value("z", 0.0f);
        o.x = o_json.value("x", 0.0f);
        o.y = o_json.value("y", 0.0f);
        o.sizeX = o_json.value("sizeX", 1.0f);
        o.sizeY = o_json.value("sizeY", 1.5f);
        o.sizeZ = o_json.value("sizeZ", 1.0f);
        o.colorIndex = o_json.value("colorIndex", -1);
        o.shape = GetObstacleShape(o_json, "shape", ObstacleShape::Unset);
        o.rotation = o_json.value("rotation", -999.0f);
      }
    }

    level.totalLength = data.value("totalLength", 0.0f);

    // Parse Start Zone
    if (data.contains("start")) {
      const auto &sz = data["start"];
      level.start.spawnZ = sz.value("spawnZ", 0.0f);
      level.start.gateZ = sz.value("gateZ", 0.0f);
      level.start.zoneDepth = sz.value("zoneDepth", 10.0f);
      level.start.style = GetStartStyle(sz, "style", StartStyle::None);
      level.start.width = sz.value("width", 8.0f);
      level.start.xOffset = sz.value("xOffset", 0.0f);
      level.start.topY = sz.value("topY", 0.0f);
      level.start.pylonSpacing = sz.value("pylonSpacing", 2.0f);
      level.start.glowIntensity = sz.value("glowIntensity", 1.0f);
      level.start.stripeCount = sz.value("stripeCount", 5);
      level.start.ringCount = sz.value("ringCount", 3.0f);
    }

    // Parse Finish Zone
    if (data.contains("finish")) {
      const auto &fz = data["finish"];
      level.finish.startZ = fz.value("startZ", 0.0f);
      level.finish.endZ = fz.value("endZ", 0.0f);
      level.finish.style = GetFinishStyle(fz, "style", FinishStyle::None);
      level.finish.width = fz.value("width", 8.0f);
      level.finish.xOffset = fz.value("xOffset", 0.0f);
      level.finish.topY = fz.value("topY", 0.0f);
      level.finish.ringCount = fz.value("ringCount", 3.0f);
      level.finish.glowIntensity = fz.value("glowIntensity", 1.0f);
      level.finish.hasRunway = fz.value("hasRunway", true);
    }

    AssignVariants(level);
    return true;
  } catch (const json::parse_error &e) {
    LOG_ERROR("JSON parse error in {}: {}", relativePath, e.what());
    return false;
  }
}

bool TestAllLevelsAccessibility() {
  namespace fs = std::filesystem;
  std::string levelsDir = assets::Path("levels");

  if (!fs::exists(levelsDir) || !fs::is_directory(levelsDir)) {
    LOG_ERROR("Levels directory not found: {}", levelsDir);
    return false;
  }

  bool allOk = true;
  int count = 0;
  int failed = 0;

  LOG_INFO("--- Starting Dynamic Level Accessibility Test ---");

  for (const auto &entry : fs::directory_iterator(levelsDir)) {
    if (entry.is_regular_file() && entry.path().extension() == ".json") {
      std::string filename = entry.path().filename().string();
      std::string relativePath = "levels/" + filename;

      Level dummy;
      if (LoadLevelFromFile(dummy, relativePath.c_str())) {
        LOG_INFO("[PASS] {}", filename);
      } else {
        LOG_ERROR("[FAIL] {}", filename);
        allOk = false;
        failed++;
      }
      count++;
    }
  }

  LOG_INFO("--- Level Test Complete: {} total, {} failed ---", count, failed);
  return allOk;
}
