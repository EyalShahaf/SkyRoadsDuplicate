#include "render/SpaceObjects.hpp"

#include <cmath>

#include "core/Config.hpp"
#include "render/Palette.hpp"
#include "render/RenderUtils.hpp"

namespace render {

// ─── Types
// ────────────────────────────────────────────────────────────────────

enum class SpaceObjectType { Star, Planet, Asteroid, Comet, Nebula };

struct SpaceObject {
  Vector3 basePos;
  Vector3 currentPos;
  float brightness;
  float size;
  SpaceObjectType type;

  // Linear drift
  Vector3 velocity;

  // Orbital motion
  float orbitalRadius;
  float orbitalSpeed;
  float orbitalAngle;
  // Pre-computed orbital basis
  Vector3 orbitalRight;
  Vector3 orbitalForward;

  float parallaxFactor;

  // Visual
  int textureIndex;
  Color tint;
  float pulseRate;
  float pulseOffset;
};

// ─── State
// ────────────────────────────────────────────────────────────────────

static bool g_inited = false;
static uint32_t g_seed = 0u;

constexpr int kMaxSpaceObjects = cfg::kStarCount + 100;
static SpaceObject g_objects[kMaxSpaceObjects];
static int g_count = 0;

// Loaded via InitRenderer — forwarded in as extern to avoid a circular dep.
// Declared extern so SpaceObjects.cpp can reference it from Render.cpp's TU.
extern bool g_texturesLoaded;
extern Texture2D g_planetTextures[10];

// ─── Internal helpers
// ─────────────────────────────────────────────────────────

// Build an orthonormal basis for the orbital plane around `axis`.
// Returns {right, forward} stored in the SpaceObject.
static void ComputeOrbitalBasis(SpaceObject &obj, const Vector3 &axis) {
  Vector3 up = {0.0f, 1.0f, 0.0f};

  // right = axis × up  (manual cross product)
  Vector3 right = {axis.y * up.z - axis.z * up.y, axis.z * up.x - axis.x * up.z,
                   axis.x * up.y - axis.y * up.x};
  float rightLen =
      std::sqrt(right.x * right.x + right.y * right.y + right.z * right.z);
  if (rightLen < 0.1f) {
    right = {1.0f, 0.0f, 0.0f};
    rightLen = 1.0f;
  }
  right = {right.x / rightLen, right.y / rightLen, right.z / rightLen};

  // forward = right × axis
  Vector3 forward = {right.y * axis.z - right.z * axis.y,
                     right.z * axis.x - right.x * axis.z,
                     right.x * axis.y - right.y * axis.x};
  const float fLen = std::sqrt(forward.x * forward.x + forward.y * forward.y +
                               forward.z * forward.z);
  if (fLen > 0.001f) {
    forward = {forward.x / fLen, forward.y / fLen, forward.z / fLen};
  } else {
    forward = {0.0f, 0.0f, 1.0f};
  }

  obj.orbitalRight = right;
  obj.orbitalForward = forward;
}

// ─── Public API
// ───────────────────────────────────────────────────────────────

void RegenerateSpaceObjects(uint32_t seed) {
  g_seed = seed;
  g_count = 0;

  uint32_t rng = seed;

  // Stars (Layered)
  for (int i = 0; i < cfg::kStarCount && g_count < kMaxSpaceObjects; ++i) {
    auto &obj = g_objects[g_count++];
    obj.type = SpaceObjectType::Star;

    const uint32_t s = static_cast<uint32_t>(i) * 7919u + seed;

    // Distribute stars in 3 depth layers
    float layer = HashFloat01(s + 11u);
    float rangeScale = (layer < 0.3f) ? 0.5f : (layer < 0.8f) ? 1.0f : 1.5f;
    float pFactor = (layer < 0.3f) ? 0.2f : (layer < 0.8f) ? 0.1f : 0.02f;

    obj.basePos = Vector3{
        (HashFloat01(s) - 0.5f) * 2.0f * cfg::kStarFieldRadius * rangeScale,
        HashFloat01(s + 1u) * cfg::kStarFieldHeight * rangeScale + 2.0f,
        (HashFloat01(s + 2u) - 0.3f) * cfg::kStarFieldDepth * rangeScale};
    obj.currentPos = obj.basePos;
    obj.brightness = 0.2f + 0.8f * HashFloat01(s + 3u);
    obj.size = (0.04f + HashFloat01(s + 4u) * 0.15f) * (2.0f - pFactor * 5.0f);

    if (HashFloat01(s + 5u) > 0.85f) {
      obj.velocity = {(HashFloat01(s + 6u) - 0.5f) * 0.3f,
                      (HashFloat01(s + 7u) - 0.5f) * 0.2f,
                      (HashFloat01(s + 8u) - 0.5f) * 0.1f};
    } else {
      obj.velocity = {0.0f, 0.0f, 0.0f};
    }

    obj.parallaxFactor = pFactor;
    obj.orbitalRadius = 0.0f;
    obj.orbitalSpeed = 0.0f;
    obj.orbitalAngle = 0.0f;
    obj.textureIndex = 0;
    obj.tint = WHITE;
    obj.pulseRate = 1.0f + HashFloat01(s + 10u) * 2.0f;
    obj.pulseOffset = HashFloat01(s + 11u) * 10.0f;
  }

  // Comets
  for (int i = 0; i < cfg::kCometCount && g_count < kMaxSpaceObjects; ++i) {
    auto &obj = g_objects[g_count++];
    obj.type = SpaceObjectType::Comet;

    const uint32_t s = static_cast<uint32_t>(i) * 3137u + seed + 5000u;
    const float ang = HashFloat01(s) * 2.0f * kPi;
    const float dist = cfg::kStarFieldRadius * 0.8f;

    obj.basePos = {std::cos(ang) * dist, 15.0f + HashFloat01(s + 1u) * 40.0f,
                   (HashFloat01(s + 2u) - 0.5f) * cfg::kStarFieldDepth};
    obj.currentPos = obj.basePos;
    obj.brightness = 0.9f;
    obj.size = 0.4f + HashFloat01(s + 3u) * 0.3f;

    // Fast transversal movement
    obj.velocity = {(HashFloat01(s + 4u) - 0.5f) * 15.0f,
                    (HashFloat01(s + 5u) - 0.5f) * 5.0f,
                    -5.0f - HashFloat01(s + 6u) * 10.0f};
    obj.parallaxFactor = 0.03f;
    obj.tint = Color{180, 220, 255, 255};
    obj.orbitalRadius = 0.0f;
  }

  // Nebulae (Far distant features)
  for (int i = 0; i < cfg::kNebulaCount && g_count < kMaxSpaceObjects; ++i) {
    auto &obj = g_objects[g_count++];
    obj.type = SpaceObjectType::Nebula;

    const uint32_t s = static_cast<uint32_t>(i) * 5501u + seed + 6000u;
    const float ang = HashFloat01(s) * 2.0f * kPi;
    const float dist = cfg::kFarFieldRadius;
    const float h = -10.0f + HashFloat01(s + 1u) * cfg::kFarFieldHeight;

    obj.basePos = {std::sin(ang) * dist, h, std::cos(ang) * dist};
    obj.currentPos = obj.basePos;
    obj.brightness = 0.2f + HashFloat01(s + 2u) * 0.3f;
    obj.size = 60.0f + HashFloat01(s + 3u) * 80.0f;
    obj.textureIndex = static_cast<int>(Hash(s + 4u) % 10u);
    obj.parallaxFactor = 0.005f;
    obj.tint =
        Color{static_cast<unsigned char>(100 + HashFloat01(s + 5u) * 155),
              static_cast<unsigned char>(100 + HashFloat01(s + 6u) * 155),
              static_cast<unsigned char>(100 + HashFloat01(s + 7u) * 155), 255};
    obj.orbitalRadius = 0.0f;
  }

  // Planets
  const int planetCount = 5 + static_cast<int>(HashFloat01(rng) * 5.0f);
  rng = Hash(rng);
  for (int i = 0; i < planetCount && g_count < kMaxSpaceObjects; ++i) {
    auto &obj = g_objects[g_count++];
    obj.type = SpaceObjectType::Planet;

    const uint32_t s = static_cast<uint32_t>(i) * 6271u + seed + 1000u;
    const float dist = 100.0f + HashFloat01(s) * 80.0f;
    const float ang = HashFloat01(s + 1u) * 2.0f * kPi;
    const float h = 10.0f + HashFloat01(s + 2u) * 40.0f;

    obj.basePos = {std::sin(ang) * dist, h, std::cos(ang) * dist};
    obj.currentPos = obj.basePos;
    obj.brightness = 0.7f + HashFloat01(s + 3u) * 0.3f;
    obj.size = 8.0f + HashFloat01(s + 4u) * 6.0f;
    obj.textureIndex = static_cast<int>(Hash(s + 5u) % 10u);

    obj.orbitalRadius = 15.0f + HashFloat01(s + 6u) * 40.0f;
    obj.orbitalSpeed = 0.05f + HashFloat01(s + 7u) * 0.15f;
    obj.orbitalAngle = HashFloat01(s + 8u) * 2.0f * kPi;

    const float ax = (HashFloat01(s + 9u) - 0.5f) * 0.3f;
    const float az = (HashFloat01(s + 10u) - 0.5f) * 0.3f;
    const float aLen = std::sqrt(ax * ax + 1.0f + az * az);
    const Vector3 axis = {ax / aLen, 1.0f / aLen, az / aLen};
    ComputeOrbitalBasis(obj, axis);

    obj.velocity = {0.0f, 0.0f, 0.0f};
    obj.parallaxFactor = 0.015f;
    obj.tint = WHITE;
  }

  // Asteroids
  const int asteroidCount = 12 + static_cast<int>(HashFloat01(rng) * 15.0f);
  rng = Hash(rng);
  for (int i = 0; i < asteroidCount && g_count < kMaxSpaceObjects; ++i) {
    auto &obj = g_objects[g_count++];
    obj.type = SpaceObjectType::Asteroid;

    const uint32_t s = static_cast<uint32_t>(i) * 4513u + seed + 2000u;
    obj.basePos = {(HashFloat01(s) - 0.5f) * 2.0f * cfg::kStarFieldRadius,
                   HashFloat01(s + 1u) * cfg::kStarFieldHeight * 0.8f + 5.0f,
                   (HashFloat01(s + 2u) - 0.2f) * cfg::kStarFieldDepth};
    obj.currentPos = obj.basePos;
    obj.brightness = 0.4f + HashFloat01(s + 3u) * 0.4f;
    obj.size = 0.4f + HashFloat01(s + 4u) * 1.0f;

    obj.velocity = {(HashFloat01(s + 5u) - 0.5f) * 2.5f,
                    (HashFloat01(s + 6u) - 0.5f) * 2.0f,
                    (HashFloat01(s + 7u) - 0.5f) * 1.5f};

    if (HashFloat01(s + 8u) > 0.4f) {
      obj.orbitalRadius = 4.0f + HashFloat01(s + 9u) * 8.0f;
      obj.orbitalSpeed = 0.4f + HashFloat01(s + 10u) * 1.2f;
      obj.orbitalAngle = HashFloat01(s + 11u) * 2.0f * kPi;

      const float ax = HashFloat01(s + 12u) - 0.5f;
      const float ay = HashFloat01(s + 13u) - 0.5f;
      const float az = HashFloat01(s + 14u) - 0.5f;
      const float aLen = std::sqrt(ax * ax + ay * ay + az * az);
      const Vector3 axis = (aLen > 0.001f)
                               ? Vector3{ax / aLen, ay / aLen, az / aLen}
                               : Vector3{0.0f, 1.0f, 0.0f};
      ComputeOrbitalBasis(obj, axis);
    } else {
      obj.orbitalRadius = 0.0f;
      obj.orbitalSpeed = 0.0f;
      obj.orbitalAngle = 0.0f;
      obj.orbitalRight = {1.0f, 0.0f, 0.0f};
      obj.orbitalForward = {0.0f, 0.0f, 1.0f};
    }

    obj.parallaxFactor = 0.1f + HashFloat01(s + 15u) * 0.15f;
    obj.textureIndex = 0;
    obj.tint = Color{
        static_cast<unsigned char>(160 + HashFloat01(s + 16u) * 95),
        static_cast<unsigned char>(150 + HashFloat01(s + 17u) * 105),
        static_cast<unsigned char>(130 + HashFloat01(s + 18u) * 125), 255};
  }

  g_inited = true;
}

void UpdateSpaceObjects(float dt, const Vector3 &playerPos) {
  for (int i = 0; i < g_count; ++i) {
    auto &obj = g_objects[i];

    // Orbital motion — uses pre-cached basis vectors (no cross product needed)
    if (obj.orbitalRadius > 0.0f) {
      obj.orbitalAngle += obj.orbitalSpeed * dt;
      const float cosA = std::cos(obj.orbitalAngle);
      const float sinA = std::sin(obj.orbitalAngle);
      const Vector3 offset = {
          (obj.orbitalRight.x * cosA + obj.orbitalForward.x * sinA) *
              obj.orbitalRadius,
          (obj.orbitalRight.y * cosA + obj.orbitalForward.y * sinA) *
              obj.orbitalRadius,
          (obj.orbitalRight.z * cosA + obj.orbitalForward.z * sinA) *
              obj.orbitalRadius};
      obj.currentPos = {obj.basePos.x + offset.x, obj.basePos.y + offset.y,
                        obj.basePos.z + offset.z};
    } else {
      obj.currentPos = obj.basePos;
    }

    // Linear drift
    obj.currentPos.x += obj.velocity.x * dt;
    obj.currentPos.y += obj.velocity.y * dt;
    obj.currentPos.z += obj.velocity.z * dt;

    // Parallax
    obj.currentPos.x += playerPos.x * obj.parallaxFactor;
    obj.currentPos.z += playerPos.z * obj.parallaxFactor * 2.0f;
  }
}

void RenderSpaceObjects(const Camera3D &camera, const LevelPalette &pal,
                        float simTime) {
  for (int i = 0; i < g_count; ++i) {
    const auto &obj = g_objects[i];

    if (obj.type == SpaceObjectType::Star) {
      const float twinkle =
          0.5f + 0.5f * std::sin(simTime * obj.pulseRate + obj.pulseOffset);
      const Color col = (obj.brightness > 0.7f)
                            ? Fade(pal.starBright, twinkle * obj.brightness)
                            : Fade(pal.starDim, twinkle * obj.brightness);
      DrawCubeV(obj.currentPos, Vector3{obj.size, obj.size, obj.size}, col);
    } else if (obj.type == SpaceObjectType::Comet) {
      const Color headCol = Fade(obj.tint, obj.brightness);
      DrawCubeV(obj.currentPos, Vector3{obj.size, obj.size, obj.size}, headCol);

      // Draw tail as series of fading particles
      Vector3 dir = {-obj.velocity.x, -obj.velocity.y, -obj.velocity.z};
      float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
      if (len > 0.001f) {
        dir = {dir.x / len, dir.y / len, dir.z / len};
        for (int j = 1; j <= 8; ++j) {
          float t = static_cast<float>(j) / 8.0f;
          Vector3 tailPos = {obj.currentPos.x + dir.x * t * 3.0f,
                             obj.currentPos.y + dir.y * t * 3.0f,
                             obj.currentPos.z + dir.z * t * 3.0f};
          float tailSize = obj.size * (1.0f - t * 0.7f);
          DrawCubeV(tailPos, {tailSize, tailSize, tailSize},
                    Fade(obj.tint, obj.brightness * (1.0f - t) * 0.4f));
        }
      }
    } else if (obj.type == SpaceObjectType::Nebula && g_texturesLoaded) {
      DrawBillboardRec(
          camera, g_planetTextures[obj.textureIndex],
          Rectangle{
              0.0f, 0.0f,
              static_cast<float>(g_planetTextures[obj.textureIndex].width),
              static_cast<float>(g_planetTextures[obj.textureIndex].height)},
          obj.currentPos, Vector2{obj.size, obj.size},
          Fade(obj.tint, obj.brightness * 0.3f));
    } else if (obj.type == SpaceObjectType::Planet && g_texturesLoaded) {
      DrawBillboardRec(
          camera, g_planetTextures[obj.textureIndex],
          Rectangle{
              0.0f, 0.0f,
              static_cast<float>(g_planetTextures[obj.textureIndex].width),
              static_cast<float>(g_planetTextures[obj.textureIndex].height)},
          obj.currentPos, Vector2{obj.size, obj.size}, obj.tint);
    } else if (obj.type == SpaceObjectType::Asteroid) {
      const Color col = Fade(obj.tint, obj.brightness);
      DrawCubeV(obj.currentPos, Vector3{obj.size, obj.size, obj.size}, col);
      DrawCubeWiresV(obj.currentPos, Vector3{obj.size, obj.size, obj.size},
                     Fade(col, 0.6f));
    }
  }
}

} // namespace render
