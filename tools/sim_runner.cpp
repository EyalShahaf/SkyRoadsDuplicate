// sim_runner — headless level validator with optional screenshot support
//
// Runs the simulation without a window using a deterministic bot.
// Outputs structured metrics for level validation and regression testing.
// Can optionally take screenshots during gameplay for documentation and validation.
//
// Usage:
//   sim_runner [options]
//     --seed <hex|dec>              Run seed (default: 0xC0FFEE)
//     --ticks <n>                   Max sim ticks to run (default: 36000 = 5 min at 120Hz)
//     --bot <style>                 Bot style: cautious|aggressive|random (default: cautious)
//     --level <n>                   Level index (1-30, default: 1)
//     --palette <n>                 Palette index (0-2, default: 0)
//     --bloom                       Enable bloom effect (default: off)
//     --screenshots                  Enable screenshot capture
//     --screenshot-output <dir>      Output directory (default: docs/screenshots-raw)
//     --screenshot-interval <n>     Take screenshot every N ticks (0 = disabled)
//     --screenshot-at-ticks <list>   Comma-separated list of ticks to screenshot
//     --screenshot-at-distance <list> Comma-separated list of distances to screenshot
//     --json                        Output as JSON instead of plain text
//     --quiet                       Only output final summary line
//     -h, --help                    Print usage

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>
#include <fstream>
#include <ctime>

#ifdef _WIN32
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include "core/Config.hpp"
#include "core/Rng.hpp"
#include "game/Game.hpp"
#include "sim/Bot.hpp"
#include "sim/Sim.hpp"
#include <raylib.h>
#include "render/Render.hpp"

namespace {

struct RunnerArgs {
    uint32_t seed = 0xC0FFEEu;
    int maxTicks = 36000;           // 5 minutes at 120 Hz
    BotStyle botStyle = BotStyle::Cautious;
    int levelIndex = 1;             // Level index (1-30)
    int paletteIndex = 0;           // Palette index (0-2)
    bool bloomEnabled = false;      // Bloom effect
    bool enableScreenshots = false; // Enable screenshot capture
    std::string screenshotOutputDir = "docs/screenshots-raw";
    int screenshotInterval = 0;     // Take screenshot every N ticks (0 = disabled)
    std::vector<int> screenshotAtTicks;      // Specific ticks to screenshot
    std::vector<float> screenshotAtDistance; // Specific distances to screenshot
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

std::vector<int> ParseIntList(const char* str) {
    std::vector<int> result;
    char* copy = new char[std::strlen(str) + 1];
    std::strcpy(copy, str);
    char* token = std::strtok(copy, ",");
    while (token != nullptr) {
        result.push_back(std::atoi(token));
        token = std::strtok(nullptr, ",");
    }
    delete[] copy;
    return result;
}

std::vector<float> ParseFloatList(const char* str) {
    std::vector<float> result;
    char* copy = new char[std::strlen(str) + 1];
    std::strcpy(copy, str);
    char* token = std::strtok(copy, ",");
    while (token != nullptr) {
        result.push_back(static_cast<float>(std::atof(token)));
        token = std::strtok(nullptr, ",");
    }
    delete[] copy;
    return result;
}

void CreateDirectoryRecursive(const std::string& path) {
    if (path.empty()) return;
    
    size_t pos = 0;
    std::string dir;
    
    // Handle Windows drive letters (C:, D:, etc.)
    if (path.length() >= 2 && path[1] == ':') {
        pos = 2;
    }
    
    while ((pos = path.find_first_of("/\\", pos + 1)) != std::string::npos) {
        dir = path.substr(0, pos);
        if (!dir.empty()) {
            mkdir(dir.c_str(), 0755);
            // Ignore errors - directory might already exist
        }
    }
    
    // Create final directory
    if (!path.empty()) {
        mkdir(path.c_str(), 0755);
        // Ignore errors - directory might already exist
    }
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
        } else if ((std::strcmp(argv[i], "--level") == 0) && i + 1 < argc) {
            args.levelIndex = std::atoi(argv[++i]);
            if (args.levelIndex < 1) args.levelIndex = 1;
            if (args.levelIndex > 30) args.levelIndex = 30;
        } else if ((std::strcmp(argv[i], "--palette") == 0) && i + 1 < argc) {
            args.paletteIndex = std::atoi(argv[++i]);
            if (args.paletteIndex < 0) args.paletteIndex = 0;
            if (args.paletteIndex >= cfg::kPaletteCount) args.paletteIndex = cfg::kPaletteCount - 1;
        } else if (std::strcmp(argv[i], "--bloom") == 0) {
            args.bloomEnabled = true;
        } else if (std::strcmp(argv[i], "--screenshots") == 0) {
            args.enableScreenshots = true;
        } else if ((std::strcmp(argv[i], "--screenshot-output") == 0) && i + 1 < argc) {
            args.screenshotOutputDir = argv[++i];
        } else if ((std::strcmp(argv[i], "--screenshot-interval") == 0) && i + 1 < argc) {
            args.screenshotInterval = std::atoi(argv[++i]);
        } else if ((std::strcmp(argv[i], "--screenshot-at-ticks") == 0) && i + 1 < argc) {
            args.screenshotAtTicks = ParseIntList(argv[++i]);
        } else if ((std::strcmp(argv[i], "--screenshot-at-distance") == 0) && i + 1 < argc) {
            args.screenshotAtDistance = ParseFloatList(argv[++i]);
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
        "sim_runner — headless SkyRoads level validator with screenshot support\n"
        "\n"
        "Usage: sim_runner [options]\n"
        "  --seed <hex|dec>              Run seed (default: 0xC0FFEE)\n"
        "  --ticks <n>                   Max sim ticks (default: 36000 = 5 min)\n"
        "  --bot <style>                 cautious|aggressive|random (default: cautious)\n"
        "  --level <n>                   Level index 1-30 (default: 1)\n"
        "  --palette <n>                 Palette index 0-2 (default: 0)\n"
        "  --bloom                       Enable bloom effect\n"
        "  --screenshots                  Enable screenshot capture\n"
        "  --screenshot-output <dir>      Output directory (default: docs/screenshots-raw)\n"
        "  --screenshot-interval <n>     Take screenshot every N ticks (0 = disabled)\n"
        "  --screenshot-at-ticks <list>   Comma-separated ticks to screenshot (e.g., 1200,6000)\n"
        "  --screenshot-at-distance <list> Comma-separated distances to screenshot (e.g., 100,200)\n"
        "  --json                        Output as JSON\n"
        "  --quiet                       Only final summary line\n"
        "  -h, --help                    This message\n"
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

std::string GenerateScreenshotFilename(const RunnerArgs& args, int tick, float distance, const Game& /*game*/) {
    char filename[512];
    std::snprintf(filename, sizeof(filename), "%s/level_%d_palette_%d_tick_%d_dist_%.0f_seed_0x%08X.png",
                  args.screenshotOutputDir.c_str(),
                  args.levelIndex,
                  args.paletteIndex,
                  tick,
                  distance,
                  args.seed);
    return std::string(filename);
}

void SaveScreenshotMetadata(const std::string& imagePath, const RunnerArgs& args, int tick, const Game& game) {
    std::string jsonPath = imagePath;
    size_t dotPos = jsonPath.find_last_of('.');
    if (dotPos != std::string::npos) {
        jsonPath = jsonPath.substr(0, dotPos) + ".json";
    } else {
        jsonPath += ".json";
    }

    std::ofstream json(jsonPath);
    if (!json.is_open()) return;

    const float distance = game.player.position.z - cfg::kPlatformStartZ;
    const float score = GetCurrentScore(game);
    
    std::time_t now = std::time(nullptr);
    char timeStr[64];
    std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&now));

    json << "{\n";
    json << "  \"seed\": \"0x" << std::hex << std::uppercase << args.seed << std::dec << "\",\n";
    json << "  \"level\": " << args.levelIndex << ",\n";
    json << "  \"palette\": " << args.paletteIndex << ",\n";
    json << "  \"bloom\": " << (args.bloomEnabled ? "true" : "false") << ",\n";
    json << "  \"tick\": " << tick << ",\n";
    json << "  \"distance\": " << distance << ",\n";
    json << "  \"score\": " << score << ",\n";
    json << "  \"difficulty\": " << game.difficultyT << ",\n";
    json << "  \"multiplier\": " << game.scoreMultiplier << ",\n";
    json << "  \"run_time\": " << game.runTime << ",\n";
    json << "  \"timestamp\": \"" << timeStr << "\"\n";
    json << "}\n";
    json.close();
}

bool ShouldTakeScreenshot(const RunnerArgs& args, int tick, float distance, const Game& /*game*/) {
    // Check interval
    if (args.screenshotInterval > 0 && (tick % args.screenshotInterval == 0)) {
        return true;
    }
    
    // Check specific ticks
    for (int targetTick : args.screenshotAtTicks) {
        if (tick == targetTick) {
            return true;
        }
    }
    
    // Check specific distances (within 5 units tolerance)
    for (float targetDist : args.screenshotAtDistance) {
        if (std::abs(distance - targetDist) < 5.0f) {
            return true;
        }
    }
    
    return false;
}

int main(int argc, char* argv[]) {
    const RunnerArgs args = ParseArgs(argc, argv);
    if (args.help) {
        PrintUsage();
        return 0;
    }

    // --- Initialize raylib and renderer if screenshots are enabled ---
    if (args.enableScreenshots) {
        SetConfigFlags(FLAG_WINDOW_HIDDEN | FLAG_MSAA_4X_HINT);
        SetExitKey(0);  // Disable ESC=quit
        InitWindow(cfg::kScreenWidth, cfg::kScreenHeight, "SkyRoads Screenshot");
        SetTargetFPS(0);  // Disable frame limiting
        InitRenderer();
        
        // Create output directory
        CreateDirectoryRecursive(args.screenshotOutputDir);
    }

    // --- Init game state ---
    Game game{};
    game.rngState = (args.seed == 0u) ? 1u : args.seed;
    game.bestScore = 0.0f;
    game.paletteIndex = args.paletteIndex;
    game.bloomEnabled = args.bloomEnabled;
    game.simTicks = 0;
    game.screen = GameScreen::Playing;
    game.leaderboardCount = 0;
    ResetRun(game, game.rngState, args.levelIndex);

    Bot bot{};
    InitBot(bot, args.botStyle, args.seed ^ 0x12345678u);

    // --- Run sim (with optional rendering for screenshots) ---
    using Clock = std::chrono::steady_clock;
    const auto wallStart = Clock::now();

    int ticksRun = 0;
    float deathX = 0.0f, deathY = 0.0f, deathZ = 0.0f;
    int lastScreenshotTick = -1;
    float lastScreenshotDistance = -1.0f;

    for (int t = 0; t < args.maxTicks; ++t) {
        BotInput(bot, game);
        game.previousPlayer = game.player;
        SimStep(game, cfg::kFixedDt);
        ++game.simTicks;
        ++ticksRun;

        // --- Take screenshots if enabled ---
        if (args.enableScreenshots) {
            const float distance = game.player.position.z - cfg::kPlatformStartZ;
            
            // Check if we should take a screenshot
            if (ShouldTakeScreenshot(args, ticksRun, distance, game)) {
                // Avoid duplicate screenshots at the same tick/distance
                if (ticksRun != lastScreenshotTick || std::abs(distance - lastScreenshotDistance) > 1.0f) {
                    // Process window events to prevent hanging (even for hidden windows)
                    PollInputEvents();
                    
                    // Update camera and render a frame
                    game.accumulator = 0.0f; // No interpolation for screenshots
                    const float alpha = 0.0f;
                    const float renderDt = cfg::kFixedDt;
                    
                    RenderFrame(game, alpha, renderDt);
                    
                    // Take screenshot
                    std::string filename = GenerateScreenshotFilename(args, ticksRun, distance, game);
                    TakeScreenshot(filename.c_str());
                    
                    // Save metadata
                    SaveScreenshotMetadata(filename, args, ticksRun, game);
                    
                    if (!args.quiet) {
                        std::fprintf(stderr, "[Screenshot] %s\n", filename.c_str());
                    }
                    
                    lastScreenshotTick = ticksRun;
                    lastScreenshotDistance = distance;
                }
            }
        }

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

    // --- Cleanup renderer and window if screenshots were enabled ---
    if (args.enableScreenshots) {
        CleanupRenderer();
        CloseWindow();
    }

    return survived ? 0 : 1;
}
