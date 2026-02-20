#pragma once

#include <cstdint>

struct Game;

// Bot behavior presets.
enum class BotStyle {
    Cautious,    // stays centered, jumps conservatively
    Aggressive,  // strafes wide, dashes often, jumps frequently
    Random,      // seeded random inputs for stress testing
};

// Deterministic bot that generates input for one sim tick.
// No heap allocations, no raylib dependency.
// Uses its own RNG state so it doesn't pollute game.rngState.
struct Bot {
    BotStyle style = BotStyle::Cautious;
    uint32_t rng = 1u;
    int ticksSinceJump = 0;
    int ticksSinceDash = 0;
};

void InitBot(Bot& bot, BotStyle style, uint32_t seed);

// Writes deterministic input into game.input for the current tick.
void BotInput(Bot& bot, Game& game);
