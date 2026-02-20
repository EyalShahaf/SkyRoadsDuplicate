// sim_runner — headless level validator
//
// Runs the simulation without a window using a deterministic bot.
// Outputs structured metrics for level validation and regression testing.
//
// Usage:
//   sim_runner [options]
//     --seed <hex|dec>    Run seed (default: 0xC0FFEE)
//     --ticks <n>         Max sim ticks to run (default: 36000 = 5 min at 120Hz)
//     --bot <style>       Bot style: cautious|aggressive|random (default: cautious)
//     --json              Output as JSON instead of plain text
//     --quiet             Only output final summary line
//     -h, --help          Print usage

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

#include "core/Config.hpp"
#include "core/Rng.hpp"
#include "game/Game.hpp"
#include "sim/Bot.hpp"
#include "sim/Sim.hpp"

namespace {

struct RunnerArgs {
    uint32_t seed = 0xC0FFEEu;
    int maxTicks = 36000;           // 5 minutes at 120 Hz
    BotStyle botStyle = BotStyle::Cautious;
    bool json = false;
    bool quiet = false;
    bool help = false;
};

uint32_t ParseSeed(const char* str) {
    // Accept 0x prefix for hex, otherwise decimal.
    return static_cast<uint32_t>(std::strtoul(str, nullptr, 0));
}

BotStyle ParseBotStyle(const char* str) {
    if (std::strcmp(str, "aggressive") == 0) return BotStyle::Aggressive;
    if (std::strcmp(str, "random") == 0) return BotStyle::Random;
    return BotStyle::Cautious;
}

RunnerArgs ParseArgs(int argc, char* argv[]) {
    RunnerArgs args{};
    for (int i = 1; i < argc; ++i) {
        if ((std::strcmp(argv[i], "--seed") == 0) && i + 1 < argc) {
            args.seed = ParseSeed(argv[++i]);
        } else if ((std::strcmp(argv[i], "--ticks") == 0) && i + 1 < argc) {
            args.maxTicks = std::atoi(argv[++i]);
        } else if ((std::strcmp(argv[i], "--bot") == 0) && i + 1 < argc) {
            args.botStyle = ParseBotStyle(argv[++i]);
        } else if (std::strcmp(argv[i], "--json") == 0) {
            args.json = true;
        } else if (std::strcmp(argv[i], "--quiet") == 0) {
            args.quiet = true;
        } else if (std::strcmp(argv[i], "-h") == 0 || std::strcmp(argv[i], "--help") == 0) {
            args.help = true;
        }
    }
    return args;
}

void PrintUsage() {
    std::printf(
        "sim_runner — headless SkyRoads level validator\n"
        "\n"
        "Usage: sim_runner [options]\n"
        "  --seed <hex|dec>   Run seed (default: 0xC0FFEE)\n"
        "  --ticks <n>        Max sim ticks (default: 36000 = 5 min)\n"
        "  --bot <style>      cautious|aggressive|random (default: cautious)\n"
        "  --json             Output as JSON\n"
        "  --quiet            Only final summary line\n"
        "  -h, --help         This message\n"
    );
}

const char* BotStyleName(BotStyle s) {
    switch (s) {
        case BotStyle::Cautious:   return "cautious";
        case BotStyle::Aggressive: return "aggressive";
        case BotStyle::Random:     return "random";
    }
    return "unknown";
}

}  // namespace

int main(int argc, char* argv[]) {
    const RunnerArgs args = ParseArgs(argc, argv);
    if (args.help) {
        PrintUsage();
        return 0;
    }

    // --- Init game state (no window, no raylib graphics) ---
    Game game{};
    // Manual init without raylib calls (InitGame calls LoadLeaderboard which uses fopen — fine).
    game.rngState = (args.seed == 0u) ? 1u : args.seed;
    game.bestScore = 0.0f;
    game.paletteIndex = 0;
    game.bloomEnabled = false;
    game.simTicks = 0;
    game.screen = GameScreen::Playing;
    game.leaderboardCount = 0;
    ResetRun(game, game.rngState);

    Bot bot{};
    InitBot(bot, args.botStyle, args.seed ^ 0x12345678u);

    // --- Run headless sim ---
    using Clock = std::chrono::steady_clock;
    const auto wallStart = Clock::now();

    int ticksRun = 0;
    float deathX = 0.0f, deathY = 0.0f, deathZ = 0.0f;

    for (int t = 0; t < args.maxTicks; ++t) {
        BotInput(bot, game);
        game.previousPlayer = game.player;
        SimStep(game, cfg::kFixedDt);
        ++game.simTicks;
        ++ticksRun;

        if (!game.runActive) {
            deathX = game.player.position.x;
            deathY = game.player.position.y;
            deathZ = game.player.position.z;
            break;
        }
    }

    const char* deathCause = "none";
    switch (game.deathCause) {
        case 1: deathCause = "fell"; break;
        case 2: deathCause = "obstacle"; break;
        case 3: deathCause = "level_complete"; break;
        default: deathCause = game.runActive ? "none" : "unknown"; break;
    }

    const auto wallEnd = Clock::now();
    const float wallMs = std::chrono::duration<float, std::milli>(wallEnd - wallStart).count();

    // --- Compute metrics ---
    const float simTime = game.runTime;
    const float distance = game.player.position.z - cfg::kPlatformStartZ;
    const float score = GetCurrentScore(game);
    const float difficulty = game.difficultyT;
    const bool survived = game.runActive || game.levelComplete;
    const float perfMsPer1k = (ticksRun > 0) ? (wallMs / (static_cast<float>(ticksRun) / 1000.0f)) : 0.0f;

    // --- Output ---
    if (args.json) {
        std::printf("{\n");
        std::printf("  \"seed\": \"0x%08X\",\n", args.seed);
        std::printf("  \"bot\": \"%s\",\n", BotStyleName(args.botStyle));
        std::printf("  \"ticks_run\": %d,\n", ticksRun);
        std::printf("  \"ticks_max\": %d,\n", args.maxTicks);
        std::printf("  \"sim_time\": %.2f,\n", simTime);
        std::printf("  \"distance\": %.1f,\n", distance);
        std::printf("  \"score\": %.1f,\n", score);
        std::printf("  \"difficulty\": %.3f,\n", difficulty);
        std::printf("  \"multiplier\": %.2f,\n", game.scoreMultiplier);
        std::printf("  \"status\": \"%s\",\n", survived ? "SURVIVED" : "DIED");
        std::printf("  \"death_cause\": \"%s\",\n", deathCause);
        std::printf("  \"death_pos\": [%.2f, %.2f, %.2f],\n", deathX, deathY, deathZ);
        std::printf("  \"wall_ms\": %.2f,\n", wallMs);
        std::printf("  \"perf_ms_per_1k\": %.3f\n", perfMsPer1k);
        std::printf("}\n");
    } else if (args.quiet) {
        std::printf("seed=0x%08X  status=%-8s  score=%-10.0f  dist=%-8.1f  time=%-7.2fs  diff=%.3f  perf=%.3fms/1k\n",
                    args.seed,
                    survived ? "SURVIVED" : "DIED",
                    score, distance, simTime, difficulty, perfMsPer1k);
    } else {
        std::printf("=== SkyRoads Headless Sim Runner ===\n");
        std::printf("seed:       0x%08X\n", args.seed);
        std::printf("bot:        %s\n", BotStyleName(args.botStyle));
        std::printf("ticks:      %d / %d\n", ticksRun, args.maxTicks);
        std::printf("sim_time:   %.2f s\n", simTime);
        std::printf("distance:   %.1f units\n", distance);
        std::printf("score:      %.1f\n", score);
        std::printf("difficulty: %.3f / %.1f\n", difficulty, cfg::kDifficultyMaxCap);
        std::printf("multiplier: %.2f\n", game.scoreMultiplier);
        std::printf("status:     %s\n", survived ? "SURVIVED" : "DIED");
        if (!survived) {
            std::printf("death:      %s at (%.2f, %.2f, %.2f)\n", deathCause, deathX, deathY, deathZ);
        }
        std::printf("wall_time:  %.2f ms\n", wallMs);
        std::printf("perf:       %.3f ms / 1000 ticks\n", perfMsPer1k);
    }

    return survived ? 0 : 1;
}
