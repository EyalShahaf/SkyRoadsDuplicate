#include "sim/PowerUp.hpp"

const char* GetPowerUpLabel(PowerUpType type) {
  switch (type) {
    case PowerUpType::Shield:
      return "SHIELD";
    case PowerUpType::ScoreMultiplier:
      return "BONUS";
    case PowerUpType::SpeedBoostShield:
      return "RUSH";
    case PowerUpType::SpeedBoostGhost:
      return "PHASE";
    case PowerUpType::ObstacleReveal:
      return "REVEAL";
    case PowerUpType::SpeedDrain:
      return "SLOW";
    case PowerUpType::ObstacleSurge:
      return "SURGE";
    default:
      return "";
  }
}
