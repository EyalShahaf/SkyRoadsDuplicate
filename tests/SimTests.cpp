#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>

#include "core/Config.hpp"
#include "core/Log.hpp"
#include "game/Game.hpp"
#include "sim/Level.hpp"
#include "sim/Sim.hpp"

namespace {
bool NearlyEqual(const float a, const float b, const float eps = 1e-5f) {
  return std::fabs(a - b) <= eps;
}

Game MakeBaseGame() {
  Game game{};
  game.player.position = Vector3{0.0f, cfg::kPlayerHalfHeight, 2.0f};
  game.player.velocity = Vector3{0.0f, 0.0f, cfg::kForwardSpeed};
  game.player.grounded = true;
  game.player.jumpBufferTimer = 0.0f;
  game.player.coyoteTimer = cfg::kCoyoteTime;
  game.level = &GetLevel1();
  return game;
}

bool TestJumpQueueReliability() {
  Game game = MakeBaseGame();
  game.input.jumpQueued = true;

  SimStep(game, cfg::kFixedDt);

  return !game.player.grounded && (game.player.velocity.y > 0.0f);
}

bool TestRepeatedJumpsAfterLanding() {
  Game game = MakeBaseGame();
  game.input.jumpQueued = true;
  SimStep(game, cfg::kFixedDt);

  bool landed = false;
  for (int i = 0; i < 600; ++i) {
    SimStep(game, cfg::kFixedDt);
    if (game.player.grounded) {
      landed = true;
      break;
    }
  }
  if (!landed) {
    return false;
  }

  game.input.jumpQueued = true;
  SimStep(game, cfg::kFixedDt);
  return !game.player.grounded && (game.player.velocity.y > 0.0f);
}

bool TestDeterministicSimScript() {
  Game a = MakeBaseGame();
  Game b = MakeBaseGame();

  for (int i = 0; i < 360; ++i) {
    const float moveX =
        ((i / 60) % 3 == 0) ? -1.0f : (((i / 60) % 3 == 1) ? 0.0f : 1.0f);
    a.input.moveX = moveX;
    b.input.moveX = moveX;

    const bool shouldJump = (i == 8) || (i == 120) || (i == 230);
    if (shouldJump) {
      a.input.jumpQueued = true;
      b.input.jumpQueued = true;
    }

    SimStep(a, cfg::kFixedDt);
    SimStep(b, cfg::kFixedDt);
  }

  return NearlyEqual(a.player.position.x, b.player.position.x) &&
         NearlyEqual(a.player.position.y, b.player.position.y) &&
         NearlyEqual(a.player.position.z, b.player.position.z) &&
         NearlyEqual(a.player.velocity.x, b.player.velocity.x) &&
         NearlyEqual(a.player.velocity.y, b.player.velocity.y) &&
         NearlyEqual(a.player.velocity.z, b.player.velocity.z) &&
         (a.player.grounded == b.player.grounded);
}

bool TestGroundClampOnPlatform() {
  Game game = MakeBaseGame();
  game.input.moveX = 0.0f;

  for (int i = 0; i < 120; ++i) {
    SimStep(game, cfg::kFixedDt);
  }

  return NearlyEqual(game.player.position.y,
                     cfg::kPlatformTopY + cfg::kPlayerHalfHeight) &&
         game.player.grounded;
}

bool TestDashImpulseGrounded() {
  Game game = MakeBaseGame();
  game.input.dashQueued = true;
  SimStep(game, cfg::kFixedDt);

  return (game.player.velocity.z > cfg::kForwardSpeed) &&
         (game.player.dashTimer > 0.0f) &&
         (game.player.dashCooldownTimer > 0.0f);
}

bool TestDashCooldownBlocksRetrigger() {
  Game game = MakeBaseGame();
  game.input.dashQueued = true;
  SimStep(game, cfg::kFixedDt);

  // Let dash end while cooldown is still active.
  for (int i = 0; i < 30; ++i) {
    SimStep(game, cfg::kFixedDt);
  }

  if (!(game.player.dashTimer <= 0.0f) ||
      !(game.player.dashCooldownTimer > 0.0f)) {
    return false;
  }

  game.input.dashQueued = true;
  SimStep(game, cfg::kFixedDt);
  // Base speed depends on throttle (default 0.5)
  float throttleSpeed =
      cfg::kThrottleSpeedMin +
      (cfg::kThrottleSpeedMax - cfg::kThrottleSpeedMin) * game.throttle;
  const float expectedBase = throttleSpeed + game.diffSpeedBonus;
  if (!NearlyEqual(game.player.velocity.z, expectedBase, 0.01f)) {
    return false;
  }

  while (game.player.dashCooldownTimer > 0.0f) {
    SimStep(game, cfg::kFixedDt);
  }

  const float baseBeforeDash = cfg::kForwardSpeed + game.diffSpeedBonus;
  game.input.dashQueued = true;
  SimStep(game, cfg::kFixedDt);
  return game.player.velocity.z > baseBeforeDash;
}

bool TestAirControlBounded() {
  Game game = MakeBaseGame();
  game.player.grounded = false;
  game.player.position.y = 3.0f;
  game.player.position.z = cfg::kPlatformStartZ + cfg::kPlatformLength + 5.0f;
  game.player.velocity = Vector3{0.0f, 0.0f, cfg::kForwardSpeed};
  game.input.moveX = 1.0f;

  for (int i = 0; i < 60; ++i) {
    SimStep(game, cfg::kFixedDt);
  }

  const float maxAirStrafe = cfg::kStrafeSpeed * cfg::kAirControlFactor;
  return std::fabs(game.player.velocity.x) <= (maxAirStrafe + 1e-4f);
}

bool TestFailStateTrigger() {
  Game game = MakeBaseGame();
  game.player.position.y = cfg::kFailKillY - 0.1f;
  SimStep(game, cfg::kFixedDt);
  return game.runOver && !game.runActive;
}

bool TestDeterministicScoreProgression() {
  Game a{};
  Game b{};
  InitGame(a, 12345u);
  InitGame(b, 12345u);

  for (int i = 0; i < 300; ++i) {
    const float moveX = (i % 120 < 60) ? -1.0f : 1.0f;
    a.input.moveX = moveX;
    b.input.moveX = moveX;

    if (i == 20 || i == 120 || i == 220) {
      a.input.jumpQueued = true;
      b.input.jumpQueued = true;
    }
    if (i == 60 || i == 180) {
      a.input.dashQueued = true;
      b.input.dashQueued = true;
    }

    SimStep(a, cfg::kFixedDt);
    SimStep(b, cfg::kFixedDt);
  }

  return NearlyEqual(GetCurrentScore(a), GetCurrentScore(b), 1e-3f) &&
         NearlyEqual(a.scoreMultiplier, b.scoreMultiplier, 1e-5f) &&
         NearlyEqual(a.distanceScore, b.distanceScore, 1e-3f) &&
         NearlyEqual(a.styleScore, b.styleScore, 1e-3f);
}

bool TestMultiplierBounds() {
  Game game = MakeBaseGame();
  for (int i = 0; i < 200; ++i) {
    if (i == 20 || i == 100) {
      game.input.dashQueued = true;
    }
    SimStep(game, cfg::kFixedDt);
    if (game.scoreMultiplier < cfg::kScoreMultiplierMin ||
        game.scoreMultiplier > cfg::kScoreMultiplierMax) {
      return false;
    }
  }
  return true;
}

bool TestRestartResetInvariants() {
  Game game{};
  InitGame(game, 0xDEADBEEFu);
  for (int i = 0; i < 120; ++i) {
    if (i == 30) {
      game.input.dashQueued = true;
    }
    SimStep(game, cfg::kFixedDt);
  }
  game.bestScore = 111.0f;
  const uint32_t seedBefore = game.runSeed;

  ResetRun(game, seedBefore);

  return game.runActive && !game.runOver && NearlyEqual(game.runTime, 0.0f) &&
         NearlyEqual(game.distanceScore, 0.0f) &&
         NearlyEqual(game.styleScore, 0.0f) &&
         NearlyEqual(game.scoreMultiplier, cfg::kScoreMultiplierMin) &&
         NearlyEqual(game.bestScore, 111.0f) &&
         NearlyEqual(game.player.velocity.z, cfg::kForwardSpeed) &&
         NearlyEqual(game.difficultyT, 0.0f) &&
         NearlyEqual(game.diffSpeedBonus, 0.0f) &&
         NearlyEqual(game.hazardProbability, cfg::kDiffHazardProbMin);
}
bool TestDifficultyRisesMonotonically() {
  Game game{};
  InitGame(game, 42u);
  float prevT = 0.0f;
  for (int i = 0; i < 6000; ++i) { // ~50 seconds of sim
    SimStep(game, cfg::kFixedDt);
    if (game.difficultyT < prevT - 1e-7f) {
      return false; // must never decrease
    }
    prevT = game.difficultyT;
  }
  // By 50s difficulty should have risen above zero.
  return game.difficultyT > 0.0f;
}

bool TestDifficultyCap() {
  Game game{};
  InitGame(game, 99u);
  // Fast-forward 200s of sim time by running many steps.
  constexpr int steps = 200 * 120; // 200 seconds at 120Hz
  for (int i = 0; i < steps; ++i) {
    SimStep(game, cfg::kFixedDt);
    if (!game.runActive) {
      break; // fell off — expected at track end
    }
  }
  // Difficulty must never exceed cap.
  return game.difficultyT <= cfg::kDifficultyMaxCap + 1e-6f &&
         game.hazardProbability <= cfg::kDiffHazardProbMax + 1e-6f &&
         game.diffSpeedBonus <= cfg::kDiffSpeedBonus + 1e-4f;
}

bool TestDeterministicDifficultyProgression() {
  Game a{};
  Game b{};
  InitGame(a, 0xBEEFu);
  InitGame(b, 0xBEEFu);

  for (int i = 0; i < 3600; ++i) { // 30 seconds
    const float moveX =
        ((i / 120) % 3 == 0) ? -1.0f : (((i / 120) % 3 == 1) ? 0.0f : 1.0f);
    a.input.moveX = moveX;
    b.input.moveX = moveX;
    if (i == 100 || i == 600 || i == 1800) {
      a.input.jumpQueued = true;
      b.input.jumpQueued = true;
    }
    if (i == 300 || i == 1200) {
      a.input.dashQueued = true;
      b.input.dashQueued = true;
    }
    SimStep(a, cfg::kFixedDt);
    SimStep(b, cfg::kFixedDt);
  }

  return NearlyEqual(a.difficultyT, b.difficultyT) &&
         NearlyEqual(a.diffSpeedBonus, b.diffSpeedBonus) &&
         NearlyEqual(a.hazardProbability, b.hazardProbability) &&
         NearlyEqual(a.player.velocity.z, b.player.velocity.z);
}

bool TestSubmitScoreQualifying() {
  Game game{};
  InitGame(game, 42u);

  // Set up a leaderboard with some scores
  game.leaderboardCount = 5;
  for (int i = 0; i < 5; ++i) {
    game.leaderboard[i].score = 10000.0f - (i * 1000.0f);
    game.leaderboard[i].runTime = 30.0f + i;
    game.leaderboard[i].seed = 100u + i;
  }

  // Set a score that should qualify (better than 5th place)
  game.distanceScore = 8000.0f;
  game.styleScore = 0.0f;
  game.runTime = 25.0f;
  game.runSeed = 999u;

  SubmitScore(game);

  // Should have pending score
  if (!NearlyEqual(game.pendingEntry.score, 8000.0f))
    return false;
  if (game.pendingEntry.name[0] != '\0')
    return false; // Name should be empty

  return true;
}

bool TestSubmitScoreNonQualifying() {
  Game game{};
  InitGame(game, 42u);

  // Set up a full leaderboard
  game.leaderboardCount = cfg::kLeaderboardSize;
  for (int i = 0; i < cfg::kLeaderboardSize; ++i) {
    game.leaderboard[i].score = 10000.0f - (i * 500.0f);
    game.leaderboard[i].runTime = 30.0f + i;
    game.leaderboard[i].seed = 100u + i;
  }

  // Set a score that doesn't qualify (worse than 10th place)
  game.distanceScore = 1000.0f;
  game.styleScore = 0.0f;
  game.runTime = 10.0f;

  SubmitScore(game);

  // Should NOT have pending score
  if (game.hasPendingScore)
    return false;

  return true;
}

bool TestFinalizeScoreEntry() {
  Game game{};
  InitGame(game, 42u);

  // Set up leaderboard
  game.leaderboardCount = 3;
  game.leaderboard[0].score = 5000.0f;
  game.leaderboard[1].score = 4000.0f;
  game.leaderboard[2].score = 3000.0f;

  // Set up pending entry
  game.hasPendingScore = true;
  game.pendingEntryIndex = 1; // Should insert at position 1
  game.pendingEntry.score = 4500.0f;
  game.pendingEntry.runTime = 25.0f;
  game.pendingEntry.seed = 123u;
  std::strncpy(game.nameInputBuffer, "TestPlayer",
               sizeof(game.nameInputBuffer) - 1);
  game.nameInputLength = 11;

  FinalizeScoreEntry(game);

  // Check that entry was inserted correctly
  if (game.hasPendingScore)
    return false;
  if (game.leaderboardCount != 4)
    return false;
  if (!NearlyEqual(game.leaderboard[1].score, 4500.0f))
    return false;
  if (std::strcmp(game.leaderboard[1].name, "TestPlayer") != 0)
    return false;
  if (game.leaderboard[1].seed != 123u)
    return false;

  // Check that other entries shifted correctly
  if (!NearlyEqual(game.leaderboard[0].score, 5000.0f))
    return false;
  if (!NearlyEqual(game.leaderboard[2].score, 4000.0f))
    return false;
  if (!NearlyEqual(game.leaderboard[3].score, 3000.0f))
    return false;

  return true;
}

bool TestFinalizeScoreEntryDefaultName() {
  Game game{};
  InitGame(game, 42u);

  game.leaderboardCount = 1;
  game.leaderboard[0].score = 5000.0f;

  game.hasPendingScore = true;
  game.pendingEntryIndex = 0;
  game.pendingEntry.score = 6000.0f;
  game.nameInputLength = 0; // Empty name

  FinalizeScoreEntry(game);

  // Should use default name "Player"
  if (std::strcmp(game.leaderboard[0].name, "Player") != 0)
    return false;

  return true;
}

bool TestCalculateLeaderboardStatsQualifying() {
  Game game{};
  InitGame(game, 42u);

  // Set up leaderboard
  game.leaderboardCount = 5;
  game.leaderboard[0].score = 10000.0f;
  game.leaderboard[1].score = 8000.0f;
  game.leaderboard[2].score = 6000.0f;
  game.leaderboard[3].score = 4000.0f;
  game.leaderboard[4].score = 2000.0f;

  // Set a score that qualifies
  game.distanceScore = 7000.0f;
  game.styleScore = 0.0f;
  game.runTime = 30.0f;

  CalculateLeaderboardStats(game);

  if (!game.leaderboardStats.scoreQualified)
    return false;
  if (game.leaderboardStats.rankIfQualified != 3)
    return false; // Should be 3rd place

  return true;
}

bool TestCalculateLeaderboardStatsNonQualifying() {
  Game game{};
  InitGame(game, 42u);

  // Set up full leaderboard
  game.leaderboardCount = cfg::kLeaderboardSize;
  for (int i = 0; i < cfg::kLeaderboardSize; ++i) {
    game.leaderboard[i].score = 10000.0f - (i * 500.0f);
    game.leaderboard[i].runTime = 30.0f + i;
  }

  // Set a score that doesn't qualify
  game.distanceScore = 2000.0f;
  game.styleScore = 0.0f;
  game.runTime = 20.0f;

  CalculateLeaderboardStats(game);

  if (game.leaderboardStats.scoreQualified)
    return false;
  if (game.leaderboardStats.rankIfQualified != 11)
    return false; // Would be 11th
  if (game.leaderboardStats.scoreDifference10th <= 0.0f)
    return false;
  if (game.leaderboardStats.scoreDifference1st <= 0.0f)
    return false;
  if (game.leaderboardStats.scorePercent10th <= 0.0f ||
      game.leaderboardStats.scorePercent10th >= 100.0f)
    return false;

  // Check that differences are correct
  const float expectedDiff10th =
      game.leaderboard[cfg::kLeaderboardSize - 1].score - 2000.0f;
  if (!NearlyEqual(game.leaderboardStats.scoreDifference10th, expectedDiff10th,
                   1.0f))
    return false;

  return true;
}

bool TestCalculateLeaderboardStatsEmpty() {
  Game game{};
  InitGame(game, 42u);

  game.leaderboardCount = 0;
  game.distanceScore = 1000.0f;
  game.styleScore = 0.0f;

  CalculateLeaderboardStats(game);

  // Empty leaderboard should mark as qualified
  if (!game.leaderboardStats.scoreQualified)
    return false;

  return true;
}

bool TestFinishZoneCompletion() {
  Game game{};
  InitGame(game, 42u);
  game.level = &GetLevel1();
  game.currentLevelIndex = 1;

  // Move player to just before finish zone
  const float finishEndZ = game.level->finish.endZ;
  game.player.position.z = finishEndZ - 1.0f;
  game.player.position.y = cfg::kPlayerHalfHeight;
  game.player.grounded = true;
  game.player.velocity.z = cfg::kForwardSpeed;

  // Step simulation - should not complete yet
  SimStep(game, cfg::kFixedDt);
  if (game.levelComplete || game.runOver)
    return false;

  // Move player past finish zone
  game.player.position.z = finishEndZ + 0.1f;
  SimStep(game, cfg::kFixedDt);

  // Should complete now
  if (!game.levelComplete)
    return false;
  if (!game.runOver)
    return false;
  if (game.deathCause != 3)
    return false; // deathCause 3 = level_complete

  return true;
}

bool TestFinishZoneNoPrematureCompletion() {
  Game game{};
  InitGame(game, 42u);
  game.level = &GetLevel1();
  game.currentLevelIndex = 1;

  // Move player to start of finish zone (but not past end)
  const float finishStartZ = game.level->finish.startZ;
  const float finishEndZ = game.level->finish.endZ;
  game.player.position.z = finishStartZ + (finishEndZ - finishStartZ) *
                                              0.5f; // Middle of finish zone
  game.player.position.y = cfg::kPlayerHalfHeight;
  game.player.grounded = true;
  game.player.velocity.z = cfg::kForwardSpeed;

  // Step simulation - should not complete yet (not past endZ)
  SimStep(game, cfg::kFixedDt);
  if (game.levelComplete || game.runOver)
    return false;

  return true;
}

bool TestFinishZoneDeterministic() {
  Game a{};
  Game b{};
  InitGame(a, 12345u);
  InitGame(b, 12345u);
  a.level = &GetLevel1();
  b.level = &GetLevel1();
  a.currentLevelIndex = 1;
  b.currentLevelIndex = 1;

  // Position both players identically just before finish zone
  const float finishEndZ = a.level->finish.endZ;
  a.player.position.z = finishEndZ - 5.0f;
  b.player.position.z = finishEndZ - 5.0f;
  a.player.position.y = cfg::kPlayerHalfHeight;
  b.player.position.y = cfg::kPlayerHalfHeight;
  a.player.grounded = true;
  b.player.grounded = true;
  a.player.velocity.z = cfg::kForwardSpeed;
  b.player.velocity.z = cfg::kForwardSpeed;

  // Run identical simulation steps
  const int steps = 200;
  for (int i = 0; i < steps; ++i) {
    SimStep(a, cfg::kFixedDt);
    SimStep(b, cfg::kFixedDt);

    // Both should complete at the same time
    if (a.levelComplete != b.levelComplete)
      return false;
    if (a.runOver != b.runOver)
      return false;

    if (a.levelComplete)
      break; // Stop once completion occurs
  }

  // Both should have completed
  if (!a.levelComplete || !b.levelComplete)
    return false;

  // Completion timing should be identical (same simTicks)
  if (a.simTicks != b.simTicks)
    return false;

  return true;
}

bool TestFinishZonePlaceholderLevel() {
  Game game{};
  InitGame(game, 42u);
  game.level = &GetLevelByIndex(5); // Unimplemented level (placeholder)
  game.currentLevelIndex = 5;

  // Placeholder levels should fall back to totalLength check
  game.player.position.z = game.level->totalLength - 1.0f;
  game.player.position.y = cfg::kPlayerHalfHeight;
  game.player.grounded = true;
  game.player.velocity.z = cfg::kForwardSpeed;

  // Should not complete yet
  SimStep(game, cfg::kFixedDt);
  if (game.levelComplete || game.runOver)
    return false;

  // Move past totalLength
  game.player.position.z = game.level->totalLength + 0.1f;
  SimStep(game, cfg::kFixedDt);

  // Should complete (fallback behavior)
  if (!game.levelComplete)
    return false;

  return true;
}

bool TestStartZoneSpawnPosition() {
  Game game{};
  InitGame(game, 42u);
  game.level = &GetLevel1();
  game.currentLevelIndex = 1;

  // Reset run to use start zone spawn
  ResetRun(game, 42u, 1);

  // Spawn Z should come from start zone
  const float expectedSpawnZ = game.level->start.spawnZ;
  if (!NearlyEqual(game.player.position.z, expectedSpawnZ, 0.01f))
    return false;

  // Player should be grounded and at correct height
  if (!game.player.grounded)
    return false;
  if (!NearlyEqual(game.player.position.y, cfg::kPlayerHalfHeight, 0.01f))
    return false;

  return true;
}

bool TestStartZoneSpawnSafe() {
  Game game{};
  InitGame(game, 42u);
  game.level = &GetLevel1();
  game.currentLevelIndex = 1;

  ResetRun(game, 42u, 1);

  // Player should be on a valid segment (not falling)
  const int segIdx =
      FindSegmentUnder(*game.level, game.player.position.z,
                       game.player.position.x, cfg::kPlayerWidth * 0.5f);
  if (segIdx < 0)
    return false; // Should be on a segment

  // Player should not collide with obstacles at spawn
  if (CheckObstacleCollision(
          *game.level, game.player.position, cfg::kPlayerWidth * 0.45f,
          cfg::kPlayerHalfHeight * 0.9f, cfg::kPlayerDepth * 0.45f)) {
    return false; // Should not spawn in obstacle
  }

  // Run a few steps - player should remain safe
  for (int i = 0; i < 10; ++i) {
    SimStep(game, cfg::kFixedDt);
    if (game.runOver && !game.levelComplete)
      return false; // Should not die immediately
  }

  return true;
}

bool TestStartZoneDeterministic() {
  Game a{};
  Game b{};
  InitGame(a, 12345u);
  InitGame(b, 12345u);

  // Both should spawn at same position with same seed
  ResetRun(a, 12345u, 1);
  ResetRun(b, 12345u, 1);

  if (!NearlyEqual(a.player.position.x, b.player.position.x, 0.01f))
    return false;
  if (!NearlyEqual(a.player.position.y, b.player.position.y, 0.01f))
    return false;
  if (!NearlyEqual(a.player.position.z, b.player.position.z, 0.01f))
    return false;
  if (a.player.grounded != b.player.grounded)
    return false;

  // Run a few steps - should remain deterministic
  for (int i = 0; i < 20; ++i) {
    SimStep(a, cfg::kFixedDt);
    SimStep(b, cfg::kFixedDt);

    if (!NearlyEqual(a.player.position.x, b.player.position.x, 0.01f))
      return false;
    if (!NearlyEqual(a.player.position.y, b.player.position.y, 0.01f))
      return false;
    if (!NearlyEqual(a.player.position.z, b.player.position.z, 0.01f))
      return false;
  }

  return true;
}

bool TestStartZonePlaceholderLevel() {
  Game game{};
  InitGame(game, 42u);
  game.level = &GetLevelByIndex(5); // Unimplemented level (placeholder)
  game.currentLevelIndex = 5;

  ResetRun(game, 42u, 5);

  // Placeholder levels should fall back to default spawn Z (2.0f)
  if (!NearlyEqual(game.player.position.z, 2.0f, 0.01f))
    return false;

  return true;
}

} // namespace

int main() {
  Log::Init();
  int failed = 0;

  auto run = [&](const char *name, const bool ok) {
    if (!ok) {
      std::cerr << "[FAIL] " << name << '\n';
      ++failed;
    } else {
      std::cout << "[PASS] " << name << '\n';
    }
  };

  run("jump_queue_reliability", TestJumpQueueReliability());
  run("repeated_jumps_after_landing", TestRepeatedJumpsAfterLanding());
  run("deterministic_sim_script", TestDeterministicSimScript());
  run("ground_clamp_on_platform", TestGroundClampOnPlatform());
  run("dash_impulse_grounded", TestDashImpulseGrounded());
  run("dash_cooldown_blocks_retrigger", TestDashCooldownBlocksRetrigger());
  run("air_control_bounded", TestAirControlBounded());
  run("fail_state_trigger", TestFailStateTrigger());
  run("deterministic_score_progression", TestDeterministicScoreProgression());
  run("multiplier_bounds", TestMultiplierBounds());
  run("restart_reset_invariants", TestRestartResetInvariants());
  run("difficulty_rises_monotonically", TestDifficultyRisesMonotonically());
  run("difficulty_cap", TestDifficultyCap());
  run("deterministic_difficulty_progression",
      TestDeterministicDifficultyProgression());
  run("submit_score_qualifying", TestSubmitScoreQualifying());
  run("submit_score_non_qualifying", TestSubmitScoreNonQualifying());
  run("finalize_score_entry", TestFinalizeScoreEntry());
  run("finalize_score_entry_default_name", TestFinalizeScoreEntryDefaultName());
  run("calculate_leaderboard_stats_qualifying",
      TestCalculateLeaderboardStatsQualifying());
  run("calculate_leaderboard_stats_non_qualifying",
      TestCalculateLeaderboardStatsNonQualifying());
  run("calculate_leaderboard_stats_empty",
      TestCalculateLeaderboardStatsEmpty());
  run("finish_zone_completion", TestFinishZoneCompletion());
  run("finish_zone_no_premature_completion",
      TestFinishZoneNoPrematureCompletion());
  run("finish_zone_deterministic", TestFinishZoneDeterministic());
  run("finish_zone_placeholder_level", TestFinishZonePlaceholderLevel());
  run("start_zone_spawn_position", TestStartZoneSpawnPosition());
  run("start_zone_spawn_safe", TestStartZoneSpawnSafe());
  run("start_zone_deterministic", TestStartZoneDeterministic());
  run("start_zone_placeholder_level", TestStartZonePlaceholderLevel());

  Log::Shutdown();
  return (failed == 0) ? 0 : 1;
}
