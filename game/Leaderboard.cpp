#include "game/Game.hpp"

#include "core/Config.hpp"
#include <cstdio>
#include <cstring>
#include <ctime>

namespace {

constexpr const char *kLeaderboardFile = "leaderboard.dat";

void SetEntryName(LeaderboardEntry &e, const char *n) {
  std::strncpy(e.name, n, sizeof(e.name) - 1);
  e.name[sizeof(e.name) - 1] = '\0';
}

} // namespace

float GetCurrentScore(const Game &game) {
  return game.distanceScore + game.styleScore;
}

void SubmitScore(Game &game) {
  float currentScore = GetCurrentScore(game);
  if (currentScore <= 0.0f)
    return;

  // Check if it fits in top 10
  int insertIdx = -1;
  for (int i = 0; i < game.leaderboardCount; ++i) {
    if (currentScore > game.leaderboard[i].score) {
      insertIdx = i;
      break;
    }
  }

  if (insertIdx == -1 && game.leaderboardCount < cfg::kLeaderboardSize) {
    insertIdx = game.leaderboardCount;
  }

  if (insertIdx != -1) {
    game.hasPendingScore = true;
    game.pendingEntry.score = currentScore;
    game.pendingEntry.runTime = game.runTime;
    game.pendingEntry.seed = game.runSeed;
    game.pendingEntryIndex = insertIdx;
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

  if (game.nameInputLength == 0) {
    SetEntryName(game.pendingEntry, "Player");
  } else {
    SetEntryName(game.pendingEntry, game.nameInputBuffer);
  }

  // Shift existing entries
  if (game.leaderboardCount < cfg::kLeaderboardSize) {
    game.leaderboardCount++;
  }

  for (int i = game.leaderboardCount - 1; i > game.pendingEntryIndex; --i) {
    game.leaderboard[i] = game.leaderboard[i - 1];
  }

  game.leaderboard[game.pendingEntryIndex] = game.pendingEntry;
  SaveLeaderboard(game);

  game.hasPendingScore = false;
  game.pendingEntryIndex = -1;
  game.screen = GameScreen::GameOver;
}

void CalculateLeaderboardStats(Game &game) {
  float score = GetCurrentScore(game);
  game.leaderboardStats = {}; // Reset

  if (game.leaderboardCount >= cfg::kLeaderboardSize) {
    game.leaderboardStats.leaderboardFull = true;
    float tenthScore = game.leaderboard[cfg::kLeaderboardSize - 1].score;
    game.leaderboardStats.scoreDifference10th = tenthScore - score;
    game.leaderboardStats.scorePercent10th =
        (tenthScore > 0) ? (score / tenthScore * 100.0f) : 0.0f;
  }

  if (game.leaderboardCount > 0) {
    float firstScore = game.leaderboard[0].score;
    game.leaderboardStats.scoreDifference1st = firstScore - score;
    game.leaderboardStats.scorePercent1st =
        (firstScore > 0) ? (score / firstScore * 100.0f) : 0.0f;
  }

  // Find where it would rank
  game.leaderboardStats.rankIfQualified = game.leaderboardCount + 1;
  for (int i = 0; i < game.leaderboardCount; ++i) {
    if (score > game.leaderboard[i].score) {
      game.leaderboardStats.rankIfQualified = i + 1;
      game.leaderboardStats.scoreQualified = true;
      break;
    }
  }

  if (!game.leaderboardStats.scoreQualified &&
      game.leaderboardCount < cfg::kLeaderboardSize) {
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
  FILE *f = std::fopen(kLeaderboardFile, "wb");
  if (!f)
    return;
  std::fwrite(&game.leaderboardCount, sizeof(int), 1, f);
  if (game.leaderboardCount > 0) {
    std::fwrite(game.leaderboard.data(), sizeof(LeaderboardEntry),
                game.leaderboardCount, f);
  }
  std::fclose(f);
}

void LoadLeaderboard(Game &game) {
  FILE *f = std::fopen(kLeaderboardFile, "rb");
  if (!f) {
    SeedDefaultLeaderboard(game);
    return;
  }
  if (std::fread(&game.leaderboardCount, sizeof(int), 1, f) != 1) {
    game.leaderboardCount = 0;
  } else {
    if (game.leaderboardCount > cfg::kLeaderboardSize)
      game.leaderboardCount = cfg::kLeaderboardSize;
    if (game.leaderboardCount > 0) {
      if (std::fread(game.leaderboard.data(), sizeof(LeaderboardEntry),
                     game.leaderboardCount,
                     f) != (size_t)game.leaderboardCount) {
        game.leaderboardCount = 0;
      }
    }
  }
  std::fclose(f);
}

void SeedDefaultLeaderboard(Game &game) {
  game.leaderboardCount = 5;
  SetEntryName(game.leaderboard[0], "Antigravity");
  game.leaderboard[0].score = 5000.0f;
  game.leaderboard[0].runTime = 45.0f;

  SetEntryName(game.leaderboard[1], "VoidRunner");
  game.leaderboard[1].score = 3500.0f;
  game.leaderboard[1].runTime = 38.0f;

  SetEntryName(game.leaderboard[2], "StarDust");
  game.leaderboard[2].score = 2200.0f;
  game.leaderboard[2].runTime = 32.0f;

  SetEntryName(game.leaderboard[3], "NeonByte");
  game.leaderboard[3].score = 1500.0f;
  game.leaderboard[3].runTime = 25.0f;

  SetEntryName(game.leaderboard[4], "Novice");
  game.leaderboard[4].score = 500.0f;
  game.leaderboard[4].runTime = 12.0f;
}
