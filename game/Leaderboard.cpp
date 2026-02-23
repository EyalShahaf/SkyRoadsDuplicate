#include "game/Game.hpp"

#include "core/Config.hpp"
#include <cstdio>
#include <cstring>
#include <ctime>
#include <algorithm>

namespace {

constexpr const char *kLeaderboardFile = "leaderboard.dat";
constexpr const char *kLeaderboardFileV2 = "leaderboard_v2.dat";

void SetEntryName(LeaderboardEntry &e, const char *n) {
  std::strncpy(e.name, n, sizeof(e.name) - 1);
  e.name[sizeof(e.name) - 1] = '\0';
}

// Get leaderboard index for a level (0 = Endless Mode, 1-30 = regular levels)
int GetLeaderboardIndex(const Game& game) {
  if (game.isEndlessMode) {
    return 0;
  }
  return game.currentLevelIndex;
}

// Get the leaderboard array for a given index (non-const)
std::array<LeaderboardEntry, cfg::kLeaderboardSize>& GetLeaderboardArray(Game& game, int index) {
  return game.leaderboards[index];
}

// Get the leaderboard array for a given index (const)
const std::array<LeaderboardEntry, cfg::kLeaderboardSize>& GetLeaderboardArrayConst(const Game& game, int index) {
  auto it = game.leaderboards.find(index);
  if (it != game.leaderboards.end()) {
    return it->second;
  }
  static const std::array<LeaderboardEntry, cfg::kLeaderboardSize> empty{};
  return empty;
}

int GetLeaderboardCount(const Game& game, int index) {
  auto it = game.leaderboardCounts.find(index);
  return (it != game.leaderboardCounts.end()) ? it->second : 0;
}

void SetLeaderboardCount(Game& game, int index, int count) {
  game.leaderboardCounts[index] = count;
}

} // namespace

float GetCurrentScore(const Game &game) {
  if (game.isEndlessMode) {
    // For Endless Mode, score is based on distance traveled from start
    return game.player.position.z - game.endlessStartZ;
  }
  return game.distanceScore + game.styleScore;
}

void SubmitScore(Game &game) {
  float currentScore = GetCurrentScore(game);
  if (currentScore <= 0.0f)
    return;

  int leaderboardIndex = GetLeaderboardIndex(game);
  auto& leaderboard = GetLeaderboardArray(game, leaderboardIndex);
  int leaderboardCount = GetLeaderboardCount(game, leaderboardIndex);

  // Check if it fits in top 10
  int insertIdx = -1;
  for (int i = 0; i < leaderboardCount; ++i) {
    if (currentScore > leaderboard[i].score) {
      insertIdx = i;
      break;
    }
  }

  if (insertIdx == -1 && leaderboardCount < cfg::kLeaderboardSize) {
    insertIdx = leaderboardCount;
  }

  if (insertIdx != -1) {
    game.hasPendingScore = true;
    game.pendingEntry.score = currentScore;
    game.pendingEntry.runTime = game.runTime;
    game.pendingEntry.seed = game.runSeed;
    game.pendingEntryIndex = insertIdx;
    game.pendingLeaderboardIndex = leaderboardIndex;
    std::memset(game.pendingEntry.name, 0, sizeof(game.pendingEntry.name));
    game.screen = GameScreen::NameEntry;
    std::memset(game.nameInputBuffer, 0, sizeof(game.nameInputBuffer));
    game.nameInputLength = 0;
  } else {
    CalculateLeaderboardStats(game);
  }
}

void FinalizeScoreEntry(Game &game) {
  if (!game.hasPendingScore || game.pendingEntryIndex == -1)
    return;

  int leaderboardIndex = game.pendingLeaderboardIndex;
  auto& leaderboard = GetLeaderboardArray(game, leaderboardIndex);
  int& leaderboardCount = game.leaderboardCounts[leaderboardIndex];

  if (game.nameInputLength == 0) {
    SetEntryName(game.pendingEntry, "Player");
  } else {
    SetEntryName(game.pendingEntry, game.nameInputBuffer);
  }

  // Shift existing entries
  if (leaderboardCount < cfg::kLeaderboardSize) {
    leaderboardCount++;
  }

  for (int i = leaderboardCount - 1; i > game.pendingEntryIndex; --i) {
    leaderboard[i] = leaderboard[i - 1];
  }

  leaderboard[game.pendingEntryIndex] = game.pendingEntry;
  SaveLeaderboard(game);

  game.hasPendingScore = false;
  game.pendingEntryIndex = -1;
  game.screen = GameScreen::GameOver;
}

void CalculateLeaderboardStats(Game &game) {
  float score = GetCurrentScore(game);
  game.leaderboardStats = {}; // Reset

  int leaderboardIndex = GetLeaderboardIndex(game);
  const auto& leaderboard = GetLeaderboardArrayConst(game, leaderboardIndex);
  int leaderboardCount = GetLeaderboardCount(game, leaderboardIndex);

  if (leaderboardCount >= cfg::kLeaderboardSize) {
    game.leaderboardStats.leaderboardFull = true;
    float tenthScore = leaderboard[cfg::kLeaderboardSize - 1].score;
    game.leaderboardStats.scoreDifference10th = tenthScore - score;
    game.leaderboardStats.scorePercent10th =
        (tenthScore > 0) ? (score / tenthScore * 100.0f) : 0.0f;
  }

  if (leaderboardCount > 0) {
    float firstScore = leaderboard[0].score;
    game.leaderboardStats.scoreDifference1st = firstScore - score;
    game.leaderboardStats.scorePercent1st =
        (firstScore > 0) ? (score / firstScore * 100.0f) : 0.0f;
  }

  // Find where it would rank
  game.leaderboardStats.rankIfQualified = leaderboardCount + 1;
  for (int i = 0; i < leaderboardCount; ++i) {
    if (score > leaderboard[i].score) {
      game.leaderboardStats.rankIfQualified = i + 1;
      game.leaderboardStats.scoreQualified = true;
      break;
    }
  }

  if (!game.leaderboardStats.scoreQualified &&
      leaderboardCount < cfg::kLeaderboardSize) {
    game.leaderboardStats.scoreQualified = true;
  }

  // Time difference estimates (assuming linear score-to-time ratio of 100
  // pts/s)
  if (game.leaderboardStats.leaderboardFull) {
    game.leaderboardStats.timeDifference10th =
        game.leaderboardStats.scoreDifference10th / 100.0f;
  }
}

void SaveLeaderboard(const Game &game) {
  // Save new format (v2) with multiple leaderboards
  FILE *f = std::fopen(kLeaderboardFileV2, "wb");
  if (!f)
    return;
  
  // Write version marker
  int version = 2;
  std::fwrite(&version, sizeof(int), 1, f);
  
  // Write number of leaderboards
  int leaderboardCount = (int)game.leaderboards.size();
  std::fwrite(&leaderboardCount, sizeof(int), 1, f);
  
  // Write each leaderboard
  for (const auto& [index, entries] : game.leaderboards) {
    int count = GetLeaderboardCount(game, index);
    std::fwrite(&index, sizeof(int), 1, f);
    std::fwrite(&count, sizeof(int), 1, f);
    if (count > 0) {
      std::fwrite(entries.data(), sizeof(LeaderboardEntry), count, f);
    }
  }
  std::fclose(f);
  
  // Also save legacy format for backward compatibility (only first leaderboard)
  FILE *legacyF = std::fopen(kLeaderboardFile, "wb");
  if (legacyF) {
    // Try to save the first available leaderboard (prefer Endless Mode, then level 1)
    int legacyCount = 0;
    if (game.leaderboards.find(0) != game.leaderboards.end()) {
      legacyCount = GetLeaderboardCount(game, 0);
      std::fwrite(&legacyCount, sizeof(int), 1, legacyF);
      if (legacyCount > 0) {
        std::fwrite(game.leaderboards.at(0).data(), sizeof(LeaderboardEntry), legacyCount, legacyF);
      }
    } else if (game.leaderboards.find(1) != game.leaderboards.end()) {
      legacyCount = GetLeaderboardCount(game, 1);
      std::fwrite(&legacyCount, sizeof(int), 1, legacyF);
      if (legacyCount > 0) {
        std::fwrite(game.leaderboards.at(1).data(), sizeof(LeaderboardEntry), legacyCount, legacyF);
      }
    } else {
      std::fwrite(&legacyCount, sizeof(int), 1, legacyF);
    }
    std::fclose(legacyF);
  }
}

void LoadLeaderboard(Game &game) {
  // Try to load new format first
  FILE *f = std::fopen(kLeaderboardFileV2, "rb");
  if (f) {
    int version = 0;
    if (std::fread(&version, sizeof(int), 1, f) == 1 && version == 2) {
      int leaderboardCount = 0;
      if (std::fread(&leaderboardCount, sizeof(int), 1, f) == 1) {
        for (int i = 0; i < leaderboardCount; ++i) {
          int index = 0;
          int count = 0;
          if (std::fread(&index, sizeof(int), 1, f) == 1 &&
              std::fread(&count, sizeof(int), 1, f) == 1) {
            if (count > cfg::kLeaderboardSize)
              count = cfg::kLeaderboardSize;
            if (count > 0) {
              auto& leaderboard = GetLeaderboardArray(game, index);
              if (std::fread(leaderboard.data(), sizeof(LeaderboardEntry), count, f) == (size_t)count) {
                SetLeaderboardCount(game, index, count);
              }
            }
          }
        }
      }
    }
    std::fclose(f);
    
    // If we loaded at least one leaderboard, we're done
    if (!game.leaderboards.empty()) {
      return;
    }
  }
  
  // Fall back to legacy format
  f = std::fopen(kLeaderboardFile, "rb");
  if (!f) {
    SeedDefaultLeaderboard(game);
    return;
  }
  
  int legacyCount = 0;
  if (std::fread(&legacyCount, sizeof(int), 1, f) == 1) {
    if (legacyCount > cfg::kLeaderboardSize)
      legacyCount = cfg::kLeaderboardSize;
    if (legacyCount > 0) {
      auto& leaderboard = GetLeaderboardArray(game, 1);  // Load into level 1 leaderboard
      if (std::fread(leaderboard.data(), sizeof(LeaderboardEntry), legacyCount, f) == (size_t)legacyCount) {
        SetLeaderboardCount(game, 1, legacyCount);
        // Also populate legacy fields for backward compatibility
        game.leaderboard = leaderboard;
        game.leaderboardCount = legacyCount;
      }
    }
  }
  std::fclose(f);
}

void SeedDefaultLeaderboard(Game &game) {
  // Seed default leaderboard for level 1
  auto& leaderboard = GetLeaderboardArray(game, 1);
  SetLeaderboardCount(game, 1, 5);
  
  SetEntryName(leaderboard[0], "Antigravity");
  leaderboard[0].score = 5000.0f;
  leaderboard[0].runTime = 45.0f;

  SetEntryName(leaderboard[1], "VoidRunner");
  leaderboard[1].score = 3500.0f;
  leaderboard[1].runTime = 38.0f;

  SetEntryName(leaderboard[2], "StarDust");
  leaderboard[2].score = 2200.0f;
  leaderboard[2].runTime = 32.0f;

  SetEntryName(leaderboard[3], "NeonByte");
  leaderboard[3].score = 1500.0f;
  leaderboard[3].runTime = 25.0f;

  SetEntryName(leaderboard[4], "Novice");
  leaderboard[4].score = 500.0f;
  leaderboard[4].runTime = 12.0f;
  
  // Also populate legacy fields
  game.leaderboard = leaderboard;
  game.leaderboardCount = 5;
}
