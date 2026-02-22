#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

#include <raylib.h>

#include "game/Game.hpp"

// ─── Math constants
// ───────────────────────────────────────────────────────────
namespace render {

constexpr float kPi = 3.14159265f;

// ─── Float utilities
// ──────────────────────────────────────────────────────────

inline float Clamp01(float v) {
  if (v < 0.0f)
    return 0.0f;
  if (v > 1.0f)
    return 1.0f;
  return v;
}

inline Vector3 LerpVec3(const Vector3 &a, const Vector3 &b, float t) {
  const float k = Clamp01(t);
  return Vector3{a.x + (b.x - a.x) * k, a.y + (b.y - a.y) * k,
                 a.z + (b.z - a.z) * k};
}

inline Vector3 InterpolatePosition(const Game &game, float alpha) {
  return LerpVec3(game.previousPlayer.position, game.player.position, alpha);
}

// ─── Deterministic hashing
// ────────────────────────────────────────────────────

// Integer hash (Wang hash variant).
inline uint32_t Hash(uint32_t x) {
  x ^= x >> 16;
  x *= 0x45d9f3bu;
  x ^= x >> 16;
  x *= 0x45d9f3bu;
  x ^= x >> 16;
  return x;
}

// Seeded hash — useful for independently-keyed domains (avoids duplicate logic
// that previously existed as ``HashStage`` with a different seed multiplier).
inline uint32_t HashSeeded(uint32_t x, uint32_t salt) { return Hash(x * salt); }

// Map a hash to [0, 1).
inline float HashFloat01(uint32_t seed) {
  return static_cast<float>(Hash(seed) & 0xFFFFu) / 65535.0f;
}

// Map a seeded hash to [0, 1).
inline float HashFloat01Seeded(uint32_t seed, uint32_t salt) {
  return static_cast<float>(HashSeeded(seed, salt) & 0xFFFFu) / 65535.0f;
}

// ─── Stage / palette background colours ──────────────────────────────────────

// Salt that distinguishes stage hashes from normal hashes (replaces old
// ``HashStage`` which was an ad-hoc copy of Hash with seed*7919).
constexpr uint32_t kStageSalt = 7919u;

inline Color GetStageBackgroundTop(int stage) {
  const uint32_t h = Hash(static_cast<uint32_t>(stage) * kStageSalt);
  const float r = 20.0f + HashFloat01(h) * 60.0f;
  const float g = 20.0f + HashFloat01(h + 1u) * 60.0f;
  const float b = 30.0f + HashFloat01(h + 2u) * 80.0f;
  return Color{static_cast<unsigned char>(r), static_cast<unsigned char>(g),
               static_cast<unsigned char>(b), 255};
}

inline Color GetStageBackgroundBottom(int stage) {
  const uint32_t h = Hash((static_cast<uint32_t>(stage) + 1000u) * kStageSalt);
  const float r = 5.0f + HashFloat01(h) * 20.0f;
  const float g = 5.0f + HashFloat01(h + 1u) * 20.0f;
  const float b = 8.0f + HashFloat01(h + 2u) * 30.0f;
  return Color{static_cast<unsigned char>(r), static_cast<unsigned char>(g),
               static_cast<unsigned char>(b), 255};
}

// ─── Colour tinting
// ───────────────────────────────────────────────────────────

inline Color ApplyColorTint(const Color &base, int tintIndex) {
  if (tintIndex == 0)
    return base;
  Color result = base;
  if (tintIndex == 1) {
    result.r = static_cast<unsigned char>(std::min(255, base.r + 15));
    result.g = static_cast<unsigned char>(std::min(255, base.g + 15));
    result.b = static_cast<unsigned char>(std::min(255, base.b + 15));
  } else if (tintIndex == 2) {
    result.r = static_cast<unsigned char>(std::max(0, base.r - 10));
    result.g = static_cast<unsigned char>(std::min(255, base.g + 10));
    result.b = static_cast<unsigned char>(std::min(255, base.b + 20));
  }
  return result;
}

} // namespace render
