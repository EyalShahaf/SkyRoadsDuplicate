#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <string>

#include "core/Config.hpp"
#include "sim/Level.hpp"
#include "sim/EndlessLevelGenerator.hpp"
#include <raylib.h>

enum class GameScreen {
    MainMenu,
    LevelSelect,
    Playing,
    Paused,
    GameOver,
    NameEntry,
    Leaderboard,
    ExitConfirm,
    PlaceholderLevel,
};

struct PlayerSim {
    Vector3 position{};
    Vector3 velocity{};
    bool grounded = false;
    float jumpBufferTimer = 0.0f;
    float coyoteTimer = 0.0f;
    float dashTimer = 0.0f;
    float dashCooldownTimer = 0.0f;
};

struct InputState {
    float moveX = 0.0f;
    float throttleDelta = 0.0f;  // -1.0 to 1.0, throttle change input
    bool jumpQueued = false;
    bool dashQueued = false;
    bool restartSameQueued = false;
    bool restartNewQueued = false;
    bool cyclePaletteQueued = false;
    bool toggleBloomQueued = false;
};

struct LandingParticle {
    bool active = false;
    Vector3 position{};
    Vector3 velocity{};
    float life = 0.0f;
};

struct LeaderboardEntry {
    char name[20] = "Player";
    float score = 0.0f;
    float runTime = 0.0f;
    uint32_t seed = 0u;
};

struct LeaderboardStats {
    bool scoreQualified = false;           // Did this score make top 10?
    float scoreDifference10th = 0.0f;      // Points needed for 10th place
    float scoreDifference1st = 0.0f;       // Points needed for 1st place
    float scorePercent10th = 0.0f;        // Percentage of 10th place score
    float scorePercent1st = 0.0f;         // Percentage of 1st place score
    float timeDifference10th = 0.0f;      // Time difference vs 10th place (estimate)
    float timeDifference1st = 0.0f;       // Time difference vs 1st place (estimate)
    int rankIfQualified = -1;             // What rank this score would be (if > 10)
    bool leaderboardFull = false;         // Is leaderboard at capacity?
};

struct Game {
    GameScreen screen = GameScreen::MainMenu;
    int menuSelection = 0;
    int pauseSelection = 0;
    int exitConfirmSelection = 0;  // 0 = No, 1 = Yes
    bool wantsExit = false;

    PlayerSim player{};
    PlayerSim previousPlayer{};
    InputState input{};

    Camera3D camera{};
    Vector3 cameraPosition{};
    Vector3 cameraTarget{};
    float cameraRollDeg = 0.0f;
    bool bloomEnabled = false;
    std::array<LandingParticle, cfg::kLandingParticlePoolSize> landingParticles{};

    bool runActive = true;
    bool runOver = false;
    float runTime = 0.0f;
    float distanceScore = 0.0f;
    float styleScore = 0.0f;
    float scoreMultiplier = 1.0f;
    float bestScore = 0.0f;

    int paletteIndex = 0;
    uint32_t runSeed = 1u;

    float accumulator = 0.0f;
    uint64_t simTicks = 0;
    uint32_t rngState = 1u;

    float difficultyT = 0.0f;
    float diffSpeedBonus = 0.0f;
    float hazardProbability = 0.0f;

    // Throttle system
    float throttle = 0.5f;  // 0.0 to 1.0, controls forward speed

    const Level* level = nullptr;
    int currentLevelIndex = 1;  // 1-based level index (1-30)
    int currentStage = 1;  // Current stage (1-10), computed from currentLevelIndex
    bool levelComplete = false;
    int  deathCause = 0;  // 0=none, 1=fell, 2=obstacle, 3=level_complete
    bool isPlaceholderLevel = false;  // True if current level is a placeholder

    // Level selection screen state
    int levelSelectStage = 1;  // Currently selected stage (1-10)
    int levelSelectLevel = 1;  // Currently selected level within stage (1-3)

    // Endless Mode state
    bool isEndlessMode = false;
    float endlessStartZ = 0.0f;  // Starting Z position for distance calculation
    EndlessLevelGenerator endlessGenerator{};  // Procedural level generator for Endless Mode

    // Multiple leaderboards: key is level index (0 = Endless Mode, 1-30 = regular levels)
    std::map<int, std::array<LeaderboardEntry, cfg::kLeaderboardSize>> leaderboards{};
    std::map<int, int> leaderboardCounts{};  // Count for each leaderboard
    int currentLeaderboardIndex = 0;  // Currently viewed leaderboard (0 = Endless, 1-30 = levels)
    
    // Legacy single leaderboard (for backward compatibility during migration)
    std::array<LeaderboardEntry, cfg::kLeaderboardSize> leaderboard{};
    int leaderboardCount = 0;

    // Pending leaderboard entry (when score qualifies for top 10)
    bool hasPendingScore = false;
    LeaderboardEntry pendingEntry{};
    int pendingEntryIndex = -1;  // Where it will be inserted
    int pendingLeaderboardIndex = 0;  // Which leaderboard this entry is for
    char nameInputBuffer[20] = "";  // Current name being typed
    int nameInputLength = 0;

    // Statistics for non-qualifying scores
    LeaderboardStats leaderboardStats{};

    float updateMs = 0.0f;
    float renderMs = 0.0f;
    int   updateAllocCount = 0;

    // Screenshot notification
    float screenshotNotificationTimer = 0.0f;
    char screenshotPath[256] = {};
    bool screenshotRequested = false;
};

void InitGame(Game& game, uint32_t seed);
void ReadInput(Game& game);
void ApplyMetaActions(Game& game);
void ResetRun(Game& game, uint32_t seed, int levelIndex = 1);
float GetCurrentScore(const Game& game);
void SubmitScore(Game& game);
void FinalizeScoreEntry(Game& game);
void CalculateLeaderboardStats(Game& game);
void SaveLeaderboard(const Game& game);
void LoadLeaderboard(Game& game);
void SeedDefaultLeaderboard(Game& game);
