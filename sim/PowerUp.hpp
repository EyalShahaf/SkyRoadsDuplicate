#pragma once

#include <cstdint>

// Power-up and debuff types
enum class PowerUpType : int {
  None = -1,
  Shield = 0,
  ScoreMultiplier = 1,
  SpeedBoostShield = 2,
  SpeedBoostGhost = 3,
  ObstacleReveal = 4,
  SpeedDrain = 5,      // Debuff
  ObstacleSurge = 6    // Debuff
};

// Power-up instance in the level
struct PowerUp {
  float z = 0.0f;
  float x = 0.0f;
  float y = 0.0f;      // Height above segment
  PowerUpType type = PowerUpType::None;
  bool active = true;
  float bobOffset = 0.0f;  // For floating animation
  float rotation = 0.0f;   // For rotation animation
};

// Active effect on the player
struct ActiveEffect {
  PowerUpType type = PowerUpType::None;
  float timer = 0.0f;
  bool isPowerUp = true;
  bool consumed = false;  // For one-time effects like shield
};

// Helper function to get text label for power-up type
const char* GetPowerUpLabel(PowerUpType type);

// Check if a power-up type is a debuff
inline bool IsDebuff(PowerUpType type) {
  return type == PowerUpType::SpeedDrain || type == PowerUpType::ObstacleSurge;
}
