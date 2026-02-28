#include "sim/Sim.hpp"

#include <cmath>
#include <algorithm>

#include "core/Config.hpp"
#include "core/Rng.hpp"
#include "game/Game.hpp"
#include "sim/Level.hpp"
#include "sim/PowerUp.hpp"

namespace {
float ClampMinZero(const float value) {
  if (value < 0.0f) {
    return 0.0f;
  }
  return value;
}

float MoveToward(const float current, const float target,
                 const float maxDelta) {
  if (current < target) {
    const float next = current + maxDelta;
    return (next > target) ? target : next;
  }
  const float next = current - maxDelta;
  return (next < target) ? target : next;
}

float Clamp(const float value, const float minValue, const float maxValue) {
  if (value < minValue) {
    return minValue;
  }
  if (value > maxValue) {
    return maxValue;
  }
  return value;
}

float Clamp01(const float value) { return Clamp(value, 0.0f, 1.0f); }

void UpdateLandingParticles(Game &game, const float dt) {
  for (auto &p : game.landingParticles) {
    if (!p.active) {
      continue;
    }
    p.life = ClampMinZero(p.life - dt);
    if (p.life <= 0.0f) {
      p.active = false;
      continue;
    }

    const float dragT = Clamp01(cfg::kLandingParticleDrag * dt);
    p.velocity.x *= (1.0f - dragT);
    p.velocity.z *= (1.0f - dragT);
    p.velocity.y += cfg::kGravity * 0.35f * dt;
    p.position.x += p.velocity.x * dt;
    p.position.y += p.velocity.y * dt;
    p.position.z += p.velocity.z * dt;
  }
}

void SpawnLandingBurst(Game &game, const Vector3 &origin) {
  int spawned = 0;
  for (auto &p : game.landingParticles) {
    if (p.active) {
      continue;
    }
    const float angle = core::NextFloat01(game.rngState) * 2.0f * PI;
    const float speed =
        cfg::kLandingParticleSpeedMin +
        (cfg::kLandingParticleSpeedMax - cfg::kLandingParticleSpeedMin) *
            core::NextFloat01(game.rngState);
    p.active = true;
    p.position = origin;
    p.velocity = Vector3{std::cos(angle) * speed,
                         cfg::kLandingParticleRiseSpeed *
                             (0.7f + 0.6f * core::NextFloat01(game.rngState)),
                         std::sin(angle) * speed};
    p.life = cfg::kLandingParticleLife *
             (0.75f + 0.5f * core::NextFloat01(game.rngState));
    ++spawned;
    if (spawned >= cfg::kLandingBurstCount) {
      break;
    }
  }
}

bool CheckPowerUpCollision(const Vector3 &playerPos, const PowerUp &pu) {
  const float puRadius = 0.5f;  // Collision radius for power-up
  const float playerHalfW = cfg::kPlayerWidth * 0.5f;
  const float playerHalfH = cfg::kPlayerHalfHeight;
  const float playerHalfD = cfg::kPlayerDepth * 0.5f;
  
  const float dx = std::abs(playerPos.x - pu.x);
  const float dy = std::abs(playerPos.y - pu.y);
  const float dz = std::abs(playerPos.z - pu.z);
  
  return dx < (playerHalfW + puRadius) &&
         dy < (playerHalfH + puRadius) &&
         dz < (playerHalfD + puRadius);
}

void ActivatePowerUp(Game &game, PowerUpType type) {
  // Find an empty slot or reuse expired effect
  int slot = -1;
  for (int i = 0; i < game.activeEffectCount; ++i) {
    if (game.activeEffects[i].type == PowerUpType::None || 
        game.activeEffects[i].timer <= 0.0f) {
      slot = i;
      break;
    }
  }
  
  if (slot < 0 && game.activeEffectCount < 8) {
    slot = game.activeEffectCount++;
  }
  
  if (slot < 0) {
    return;  // No available slots
  }
  
  auto &effect = game.activeEffects[slot];
  effect.type = type;
  effect.isPowerUp = !IsDebuff(type);
  effect.consumed = false;
  
  // Set duration based on type
  switch (type) {
    case PowerUpType::Shield:
      effect.timer = cfg::kShieldDuration;  // Until consumed
      game.hasShield = true;
      break;
    case PowerUpType::ScoreMultiplier:
      effect.timer = cfg::kScoreMultiplierDuration;
      game.scoreMultiplierBoost = cfg::kScoreMultiplierBoost;
      break;
    case PowerUpType::SpeedBoostShield:
      effect.timer = cfg::kSpeedBoostDuration;
      game.speedBoostAmount = cfg::kSpeedBoostAmount;
      game.hasShield = true;
      break;
    case PowerUpType::SpeedBoostGhost:
      effect.timer = cfg::kSpeedBoostDuration;
      game.speedBoostAmount = cfg::kSpeedBoostAmount;
      game.ghostMode = true;
      break;
    case PowerUpType::ObstacleReveal:
      effect.timer = cfg::kObstacleRevealDuration;
      game.obstacleRevealActive = true;
      break;
    case PowerUpType::SpeedDrain:
      effect.timer = cfg::kSpeedDrainDuration;
      game.speedDrainAmount = cfg::kSpeedDrainAmount;
      break;
    case PowerUpType::ObstacleSurge:
      effect.timer = 0.0f;  // Instant effect, applied to next chunk
      game.obstacleSurgePending = true;
      break;
    default:
      break;
  }
}

void UpdateActiveEffects(Game &game, const float dt) {
  // Reset effect flags
  game.hasShield = false;
  game.ghostMode = false;
  game.speedBoostAmount = 0.0f;
  game.speedDrainAmount = 0.0f;
  game.scoreMultiplierBoost = 1.0f;
  game.obstacleRevealActive = false;
  
  // Update all active effects
  for (int i = 0; i < game.activeEffectCount; ++i) {
    auto &effect = game.activeEffects[i];
    
    if (effect.type == PowerUpType::None || effect.consumed) {
      continue;
    }
    
    // Decrement timer (shield has 0 duration, so skip)
    if (effect.timer > 0.0f) {
      effect.timer = ClampMinZero(effect.timer - dt);
      
      // Remove expired effects
      if (effect.timer <= 0.0f && effect.type != PowerUpType::Shield) {
        effect.type = PowerUpType::None;
        continue;
      }
    }
    
    // Apply effects (cumulative)
    switch (effect.type) {
      case PowerUpType::Shield:
        if (!effect.consumed) {
          game.hasShield = true;
        }
        break;
      case PowerUpType::ScoreMultiplier:
        game.scoreMultiplierBoost = cfg::kScoreMultiplierBoost;
        break;
      case PowerUpType::SpeedBoostShield:
        game.speedBoostAmount = cfg::kSpeedBoostAmount;
        if (!effect.consumed) {
          game.hasShield = true;
        }
        break;
      case PowerUpType::SpeedBoostGhost:
        game.speedBoostAmount = cfg::kSpeedBoostAmount;
        game.ghostMode = true;
        break;
      case PowerUpType::ObstacleReveal:
        game.obstacleRevealActive = true;
        break;
      case PowerUpType::SpeedDrain:
        game.speedDrainAmount = cfg::kSpeedDrainAmount;
        break;
      default:
        break;
    }
  }
  
  // Clean up expired effects
  int writeIdx = 0;
  for (int i = 0; i < game.activeEffectCount; ++i) {
    if (game.activeEffects[i].type != PowerUpType::None && 
        (game.activeEffects[i].timer > 0.0f || game.activeEffects[i].type == PowerUpType::Shield)) {
      if (writeIdx != i) {
        game.activeEffects[writeIdx] = game.activeEffects[i];
      }
      writeIdx++;
    }
  }
  game.activeEffectCount = writeIdx;
}
} // namespace

void SimStep(Game &game, const float dt) {
  UpdateLandingParticles(game, dt);

  if (!game.runActive) {
    game.input.jumpQueued = false;
    game.input.dashQueued = false;
    return;
  }

  auto &player = game.player;
  const bool wasGrounded = player.grounded;

  player.jumpBufferTimer = ClampMinZero(player.jumpBufferTimer - dt);
  player.coyoteTimer = ClampMinZero(player.coyoteTimer - dt);
  player.dashTimer = ClampMinZero(player.dashTimer - dt);
  player.dashCooldownTimer = ClampMinZero(player.dashCooldownTimer - dt);

  if (game.input.jumpQueued) {
    player.jumpBufferTimer = cfg::kJumpBufferTime;
    game.input.jumpQueued = false;
  }
  if (game.input.dashQueued) {
    const bool canDash = player.grounded &&
                         (player.dashCooldownTimer <= 0.0f) &&
                         (player.dashTimer <= 0.0f);
    if (canDash) {
      player.dashTimer = cfg::kDashDuration;
      player.dashCooldownTimer = cfg::kDashCooldown;
    }
    game.input.dashQueued = false;
  }

  const bool canJump = player.grounded || (player.coyoteTimer > 0.0f);
  bool jumpedThisStep = false;
  if (canJump && player.jumpBufferTimer > 0.0f) {
    player.velocity.y = cfg::kJumpForce;
    player.grounded = false;
    player.jumpBufferTimer = 0.0f;
    player.coyoteTimer = 0.0f;
    jumpedThisStep = true;
  }

  const float strafeScale = player.grounded ? 1.0f : cfg::kAirControlFactor;
  const float desiredStrafeVelocity =
      game.input.moveX * cfg::kStrafeSpeed * strafeScale;
  player.velocity.x = MoveToward(player.velocity.x, desiredStrafeVelocity,
                                 cfg::kStrafeAccel * strafeScale * dt);

  // Update throttle based on input
  if (game.input.throttleDelta != 0.0f) {
    const float throttleChange =
        game.input.throttleDelta * cfg::kThrottleChangeRate * dt;
    game.throttle = Clamp(game.throttle + throttleChange, cfg::kThrottleMin,
                          cfg::kThrottleMax);
  }

  // Dynamic difficulty: ramp with run time, capped.
  game.difficultyT = Clamp(game.runTime * cfg::kDifficultyRampRate, 0.0f,
                           cfg::kDifficultyMaxCap);
  game.diffSpeedBonus = game.difficultyT * cfg::kDiffSpeedBonus;
  game.hazardProbability =
      cfg::kDiffHazardProbMin +
      (cfg::kDiffHazardProbMax - cfg::kDiffHazardProbMin) * game.difficultyT;

  // Update active effects
  UpdateActiveEffects(game, dt);
  
  // Calculate speed based on throttle (interpolate between min and max)
  const float throttleSpeed =
      cfg::kThrottleSpeedMin +
      (cfg::kThrottleSpeedMax - cfg::kThrottleSpeedMin) * game.throttle;
  const float baseSpeed = throttleSpeed + game.diffSpeedBonus;
  player.velocity.z =
      baseSpeed + ((player.dashTimer > 0.0f) ? cfg::kDashSpeedBoost : 0.0f) +
      game.speedBoostAmount - game.speedDrainAmount;
  if (!player.grounded || jumpedThisStep) {
    player.velocity.y += cfg::kGravity * dt;
  } else {
    player.velocity.y = 0.0f;
  }

  player.position.x += player.velocity.x * dt;
  player.position.y += player.velocity.y * dt;
  player.position.z += player.velocity.z * dt;

  game.runTime += dt;

  if (player.position.y < cfg::kFailKillY) {
    game.runActive = false;
    game.runOver = true;
    game.deathCause = 1;
    if (GetCurrentScore(game) > game.bestScore) {
      game.bestScore = GetCurrentScore(game);
    }
    return;
  }

  // Extend endless level if needed
  if (game.isEndlessMode) {
    // Pass obstacle surge flag to generator
    if (game.obstacleSurgePending) {
      game.endlessGenerator.obstacleSurgePending = true;
      game.obstacleSurgePending = false;  // Consume flag
    }
    game.endlessGenerator.ExtendLevel(player.position.z, game.difficultyT);
    // Assign visual variants to newly generated segments
    AssignVariants(game.endlessGenerator.GetLevelMutable());
    game.level = &game.endlessGenerator.GetLevel();
    
    // Update power-up animations (use mutable reference)
    Level &mutableLevel = game.endlessGenerator.GetLevelMutable();
    for (int i = 0; i < mutableLevel.powerUpCount; ++i) {
      auto &pu = mutableLevel.powerUps[i];
      if (pu.active) {
        // Update rotation (will be used in rendering)
        pu.rotation += 45.0f * dt;  // 45 degrees per second
        if (pu.rotation >= 360.0f) {
          pu.rotation -= 360.0f;
        }
      }
    }
  }

  // --- Level-based collision ---
  const Level *lv = game.level;
  if (lv) {
    // Check power-up collisions (need mutable access for endless mode)
    if (game.isEndlessMode) {
      Level &mutableLevel = game.endlessGenerator.GetLevelMutable();
      for (int i = 0; i < mutableLevel.powerUpCount; ++i) {
        auto &pu = mutableLevel.powerUps[i];
        if (!pu.active) continue;
        
        if (CheckPowerUpCollision(player.position, pu)) {
          ActivatePowerUp(game, pu.type);
          pu.active = false;  // Consume power-up
          // Spawn pickup particles (reuse landing particles)
          SpawnLandingBurst(game, Vector3{pu.x, pu.y, pu.z});
        }
      }
    } else {
      // For regular levels, power-ups are read-only (not implemented for regular levels)
      // This branch is for future expansion
    }
    
    // Check obstacle collision (kill) - skip if ghost mode
    if (!game.ghostMode && CheckObstacleCollision(*lv, player.position, cfg::kPlayerWidth * 0.45f,
                               cfg::kPlayerHalfHeight * 0.9f,
                               cfg::kPlayerDepth * 0.45f)) {
      // Check for shield
      if (game.hasShield) {
        // Consume shield instead of dying
        for (int i = 0; i < game.activeEffectCount; ++i) {
          auto &effect = game.activeEffects[i];
          if ((effect.type == PowerUpType::Shield || effect.type == PowerUpType::SpeedBoostShield) &&
              !effect.consumed) {
            effect.consumed = true;
            game.hasShield = false;
            break;
          }
        }
      } else {
        // No shield, die
        game.runActive = false;
        game.runOver = true;
        game.deathCause = 2;
        if (GetCurrentScore(game) > game.bestScore) {
          game.bestScore = GetCurrentScore(game);
        }
        return;
      }
    }

    // Check level completion (crossing finish zone) - skip for Endless Mode
    if (!game.isEndlessMode && CheckFinishZoneCrossing(*lv, player.position.z)) {
      game.levelComplete = true;
      game.runActive = false;
      game.runOver = true;
      game.deathCause = 3;
      if (GetCurrentScore(game) > game.bestScore) {
        game.bestScore = GetCurrentScore(game);
      }
      return;
    }

    // Find ground segment under player.
    const int segIdx = FindSegmentUnder(
        *lv, player.position.z, player.position.x, cfg::kPlayerWidth * 0.5f);
    if (segIdx >= 0) {
      const auto &seg = lv->segments[segIdx];
      // Clamp X within segment bounds.
      const float segLeft =
          seg.xOffset - seg.width * 0.5f + cfg::kPlayerWidth * 0.5f;
      const float segRight =
          seg.xOffset + seg.width * 0.5f - cfg::kPlayerWidth * 0.5f;
      if (player.position.x < segLeft)
        player.position.x = segLeft;
      if (player.position.x > segRight)
        player.position.x = segRight;

      const float groundY = seg.topY + cfg::kPlayerHalfHeight;
      if (player.position.y <= groundY) {
        player.position.y = groundY;
        player.velocity.y = 0.0f;
        player.grounded = true;
        player.coyoteTimer = cfg::kCoyoteTime;
        if (!wasGrounded) {
          SpawnLandingBurst(game, Vector3{player.position.x, seg.topY + 0.02f,
                                          player.position.z});
        }
      } else {
        player.grounded = false;
        if (wasGrounded && !jumpedThisStep) {
          player.coyoteTimer = cfg::kCoyoteTime;
        }
      }
    } else {
      // Over a gap — no ground.
      player.grounded = false;
      if (wasGrounded && !jumpedThisStep) {
        player.coyoteTimer = cfg::kCoyoteTime;
      }
    }
  }

  const float speedBandT =
      Clamp((player.velocity.z - cfg::kForwardSpeed) / cfg::kDashSpeedBoost,
            0.0f, 1.0f);
  const float baseMultiplier =
      cfg::kScoreMultiplierMin +
      (cfg::kScoreMultiplierMax - cfg::kScoreMultiplierMin) * speedBandT;
  game.scoreMultiplier = baseMultiplier * game.scoreMultiplierBoost;

  const float distanceStep =
      player.velocity.z * dt * cfg::kScoreDistancePerUnit;
  game.distanceScore += distanceStep * game.scoreMultiplier;

  if (player.dashTimer > 0.0f) {
    game.styleScore +=
        cfg::kScoreDashStylePerSecond * dt * game.scoreMultiplier;
  }
}
