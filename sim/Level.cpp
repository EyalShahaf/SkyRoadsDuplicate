#include "sim/Level.hpp"

int GetStageFromLevelIndex(int levelIndex) {
  if (levelIndex < 1 || levelIndex > 30)
    return 1;
  return ((levelIndex - 1) / 3) + 1;
}

int GetLevelInStageFromLevelIndex(int levelIndex) {
  if (levelIndex < 1 || levelIndex > 30)
    return 1;
  return ((levelIndex - 1) % 3) + 1;
}

int GetLevelIndexFromStageAndLevel(int stage, int levelInStage) {
  if (stage < 1 || stage > 10)
    return 1;
  if (levelInStage < 1 || levelInStage > 3)
    return 1;
  return (stage - 1) * 3 + levelInStage;
}

bool IsLevelImplemented(int levelIndex) {
  return levelIndex >= 1 && levelIndex <= 6;
}
