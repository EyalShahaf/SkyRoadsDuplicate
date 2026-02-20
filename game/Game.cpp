#include "game/Game.hpp"

#include "core/Config.hpp"
#include "core/Rng.hpp"
#include "render/Render.hpp"

#include <cstdio>
#include <cstring>
#include <ctime>
#include <raylib.h>

namespace {
uint32_t NormalizeSeed(const uint32_t seed) {
    return (seed == 0u) ? 1u : seed;
}

constexpr const char* kLeaderboardFile = "leaderboard.dat";

void SetEntryName(LeaderboardEntry& e, const char* n) {
    std::strncpy(e.name, n, sizeof(e.name) - 1);
    e.name[sizeof(e.name) - 1] = '\0';
}
}  // namespace

void ResetRun(Game& game, const uint32_t seed, int levelIndex) {
    game.runSeed = NormalizeSeed(seed);
    // Clamp levelIndex to valid range (1-30)
    if (levelIndex < 1) levelIndex = 1;
    if (levelIndex > 30) levelIndex = 30;
    game.currentLevelIndex = levelIndex;
    game.currentStage = GetStageFromLevelIndex(levelIndex);
    game.isPlaceholderLevel = !IsLevelImplemented(levelIndex);
    
    uint32_t spawnState = game.runSeed;
    const float spawnX = (core::NextFloat01(spawnState) - 0.5f) * 1.5f;

    game.player.position = Vector3{spawnX, cfg::kPlayerHalfHeight, 2.0f};
    game.player.velocity = Vector3{0.0f, 0.0f, cfg::kForwardSpeed};
    game.player.grounded = true;
    game.player.jumpBufferTimer = 0.0f;
    game.player.coyoteTimer = cfg::kCoyoteTime;
    game.player.dashTimer = 0.0f;
    game.player.dashCooldownTimer = 0.0f;

    game.previousPlayer = game.player;
    game.input = {};
    game.runActive = true;
    game.runOver = false;
    game.runTime = 0.0f;
    game.distanceScore = 0.0f;
    game.styleScore = 0.0f;
    game.scoreMultiplier = cfg::kScoreMultiplierMin;
    game.accumulator = 0.0f;
    game.difficultyT = 0.0f;
    game.diffSpeedBonus = 0.0f;
    game.hazardProbability = cfg::kDiffHazardProbMin;
    // Set level using GetLevelByIndex
    game.level = &GetLevelByIndex(levelIndex);
    game.levelComplete = false;
    game.deathCause = 0;

    // Regenerate space objects for this level
    RegenerateSpaceObjects(game.runSeed);

    // Clear pending score state
    game.hasPendingScore = false;
    game.pendingEntryIndex = -1;
    game.nameInputBuffer[0] = '\0';
    game.nameInputLength = 0;
    game.leaderboardStats = {};

    game.camera.position = Vector3{0.0f, 4.0f, -8.0f};
    game.camera.target = Vector3{0.0f, 1.0f, 4.0f};
    game.camera.up = Vector3{0.0f, 1.0f, 0.0f};
    game.camera.fovy = cfg::kCameraBaseFov;
    game.camera.projection = CAMERA_PERSPECTIVE;
    game.cameraPosition = game.camera.position;
    game.cameraTarget = game.camera.target;
    game.cameraRollDeg = 0.0f;
    for (auto& p : game.landingParticles) {
        p = {};
    }
}

void SeedDefaultLeaderboard(Game& game) {
    // Funny defaults featuring Eyal and the original SkyRoads creators
    // (BlueMoon Interactive / Creative Dimensions — Estonian legends).
    struct { const char* name; float score; float time; uint32_t seed; } defaults[] = {
        {"Eyal 'NeonKing'",  42000.0f,  88.5f, 0xC0FFEEu},
        {"Kosmonaut Olav",   38500.0f,  82.1f, 0x54AD04Du},
        {"Bluemoon Dev #1",  31200.0f,  74.3f, 0xB10EB00u},
        {"Eyal The OG",      27800.0f,  65.0f, 0xDEADBEEFu},
        {"SkyRoads Ghost",   24100.0f,  58.2f, 0x19930E5u},
        {"Pixel Priit",      19500.0f,  50.7f, 0x7A111AAu},
        {"Eyal Jr.",         15200.0f,  42.4f, 0xCAFEBABEu},
        {"DOS Runner 3000",  11000.0f,  35.1f, 0xD05B007u},
        {"Cosmic Lembit",    7500.0f,   28.6f, 0xC05A1Cu},
        {"Baby's 1st Run",   3200.0f,   18.0f, 0x0000001u},
    };

    game.leaderboardCount = cfg::kLeaderboardSize;
    for (int i = 0; i < cfg::kLeaderboardSize; ++i) {
        SetEntryName(game.leaderboard[i], defaults[i].name);
        game.leaderboard[i].score = defaults[i].score;
        game.leaderboard[i].runTime = defaults[i].time;
        game.leaderboard[i].seed = defaults[i].seed;
    }
    SaveLeaderboard(game);
}

void InitGame(Game& game, const uint32_t seed) {
    game.rngState = NormalizeSeed(seed);
    game.bestScore = 0.0f;
    game.paletteIndex = 0;
    game.bloomEnabled = false;
    game.simTicks = 0;
    game.screen = GameScreen::MainMenu;
    game.menuSelection = 0;
    game.pauseSelection = 0;
    game.exitConfirmSelection = 0;
    game.wantsExit = false;
    game.currentLevelIndex = 1;
    game.screenshotNotificationTimer = 0.0f;
    game.screenshotPath[0] = '\0';
    game.screenshotRequested = false;
    game.leaderboardCount = 0;
    LoadLeaderboard(game);
    if (game.leaderboardCount == 0) {
        SeedDefaultLeaderboard(game);
    }
    ResetRun(game, game.rngState, 1);
}

void ReadInput(Game& game) {
    // Exit confirmation screen.
    if (game.screen == GameScreen::ExitConfirm) {
        if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
            game.exitConfirmSelection = (game.exitConfirmSelection + 1) % 2;
        }
        if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
            game.exitConfirmSelection = (game.exitConfirmSelection + 1) % 2;
        }
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
            if (game.exitConfirmSelection == 1) {
                game.wantsExit = true;
            } else {
                game.screen = GameScreen::MainMenu;
                game.exitConfirmSelection = 0;
            }
        }
        if (IsKeyPressed(KEY_ESCAPE)) {
            game.screen = GameScreen::MainMenu;
            game.exitConfirmSelection = 0;
        }
        return;
    }

    // Menu navigation.
    if (game.screen == GameScreen::MainMenu) {
        if (IsKeyPressed(KEY_ESCAPE)) {
            game.exitConfirmSelection = 0;  // Default to "No"
            game.screen = GameScreen::ExitConfirm;
            return;
        }
        if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
            game.menuSelection = (game.menuSelection + 2) % 3;
        }
        if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
            game.menuSelection = (game.menuSelection + 1) % 3;
        }
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
            if (game.menuSelection == 0) {
                game.levelSelectStage = 1;
                game.levelSelectLevel = 1;
                game.screen = GameScreen::LevelSelect;
            } else if (game.menuSelection == 1) {
                game.screen = GameScreen::Leaderboard;
            } else {
                game.exitConfirmSelection = 0;  // Default to "No"
                game.screen = GameScreen::ExitConfirm;
            }
        }
        return;
    }

    // Level selection screen
    if (game.screen == GameScreen::LevelSelect) {
        if (IsKeyPressed(KEY_ESCAPE)) {
            game.screen = GameScreen::MainMenu;
            game.menuSelection = 0;
            return;
        }
        if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
            game.levelSelectStage = (game.levelSelectStage - 1);
            if (game.levelSelectStage < 1) game.levelSelectStage = 10;
        }
        if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
            game.levelSelectStage = (game.levelSelectStage + 1);
            if (game.levelSelectStage > 10) game.levelSelectStage = 1;
        }
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
            game.levelSelectLevel = (game.levelSelectLevel - 1);
            if (game.levelSelectLevel < 1) game.levelSelectLevel = 3;
        }
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
            game.levelSelectLevel = (game.levelSelectLevel + 1);
            if (game.levelSelectLevel > 3) game.levelSelectLevel = 1;
        }
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
            const int levelIndex = GetLevelIndexFromStageAndLevel(game.levelSelectStage, game.levelSelectLevel);
            if (IsLevelImplemented(levelIndex)) {
                ResetRun(game, core::NextU32(game.rngState), levelIndex);
                game.simTicks = 0;
                game.screen = GameScreen::Playing;
            } else {
                // Placeholder level - show placeholder screen
                ResetRun(game, core::NextU32(game.rngState), levelIndex);
                game.simTicks = 0;
                game.screen = GameScreen::PlaceholderLevel;
            }
        }
        return;
    }

    // Placeholder level screen
    if (game.screen == GameScreen::PlaceholderLevel) {
        // Return to level selection on any key press
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || 
            IsKeyPressed(KEY_BACKSPACE)) {
            game.screen = GameScreen::LevelSelect;
        }
        // Also check for any other key
        for (int key = KEY_A; key <= KEY_Z; ++key) {
            if (IsKeyPressed(key)) {
                game.screen = GameScreen::LevelSelect;
                break;
            }
        }
        return;
    }

    // Leaderboard: ESC/ENTER/SPACE/BACKSPACE go back to menu (NOT exit game).
    if (game.screen == GameScreen::Leaderboard) {
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER) ||
            IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_BACKSPACE)) {
            game.screen = GameScreen::MainMenu;
            game.menuSelection = 0;
        }
        return;
    }

    // Pause menu.
    if (game.screen == GameScreen::Paused) {
        if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
            game.pauseSelection = (game.pauseSelection + 2) % 3;
        }
        if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
            game.pauseSelection = (game.pauseSelection + 1) % 3;
        }
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)) {
            // P key can resume from pause OR take screenshot, but screenshot takes priority
            // So if P is pressed in pause menu, just resume (screenshot won't trigger in pause anyway)
            game.screen = GameScreen::Playing;
            return;
        }
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
            if (game.pauseSelection == 0) {
                game.screen = GameScreen::Playing;
            } else if (game.pauseSelection == 1) {
                ResetRun(game, game.runSeed, game.currentLevelIndex);
                game.simTicks = 0;
                game.screen = GameScreen::Playing;
            } else {
                game.screen = GameScreen::MainMenu;
                game.menuSelection = 0;
            }
        }
        return;
    }

    // Name entry screen
    if (game.screen == GameScreen::NameEntry) {
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

        // Handle backspace
        if (IsKeyPressed(KEY_BACKSPACE) && game.nameInputLength > 0) {
            game.nameInputLength--;
            game.nameInputBuffer[game.nameInputLength] = '\0';
        }

        // Enter to confirm
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
            FinalizeScoreEntry(game);
            game.screen = GameScreen::GameOver;
            return;
        }

        // ESC to skip (use default name)
        if (IsKeyPressed(KEY_ESCAPE)) {
            FinalizeScoreEntry(game);
            game.screen = GameScreen::GameOver;
            return;
        }
        return;
    }

    // Game over.
    if (game.screen == GameScreen::GameOver) {
        if (IsKeyPressed(KEY_R)) {
            ResetRun(game, game.runSeed, game.currentLevelIndex);
            game.simTicks = 0;
            game.screen = GameScreen::Playing;
            return;
        }
        if (IsKeyPressed(KEY_N)) {
            game.levelSelectStage = GetStageFromLevelIndex(game.currentLevelIndex);
            game.levelSelectLevel = GetLevelInStageFromLevelIndex(game.currentLevelIndex);
            game.screen = GameScreen::LevelSelect;
            return;
        }
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER)) {
            game.levelSelectStage = GetStageFromLevelIndex(game.currentLevelIndex);
            game.levelSelectLevel = GetLevelInStageFromLevelIndex(game.currentLevelIndex);
            game.screen = GameScreen::LevelSelect;
            return;
        }
        return;
    }

    // --- Playing ---
    if (IsKeyPressed(KEY_ESCAPE)) {
        game.pauseSelection = 0;
        game.screen = GameScreen::Paused;
        return;
    }

    float moveX = 0.0f;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) {
        moveX -= 1.0f;
    }
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) {
        moveX += 1.0f;
    }
    game.input.moveX = moveX;

    if (IsKeyPressed(KEY_SPACE)) game.input.jumpQueued = true;
    if (IsKeyPressed(KEY_LEFT_SHIFT) || IsKeyPressed(KEY_RIGHT_SHIFT)) game.input.dashQueued = true;
    if (IsKeyPressed(KEY_R)) game.input.restartSameQueued = true;
    if (IsKeyPressed(KEY_N)) game.input.restartNewQueued = true;
    if (IsKeyPressed(KEY_TAB)) game.input.cyclePaletteQueued = true;
    if (IsKeyPressed(KEY_B)) game.input.toggleBloomQueued = true;
    if (IsKeyPressed(KEY_O)) {
        // Request screenshot (will be taken after rendering)
        game.screenshotRequested = true;
    }
}

void ApplyMetaActions(Game& game) {
    // Update screenshot notification timer (works in all screens)
    if (game.screenshotNotificationTimer > 0.0f) {
        game.screenshotNotificationTimer -= GetFrameTime();
        if (game.screenshotNotificationTimer < 0.0f) {
            game.screenshotNotificationTimer = 0.0f;
        }
    }

    if (game.screen != GameScreen::Playing) return;

    if (game.input.cyclePaletteQueued) {
        game.paletteIndex = (game.paletteIndex + 1) % cfg::kPaletteCount;
        game.input.cyclePaletteQueued = false;
    }
    if (game.input.toggleBloomQueued) {
        game.bloomEnabled = !game.bloomEnabled;
        game.input.toggleBloomQueued = false;
    }

    if (game.runOver) {
        // Check if placeholder level - return to level selection immediately
        if (game.isPlaceholderLevel) {
            game.screen = GameScreen::LevelSelect;
            return;
        }
        // Otherwise, show game over screen
        SubmitScore(game);
        CalculateLeaderboardStats(game);  // Calculate stats
        if (game.hasPendingScore) {
            // Score qualifies for top 10, show name entry
            game.screen = GameScreen::NameEntry;
        } else {
            // Score doesn't qualify, go straight to game over
            game.screen = GameScreen::GameOver;
        }
        return;
    }

    if (game.input.restartNewQueued) {
        ResetRun(game, core::NextU32(game.rngState), game.currentLevelIndex);
        game.simTicks = 0;
        game.input.restartNewQueued = false;
        game.input.restartSameQueued = false;
        return;
    }

    if (game.input.restartSameQueued) {
        ResetRun(game, game.runSeed, game.currentLevelIndex);
        game.simTicks = 0;
        game.input.restartSameQueued = false;
    }
}

float GetCurrentScore(const Game& game) {
    return game.distanceScore + game.styleScore;
}

void SubmitScore(Game& game) {
    const float score = GetCurrentScore(game);
    if (score <= 0.0f) return;

    if (score > game.bestScore) {
        game.bestScore = score;
    }

    // Check if score qualifies for top 10
    int insertIdx = game.leaderboardCount;
    for (int i = 0; i < game.leaderboardCount; ++i) {
        if (score > game.leaderboard[i].score) {
            insertIdx = i;
            break;
        }
    }

    if (insertIdx >= cfg::kLeaderboardSize) {
        // Score doesn't qualify for top 10, don't save
        return;
    }

    // Score qualifies! Prepare pending entry but don't insert yet
    game.hasPendingScore = true;
    game.pendingEntryIndex = insertIdx;
    game.pendingEntry.score = score;
    game.pendingEntry.runTime = game.runTime;
    game.pendingEntry.seed = game.runSeed;
    SetEntryName(game.pendingEntry, "");  // Empty name, user will enter it
    
    // Initialize name input
    game.nameInputBuffer[0] = '\0';
    game.nameInputLength = 0;
}

void FinalizeScoreEntry(Game& game) {
    if (!game.hasPendingScore) return;

    // Use entered name, or default to "Player" if empty
    if (game.nameInputLength == 0) {
        SetEntryName(game.pendingEntry, "Player");
    } else {
        SetEntryName(game.pendingEntry, game.nameInputBuffer);
    }

    // Insert into leaderboard
    const int lastIdx = (game.leaderboardCount < cfg::kLeaderboardSize)
                            ? game.leaderboardCount
                            : cfg::kLeaderboardSize - 1;
    for (int i = lastIdx; i > game.pendingEntryIndex; --i) {
        game.leaderboard[i] = game.leaderboard[i - 1];
    }
    game.leaderboard[game.pendingEntryIndex] = game.pendingEntry;

    if (game.leaderboardCount < cfg::kLeaderboardSize) {
        ++game.leaderboardCount;
    }

    SaveLeaderboard(game);
    
    // Clear pending state
    game.hasPendingScore = false;
    game.pendingEntryIndex = -1;
    game.nameInputBuffer[0] = '\0';
    game.nameInputLength = 0;
}

void CalculateLeaderboardStats(Game& game) {
    const float currentScore = GetCurrentScore(game);
    game.leaderboardStats = {};  // Reset
    
    if (game.leaderboardCount == 0) {
        // Empty leaderboard - show encouraging message
        game.leaderboardStats.scoreQualified = true;  // Technically qualifies!
        return;
    }
    
    game.leaderboardStats.leaderboardFull = (game.leaderboardCount >= cfg::kLeaderboardSize);
    
    // Find where this score would rank
    int insertIdx = game.leaderboardCount;
    for (int i = 0; i < game.leaderboardCount; ++i) {
        if (currentScore > game.leaderboard[i].score) {
            insertIdx = i;
            break;
        }
    }
    
    game.leaderboardStats.scoreQualified = (insertIdx < cfg::kLeaderboardSize);
    game.leaderboardStats.rankIfQualified = (insertIdx < cfg::kLeaderboardSize) 
        ? (insertIdx + 1) 
        : (game.leaderboardCount + 1);
    
    // Calculate differences from 1st place
    if (game.leaderboardCount > 0) {
        const float firstPlaceScore = game.leaderboard[0].score;
        game.leaderboardStats.scoreDifference1st = firstPlaceScore - currentScore;
        if (firstPlaceScore > 0.0f) {
            game.leaderboardStats.scorePercent1st = (currentScore / firstPlaceScore) * 100.0f;
        }
        
        if (game.leaderboard[0].runTime > 0.0f && game.runTime > 0.0f) {
            // Estimate time needed (rough approximation based on score/time ratio)
            const float scorePerSecond = currentScore / game.runTime;
            if (scorePerSecond > 0.0f) {
                game.leaderboardStats.timeDifference1st = 
                    game.leaderboardStats.scoreDifference1st / scorePerSecond;
            }
        }
    }
    
    // Calculate differences from 10th place (or last place if < 10 entries)
    const int lastPlaceIdx = (game.leaderboardCount < cfg::kLeaderboardSize)
        ? (game.leaderboardCount - 1)
        : (cfg::kLeaderboardSize - 1);
    
    if (lastPlaceIdx >= 0 && !game.leaderboardStats.scoreQualified) {
        const float lastPlaceScore = game.leaderboard[lastPlaceIdx].score;
        game.leaderboardStats.scoreDifference10th = lastPlaceScore - currentScore;
        if (lastPlaceScore > 0.0f) {
            game.leaderboardStats.scorePercent10th = (currentScore / lastPlaceScore) * 100.0f;
        }
        
        if (game.leaderboard[lastPlaceIdx].runTime > 0.0f && game.runTime > 0.0f) {
            // Estimate time needed based on score/time ratio
            const float scorePerSecond = currentScore / game.runTime;
            if (scorePerSecond > 0.0f) {
                game.leaderboardStats.timeDifference10th = 
                    game.leaderboardStats.scoreDifference10th / scorePerSecond;
            }
        }
    }
}

void SaveLeaderboard(const Game& game) {
    FILE* f = std::fopen(kLeaderboardFile, "wb");
    if (!f) return;
    if (std::fwrite(&game.leaderboardCount, sizeof(int), 1, f) != 1) {
        std::fclose(f);
        return;
    }
    if (game.leaderboardCount > 0) {
        if (std::fwrite(game.leaderboard.data(), sizeof(LeaderboardEntry), game.leaderboardCount, f) != static_cast<size_t>(game.leaderboardCount)) {
            std::fclose(f);
            return;
        }
    }
    std::fclose(f);
}

void LoadLeaderboard(Game& game) {
    FILE* f = std::fopen(kLeaderboardFile, "rb");
    if (!f) {
        game.leaderboardCount = 0;
        return;
    }
    int count = 0;
    if (std::fread(&count, sizeof(int), 1, f) != 1) {
        std::fclose(f);
        game.leaderboardCount = 0;
        return;
    }
    if (count < 0 || count > cfg::kLeaderboardSize) {
        count = 0;
    }
    if (count > 0) {
        const size_t itemsRead = std::fread(game.leaderboard.data(), sizeof(LeaderboardEntry), count, f);
        game.leaderboardCount = static_cast<int>(itemsRead);
    } else {
        game.leaderboardCount = 0;
    }
    std::fclose(f);
}
