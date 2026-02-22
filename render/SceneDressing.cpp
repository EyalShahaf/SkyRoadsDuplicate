#include "render/SceneDressing.hpp"

#include <cmath>

#include "core/Config.hpp"
#include "render/Palette.hpp"
#include "render/RenderUtils.hpp"

namespace render {

// ─── DecoCubes
// ────────────────────────────────────────────────────────────────

struct DecoCube {
  Vector3 pos;
  float size;
  int colorIndex; // 0–2
  float rotSpeed;
};

static bool g_decoInited = false;
static DecoCube g_decoCubes[cfg::kDecoCubeCount];

static Color GetDecoCubeColor(const LevelPalette &p, int idx) {
  if (idx == 0)
    return p.decoCube1;
  if (idx == 1)
    return p.decoCube2;
  return p.decoCube3;
}

// ─── AmbientDots
// ──────────────────────────────────────────────────────────────

struct AmbientDot {
  Vector3 basePos;
  float phase;
  float speed;
};

static bool g_ambientInited = false;
static AmbientDot g_ambientDots[cfg::kAmbientParticleCount];

// ─── Mountains
// ────────────────────────────────────────────────────────────────

struct Mountain {
  float angle;
  float height;
  float width;
};

static bool g_mountainsInited = false;
static Mountain g_mountains[cfg::kMountainCount];

// ─── Init
// ─────────────────────────────────────────────────────────────────────

void InitSceneDressing() {
  if (!g_decoInited) {
    for (int i = 0; i < cfg::kDecoCubeCount; ++i) {
      const uint32_t s = static_cast<uint32_t>(i) * 6271u + 42u;
      const float side = (HashFloat01(s) > 0.5f) ? 1.0f : -1.0f;
      g_decoCubes[i].pos = Vector3{
          side * (cfg::kDecoCubeSideOffset + HashFloat01(s + 1u) * 8.0f),
          0.5f + HashFloat01(s + 2u) * 4.0f,
          cfg::kPlatformStartZ + HashFloat01(s + 3u) * cfg::kPlatformLength};
      g_decoCubes[i].size =
          cfg::kDecoCubeMinSize +
          HashFloat01(s + 4u) * (cfg::kDecoCubeMaxSize - cfg::kDecoCubeMinSize);
      g_decoCubes[i].colorIndex = static_cast<int>(Hash(s + 5u) % 3u);
      g_decoCubes[i].rotSpeed = 20.0f + HashFloat01(s + 6u) * 40.0f;
    }
    g_decoInited = true;
  }

  if (!g_ambientInited) {
    for (int i = 0; i < cfg::kAmbientParticleCount; ++i) {
      const uint32_t s = static_cast<uint32_t>(i) * 4513u + 77u;
      g_ambientDots[i].basePos =
          Vector3{(HashFloat01(s) - 0.5f) * 2.0f * cfg::kAmbientParticleRadius,
                  0.5f + HashFloat01(s + 1u) * cfg::kAmbientParticleHeight,
                  (HashFloat01(s + 2u) - 0.3f) * 60.0f};
      g_ambientDots[i].phase = HashFloat01(s + 3u) * 2.0f * kPi;
      g_ambientDots[i].speed = 0.5f + HashFloat01(s + 4u) * 1.5f;
    }
    g_ambientInited = true;
  }

  if (!g_mountainsInited) {
    for (int i = 0; i < cfg::kMountainCount; ++i) {
      const uint32_t s = static_cast<uint32_t>(i) * 3137u + 13u;
      g_mountains[i].angle = static_cast<float>(i) /
                                 static_cast<float>(cfg::kMountainCount) *
                                 360.0f +
                             HashFloat01(s) * 30.0f;
      g_mountains[i].height =
          4.0f + HashFloat01(s + 1u) * cfg::kMountainMaxHeight;
      g_mountains[i].width = 15.0f + HashFloat01(s + 2u) * 25.0f;
    }
    g_mountainsInited = true;
  }
}

// ─── Render
// ───────────────────────────────────────────────────────────────────

void RenderMountains(const LevelPalette &pal, const Vector3 &playerRenderPos) {
  for (int i = 0; i < cfg::kMountainCount; ++i) {
    const Mountain &m = g_mountains[i];
    const float rad = m.angle * DEG2RAD;
    const Vector3 base = {
        std::sin(rad) * cfg::kMountainDistance + playerRenderPos.x * 0.02f,
        -2.0f,
        std::cos(rad) * cfg::kMountainDistance + playerRenderPos.z * 0.05f};
    DrawCubeV(base, Vector3{m.width, m.height, m.width * 0.5f},
              pal.mountainSilhouette);
  }
}

void RenderDecoCubes(const LevelPalette &pal, const Vector3 &playerRenderPos,
                     float simTime) {
  for (int i = 0; i < cfg::kDecoCubeCount; ++i) {
    const DecoCube &dc = g_decoCubes[i];
    if (std::fabs(dc.pos.z - playerRenderPos.z) > 60.0f)
      continue;

    const float bob = std::sin(simTime * dc.rotSpeed * 0.03f + dc.pos.x) * 0.4f;
    const Vector3 pos = {dc.pos.x, dc.pos.y + bob, dc.pos.z};
    const Color col = GetDecoCubeColor(pal, dc.colorIndex);

    // Glow halo on ground beneath cube.
    DrawCubeV(Vector3{dc.pos.x, cfg::kPlatformTopY + 0.03f, dc.pos.z},
              Vector3{dc.size * 1.6f, 0.01f, dc.size * 1.6f}, Fade(col, 0.15f));
    DrawCubeWiresV(pos, Vector3{dc.size, dc.size, dc.size}, col);
    DrawCubeV(pos, Vector3{dc.size * 0.7f, dc.size * 0.7f, dc.size * 0.7f},
              Fade(col, 0.25f));
  }
}

void RenderAmbientDots(const LevelPalette &pal, const Vector3 &playerRenderPos,
                       float simTime) {
  for (int i = 0; i < cfg::kAmbientParticleCount; ++i) {
    const AmbientDot &d = g_ambientDots[i];

    const float drift = std::sin(simTime * d.speed + d.phase) * 2.0f;
    const Vector3 pos = {d.basePos.x + playerRenderPos.x * 0.08f + drift,
                         d.basePos.y +
                             std::sin(simTime * 0.8f + d.phase) * 1.0f,
                         d.basePos.z + playerRenderPos.z * 0.15f};

    if (std::fabs(pos.z - playerRenderPos.z) > 50.0f)
      continue;

    DrawCubeV(
        pos, Vector3{0.05f, 0.05f, 0.05f},
        Fade(pal.ambientParticle, 0.4f + 0.3f * std::sin(simTime + d.phase)));
  }
}

} // namespace render
