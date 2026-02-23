#include "game/Game.hpp"

#include "core/Config.hpp"
#include "core/Rng.hpp"
#include "render/Render.hpp"
#include "sim/EndlessLevelGenerator.hpp"

#include <cstdio>
#include <cstring>
#include <ctime>
#include <raylib.h>

namespace {

uint32_t NormalizeSeed(const uint32_t seed) { return (seed == 0) ? 1u : seed; }

} // namespace

void InitGame(Game &game, const uint32_t seed) {
  game.runSeed = NormalizeSeed(seed);
  game.rngState = game.runSeed;
  game.bloomEnabled = false;

  game.camera.up = {0.0f, 1.0f, 0.0f};
  game.camera.fovy = cfg::kCameraBaseFov;
  game.camera.projection = CAMERA_PERSPECTIVE;

  LoadLeaderboard(game);
  game.screen = GameScreen::MainMenu;
  game.menuSelection = 0;
}

void ResetRun(Game &game, const uint32_t seed, int levelIndex) {
  game.runSeed = NormalizeSeed(seed);
  game.rngState = game.runSeed;
  game.currentLevelIndex = levelIndex;
  game.currentStage = (levelIndex - 1) / 3 + 1;
  game.isEndlessMode = (levelIndex == 0);  // 0 = Endless Mode

  if (game.isEndlessMode) {
    // Initialize Endless Mode
    game.endlessGenerator.Initialize(game.runSeed);
    game.level = &game.endlessGenerator.GetLevel();
    game.isPlaceholderLevel = false;
    game.endlessStartZ = 0.0f;
    
    float spawnZ = GetSpawnZ(*game.level);
    game.player.position = {0.0f, 1.0f, spawnZ};
    game.player.velocity = {0.0f, 0.0f, cfg::kForwardSpeed};
    game.player.grounded = false;
    
    // Ensure player starts grounded if spawning on a segment
    int segIdx =
        FindSegmentUnder(*game.level, spawnZ, 0.0f, cfg::kPlayerWidth * 0.5f);
    if (segIdx >= 0) {
      game.player.position.y =
          game.level->segments[segIdx].topY + cfg::kPlayerHalfHeight;
      game.player.grounded = true;
    }
  } else {
    // Regular level
    game.level = &GetLevelByIndex(levelIndex);
    game.isPlaceholderLevel = (game.level->finish.style == FinishStyle::None);

    float spawnZ = GetSpawnZ(*game.level);
    game.player.position = {0.0f, 1.0f, spawnZ};
    game.player.velocity = {0.0f, 0.0f, cfg::kForwardSpeed};
    game.player.grounded = false;

    // Ensure player starts grounded if spawning on a segment
    int segIdx =
        FindSegmentUnder(*game.level, spawnZ, 0.0f, cfg::kPlayerWidth * 0.5f);
    if (segIdx >= 0) {
      game.player.position.y =
          game.level->segments[segIdx].topY + cfg::kPlayerHalfHeight;
      game.player.grounded = true;
    }
  }

  game.previousPlayer = game.player;
  game.runTime = 0.0f;
  game.distanceScore = 0.0f;
  game.styleScore = 0.0f;
  game.scoreMultiplier = 1.0f;
  game.difficultyT = 0.0f;
  game.diffSpeedBonus = 0.0f;
  game.hazardProbability = cfg::kDiffHazardProbMin;
  game.runActive = true;
  game.runOver = false;
  game.levelComplete = false;
  game.deathCause = 0;
  game.throttle = 0.5f;
  game.input = {}; // Clear any buffered inputs

  for (auto &p : game.landingParticles)
    p.active = false;

  RegenerateSpaceObjects(game.runSeed);
  game.screen = game.isPlaceholderLevel ? GameScreen::PlaceholderLevel
                                        : GameScreen::Playing;
}

void ReadInput(Game &game) {
  game.input.moveX = 0.0f;
  game.input.throttleDelta = 0.0f;
  const auto &k = cfg::keys;

  if (game.screen == GameScreen::Playing) {
    if (IsKeyDown(k.left) || IsKeyDown(k.leftAlt))
      game.input.moveX -= 1.0f;
    if (IsKeyDown(k.right) || IsKeyDown(k.rightAlt))
      game.input.moveX += 1.0f;

    if (IsKeyDown(k.up) || IsKeyDown(k.upAlt))
      game.input.throttleDelta += 1.0f;
    if (IsKeyDown(k.down) || IsKeyDown(k.downAlt))
      game.input.throttleDelta -= 1.0f;

    if (IsKeyPressed(k.jump) || IsKeyPressed(k.jumpAlt))
      game.input.jumpQueued = true;
    if (IsKeyPressed(k.dash) || IsKeyPressed(k.dashAlt))
      game.input.dashQueued = true;

    if (IsKeyPressed(k.pause) || IsKeyPressed(k.back)) {
      game.screen = GameScreen::Paused;
      game.pauseSelection = 0;
    }

    // Transition out of Playing if run is over
    if (game.runOver) {
      SubmitScore(game);
      // SubmitScore already sets screen to NameEntry if it qualifies,
      // otherwise we should go to GameOver.
      if (game.screen == GameScreen::Playing ||
          game.screen == GameScreen::PlaceholderLevel) {
        game.screen = GameScreen::GameOver;
      }
    }
  } else if (game.screen == GameScreen::MainMenu) {
    if (IsKeyPressed(k.up))
      game.menuSelection = (game.menuSelection + 3) % 4;  // 4 menu items now
    if (IsKeyPressed(k.down))
      game.menuSelection = (game.menuSelection + 1) % 4;
    if (IsKeyPressed(k.confirm)) {
      if (game.menuSelection == 0) {
        // Start Game - Go to Level Select
        game.screen = GameScreen::LevelSelect;
        game.levelSelectStage = 1;
        game.levelSelectLevel = 1;
      } else if (game.menuSelection == 1) {
        // Endless Mode
        ResetRun(game, (uint32_t)std::time(nullptr), 0);
      } else if (game.menuSelection == 2) {
        game.screen = GameScreen::Leaderboard;
        // Find first available leaderboard (prefer Endless Mode, then level 1, etc.)
        if (game.leaderboards.find(0) != game.leaderboards.end()) {
          game.currentLeaderboardIndex = 0;
        } else {
          // Find first available leaderboard
          for (int i = 1; i <= 30; ++i) {
            if (game.leaderboards.find(i) != game.leaderboards.end()) {
              game.currentLeaderboardIndex = i;
              break;
            }
          }
        }
      } else if (game.menuSelection == 3) {
        game.screen = GameScreen::ExitConfirm;
        game.exitConfirmSelection = 0;
      }
    }
    if (IsKeyPressed(k.back)) {
      game.screen = GameScreen::ExitConfirm;
      game.exitConfirmSelection = 0;
    }
  } else if (game.screen == GameScreen::LevelSelect) {
    if (IsKeyPressed(k.back))
      game.screen = GameScreen::MainMenu;

    if (IsKeyPressed(k.up)) {
      if (game.levelSelectLevel > 1)
        game.levelSelectLevel--;
      else if (game.levelSelectStage > 5)
        game.levelSelectStage -= 5;
    }
    if (IsKeyPressed(k.down)) {
      if (game.levelSelectLevel < 3)
        game.levelSelectLevel++;
      else if (game.levelSelectStage <= 5)
        game.levelSelectStage += 5;
    }
    if (IsKeyPressed(k.left)) {
      if (game.levelSelectStage > 1)
        game.levelSelectStage--;
    }
    if (IsKeyPressed(k.right)) {
      if (game.levelSelectStage < 10)
        game.levelSelectStage++;
    }

    if (IsKeyPressed(k.confirm)) {
      int idx = GetLevelIndexFromStageAndLevel(game.levelSelectStage,
                                               game.levelSelectLevel);
      ResetRun(game, (uint32_t)std::time(nullptr), idx);
    }
  } else if (game.screen == GameScreen::Paused) {
    if (IsKeyPressed(k.up))
      game.pauseSelection = (game.pauseSelection + 2) % 3;
    if (IsKeyPressed(k.down))
      game.pauseSelection = (game.pauseSelection + 1) % 3;
    if (IsKeyPressed(k.confirm)) {
      if (game.pauseSelection == 0)
        game.screen = GameScreen::Playing;
      else if (game.pauseSelection == 1)
        ResetRun(game, game.runSeed, game.currentLevelIndex);
      else if (game.pauseSelection == 2)
        game.screen = GameScreen::MainMenu;
    }
    if (IsKeyPressed(k.pause) || IsKeyPressed(k.back))
      game.screen = GameScreen::Playing;
  } else if (game.screen == GameScreen::GameOver) {
    if (IsKeyPressed(k.restartSame))
      game.input.restartSameQueued = true;
    if (IsKeyPressed(k.restartNew))
      game.input.restartNewQueued = true;
    if (IsKeyPressed(k.back))
      game.screen = GameScreen::MainMenu;
  } else if (game.screen == GameScreen::NameEntry) {
    // Handle text input
    int key = GetCharPressed();
    while (key > 0) {
      if ((key >= 32) && (key <= 125) && (game.nameInputLength < 19)) {
        game.nameInputBuffer[game.nameInputLength] = (char)key;
        game.nameInputBuffer[game.nameInputLength + 1] = '\0';
        game.nameInputLength++;
      }
      key = GetCharPressed();
    }
    if (IsKeyPressed(k.backspace)) {
      if (game.nameInputLength > 0) {
        game.nameInputLength--;
        game.nameInputBuffer[game.nameInputLength] = '\0';
      }
    }
    if (IsKeyPressed(k.confirm)) {
      FinalizeScoreEntry(game);
    }
    if (IsKeyPressed(k.back)) {
      game.hasPendingScore = false;
      game.screen = GameScreen::GameOver;
    }
  } else if (game.screen == GameScreen::Leaderboard) {
    if (IsKeyPressed(k.back) || IsKeyPressed(k.confirm))
      game.screen = GameScreen::MainMenu;
    // Navigate between leaderboards with left/right keys
    if (IsKeyPressed(k.left)) {
      // Find previous leaderboard
      int targetIndex = game.currentLeaderboardIndex;
      int attempts = 0;
      do {
        targetIndex--;
        if (targetIndex < 0) {
          // Wrap to highest available index
          int maxIndex = 0;
          for (const auto& [idx, _] : game.leaderboards) {
            if (idx > maxIndex) maxIndex = idx;
          }
          targetIndex = maxIndex;
        }
        attempts++;
      } while (game.leaderboards.find(targetIndex) == game.leaderboards.end() && attempts < 32);
      if (targetIndex >= 0 && game.leaderboards.find(targetIndex) != game.leaderboards.end()) {
        game.currentLeaderboardIndex = targetIndex;
      }
    }
    if (IsKeyPressed(k.right)) {
      // Find next leaderboard
      int targetIndex = game.currentLeaderboardIndex;
      int attempts = 0;
      do {
        targetIndex++;
        // Wrap to 0 (Endless Mode) if we go past the highest index
        int maxIndex = 0;
        for (const auto& [idx, _] : game.leaderboards) {
          if (idx > maxIndex) maxIndex = idx;
        }
        if (targetIndex > maxIndex) {
          targetIndex = 0;
        }
        attempts++;
      } while (game.leaderboards.find(targetIndex) == game.leaderboards.end() && attempts < 32);
      if (targetIndex >= 0 && targetIndex <= 30 && game.leaderboards.find(targetIndex) != game.leaderboards.end()) {
        game.currentLeaderboardIndex = targetIndex;
      }
    }
  } else if (game.screen == GameScreen::ExitConfirm) {
    if (IsKeyPressed(k.left) || IsKeyPressed(k.up))
      game.exitConfirmSelection = 0;
    if (IsKeyPressed(k.right) || IsKeyPressed(k.down))
      game.exitConfirmSelection = 1;
    if (IsKeyPressed(k.confirm)) {
      if (game.exitConfirmSelection == 1)
        game.wantsExit = true;
      else
        game.screen = GameScreen::MainMenu;
    }
    if (IsKeyPressed(k.back))
      game.screen = GameScreen::MainMenu;
  } else if (game.screen == GameScreen::PlaceholderLevel) {
    if (GetKeyPressed() > 0)
      game.screen = GameScreen::LevelSelect;
  }

  // Global keys
  if (IsKeyPressed(k.screenshot))
    game.screenshotRequested = true;
  if (IsKeyPressed(k.cyclePalette))
    game.input.cyclePaletteQueued = true;
  if (IsKeyPressed(k.toggleBloom))
    game.input.toggleBloomQueued = true;
}

void ApplyMetaActions(Game &game) {
  if (game.input.restartSameQueued) {
    ResetRun(game, game.runSeed, game.currentLevelIndex);
    game.input.restartSameQueued = false;
  } else if (game.input.restartNewQueued) {
    ResetRun(game, (uint32_t)std::time(nullptr), game.currentLevelIndex);
    game.input.restartNewQueued = false;
  }

  if (game.input.cyclePaletteQueued) {
    game.paletteIndex = (game.paletteIndex + 1) % 4;
    game.input.cyclePaletteQueued = false;
  }

  if (game.input.toggleBloomQueued) {
    game.bloomEnabled = !game.bloomEnabled;
    game.input.toggleBloomQueued = false;
  }
}
