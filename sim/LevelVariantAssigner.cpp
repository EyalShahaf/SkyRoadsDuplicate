#include "sim/Level.hpp"

#include <cstdint>

namespace {

void AssignVariantsHelper(Level &lv) {
  // Assign segment variants
  for (int i = 0; i < lv.segmentCount; ++i) {
    auto &seg = lv.segments[i];

    // Create deterministic hash from segment properties
    // Using integer conversion to ensure consistent hashing
    uint32_t hash = static_cast<uint32_t>(seg.startZ * 10.0f) ^
                    static_cast<uint32_t>(seg.width * 100.0f) ^
                    static_cast<uint32_t>(seg.topY * 50.0f) ^
                    static_cast<uint32_t>(seg.length * 5.0f) ^
                    static_cast<uint32_t>(i * 17u);

    // Variant selection: 0=Standard, 1=Thin, 2=Thick, 3=Wide, 4=Narrow,
    // 5=Glowing, 6=Matte, 7=Striped Use width and height to influence variant
    // choice
    if (seg.variantIndex == -1) {
      if (seg.width < 5.0f) {
        seg.variantIndex = 4; // Narrow variant for narrow segments
      } else if (seg.width > 7.0f) {
        seg.variantIndex = 3; // Wide variant for wide segments
      } else if (seg.topY > 1.0f) {
        seg.variantIndex = 2; // Thick variant for elevated segments
      } else {
        seg.variantIndex = (hash % 8); // Cycle through all variants
      }
    }

    // Height scale: vary visual height (0.7 to 1.2)
    if (seg.heightScale < 0.0f) {
      seg.heightScale = 0.7f + ((hash / 8) % 6) * 0.1f;
    }

    // Color tint: 0-2 for slight color variations
    if (seg.colorTint == -1) {
      seg.colorTint = (hash / 48) % 3;
    }
  }

  // Assign obstacle variants
  for (int i = 0; i < lv.obstacleCount; ++i) {
    auto &obs = lv.obstacles[i];

    // Create deterministic hash from obstacle properties
    uint32_t hash = static_cast<uint32_t>(obs.z * 10.0f) ^
                    static_cast<uint32_t>(obs.x * 100.0f) ^
                    static_cast<uint32_t>(obs.y * 50.0f) ^
                    static_cast<uint32_t>(obs.sizeY * 30.0f) ^
                    static_cast<uint32_t>(i * 23u);

    // Shape selection based on size and position
    if (obs.shape == ObstacleShape::Unset) {
      if (obs.sizeY > 2.0f) {
        obs.shape = ObstacleShape::Spike; // Tall obstacles = spikes
      } else if (obs.sizeX > obs.sizeZ * 1.5f) {
        obs.shape = ObstacleShape::Wall; // Wide obstacles = walls
      } else if (obs.sizeX < obs.sizeZ * 0.7f) {
        obs.shape = ObstacleShape::Cylinder; // Pill-shaped for narrow obstacles
      } else {
        // Cycle through shapes based on hash
        obs.shape = static_cast<ObstacleShape>((hash % 6));
      }
    }

    // Rotation: 0, 45, 90, or 135 degrees
    if (obs.rotation < -360.0f) {
      obs.rotation = static_cast<float>((hash / 6) % 4) * 45.0f;
    }

    if (obs.colorIndex == -1) {
      obs.colorIndex = (hash / 24) % 3;
    }
  }
}

} // namespace

void AssignVariants(Level &level) { AssignVariantsHelper(level); }
