#include <chrono>
#include <cstdio>
#include <ctime>

#include <raylib.h>

#include "core/Config.hpp"
#include "core/CrashHandler.hpp"
#include "core/Log.hpp"
#include "core/PerfTracker.hpp"
#include "game/Game.hpp"
#include "render/Render.hpp"
#include "sim/Sim.hpp"

int main() {
  Log::Init();
  CrashHandler::Init();
  LOG_INFO("SkyRoads starting...");

  SetConfigFlags(FLAG_MSAA_4X_HINT);
  InitWindow(cfg::kScreenWidth, cfg::kScreenHeight, "SkyRoads Runner");
  SetExitKey(
      0); // Disable raylib's default ESC=quit so we handle ESC ourselves.
  SetTargetFPS(0);

  Game game{};
  InitGame(game, 0xC0FFEEu);
  InitRenderer();

  using Clock = std::chrono::steady_clock;

  while (!WindowShouldClose() && !game.wantsExit) {
    ReadInput(game);
    ApplyMetaActions(game);

    float frameTime = GetFrameTime();
    if (frameTime > cfg::kMaxFrameTime) {
      frameTime = cfg::kMaxFrameTime;
    }

    // --- Measure Update (sim steps) --- only when Playing ---
    perf::ResetAllocCounter();
    const auto updateStart = Clock::now();

    if (game.screen == GameScreen::Playing) {
      game.accumulator += frameTime;
      int simSteps = 0;
      constexpr int kMaxSimStepsPerFrame = 8;

      while (game.accumulator >= cfg::kFixedDt &&
             simSteps < kMaxSimStepsPerFrame) {
        game.previousPlayer = game.player;
        if (game.runActive) {
          SimStep(game, cfg::kFixedDt);
        }
        game.accumulator -= cfg::kFixedDt;
        ++game.simTicks;
        ++simSteps;
      }

      if (simSteps == kMaxSimStepsPerFrame) {
        game.accumulator = 0.0f;
      }
    }

    const auto updateEnd = Clock::now();
    game.updateMs =
        std::chrono::duration<float, std::milli>(updateEnd - updateStart)
            .count();
    game.updateAllocCount = perf::ReadAllocCounter();

#ifndef NDEBUG
    if (game.updateMs > 2.0f) {
      LOG_WARN("Update() took {:.3f} ms (> 2ms budget)", game.updateMs);
    }
    if (game.updateAllocCount > 0) {
      LOG_WARN("{} heap allocation(s) during Update()", game.updateAllocCount);
    }
#endif

    // --- Measure Render ---
    const float alpha = (game.screen == GameScreen::Playing)
                            ? game.accumulator / cfg::kFixedDt
                            : 0.0f;
    const auto renderStart = Clock::now();
    RenderFrame(game, alpha, frameTime);
    const auto renderEnd = Clock::now();
    game.renderMs =
        std::chrono::duration<float, std::milli>(renderEnd - renderStart)
            .count();

    // --- Take screenshot if requested (after rendering) ---
    if (game.screenshotRequested) {
      std::time_t now = std::time(nullptr);
      std::tm *tm = std::localtime(&now);
      char filename[256];
      std::snprintf(filename, sizeof(filename),
                    "screenshot_%04d%02d%02d_%02d%02d%02d.png",
                    tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                    tm->tm_hour, tm->tm_min, tm->tm_sec);
      TakeScreenshot(filename);
      std::snprintf(game.screenshotPath, sizeof(game.screenshotPath), "%s",
                    filename);
      game.screenshotNotificationTimer =
          3.0f; // Show notification for 3 seconds
      game.screenshotRequested = false;
    }
  }

  LOG_INFO("SkyRoads shutting down...");
  CleanupRenderer();
  CloseWindow();
  Log::Shutdown();
  return 0;
}
