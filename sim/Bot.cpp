#include "sim/Bot.hpp"

#include <cmath>

#include "core/Config.hpp"
#include "core/Rng.hpp"
#include "game/Game.hpp"
#include "sim/Level.hpp"

void InitBot(Bot& bot, const BotStyle style, const uint32_t seed) {
    bot.style = style;
    bot.rng = (seed == 0u) ? 1u : seed;
    bot.ticksSinceJump = 0;
    bot.ticksSinceDash = 0;
}

// Returns true if there's a gap ahead within `lookAhead` distance.
static bool GapAhead(const Game& game, float lookAhead) {
    if (!game.level) return false;
    const float checkZ = game.player.position.z + lookAhead;
    const int seg = FindSegmentUnder(*game.level, checkZ, game.player.position.x, cfg::kPlayerWidth * 0.5f);
    return seg < 0;
}

// Returns true if there's an obstacle ahead within `lookAhead` distance at current X.
static bool ObstacleAhead(const Game& game, float lookAhead) {
    if (!game.level) return false;
    const Vector3 futurePos = Vector3{
        game.player.position.x,
        game.player.position.y,
        game.player.position.z + lookAhead
    };
    return CheckObstacleCollision(*game.level, futurePos,
                                  cfg::kPlayerWidth * 0.5f,
                                  cfg::kPlayerHalfHeight,
                                  cfg::kPlayerDepth * 0.5f);
}

// Returns a strafe direction to dodge an obstacle (-1 left, +1 right, 0 none).
static float ObstacleDodgeDirection(const Game& game, float lookAhead) {
    if (!game.level) return 0.0f;
    // Check if going left or right would clear the obstacle.
    const float checkZ = game.player.position.z + lookAhead;
    const float offset = 2.5f;
    const Vector3 leftPos = Vector3{game.player.position.x - offset, game.player.position.y, checkZ};
    const Vector3 rightPos = Vector3{game.player.position.x + offset, game.player.position.y, checkZ};
    const bool leftClear = !CheckObstacleCollision(*game.level, leftPos,
                                                    cfg::kPlayerWidth * 0.5f,
                                                    cfg::kPlayerHalfHeight,
                                                    cfg::kPlayerDepth * 0.5f);
    const bool rightClear = !CheckObstacleCollision(*game.level, rightPos,
                                                     cfg::kPlayerWidth * 0.5f,
                                                     cfg::kPlayerHalfHeight,
                                                     cfg::kPlayerDepth * 0.5f);
    if (leftClear && !rightClear) return -1.0f;
    if (rightClear && !leftClear) return 1.0f;
    if (leftClear && rightClear) {
        // Prefer the side closer to center.
        return (game.player.position.x > 0.0f) ? -1.0f : 1.0f;
    }
    return 0.0f;  // both blocked, just jump
}

// Returns the xOffset of the next segment ahead, for steering.
static float NextSegmentCenter(const Game& game, float lookAhead) {
    if (!game.level) return 0.0f;
    const float checkZ = game.player.position.z + lookAhead;
    for (int i = 0; i < game.level->segmentCount; ++i) {
        const auto& s = game.level->segments[i];
        if (checkZ >= s.startZ && checkZ <= s.startZ + s.length) {
            return s.xOffset;
        }
    }
    // Look for the next segment starting after checkZ.
    float bestZ = 99999.0f;
    float bestX = 0.0f;
    for (int i = 0; i < game.level->segmentCount; ++i) {
        const auto& s = game.level->segments[i];
        if (s.startZ > game.player.position.z && s.startZ < bestZ) {
            bestZ = s.startZ;
            bestX = s.xOffset;
        }
    }
    return bestX;
}

void BotInput(Bot& bot, Game& game) {
    ++bot.ticksSinceJump;
    ++bot.ticksSinceDash;

    game.input.moveX = 0.0f;
    game.input.jumpQueued = false;
    game.input.dashQueued = false;

    if (!game.runActive) return;

    const auto& player = game.player;

    // Shared: steer toward the next segment's center.
    const float targetX = NextSegmentCenter(game, 8.0f);
    const float xDiff = targetX - player.position.x;

    // Shared: detect approaching gap or obstacle.
    // At ~20 u/s, need ~0.5s reaction = 10 units lookahead.
    const bool gapNear = GapAhead(game, 6.0f);
    const bool gapSoon = GapAhead(game, 3.0f);
    const bool obsNear = ObstacleAhead(game, 12.0f);
    const bool obsClose = ObstacleAhead(game, 4.0f);

    switch (bot.style) {

    case BotStyle::Cautious: {
        // Dodge obstacles by strafing.
        if (obsNear) {
            const float dodge = ObstacleDodgeDirection(game, 10.0f);
            game.input.moveX = (dodge != 0.0f) ? dodge : ((player.position.x > 0.0f) ? -1.0f : 1.0f);
            // If very close and can't dodge, try jumping.
            if (obsClose && player.grounded) {
                game.input.jumpQueued = true;
                bot.ticksSinceJump = 0;
            }
        } else {
            if (xDiff > 0.3f) game.input.moveX = 0.7f;
            else if (xDiff < -0.3f) game.input.moveX = -0.7f;
        }

        // Jump over gaps.
        if (player.grounded && gapNear) {
            game.input.jumpQueued = true;
            bot.ticksSinceJump = 0;
        }

        // Periodic dash for score.
        if (player.grounded && !gapNear && !obsNear && bot.ticksSinceDash > 480) {
            game.input.dashQueued = true;
            bot.ticksSinceDash = 0;
        }
        break;
    }

    case BotStyle::Aggressive: {
        // Dodge or jump obstacles.
        if (obsNear) {
            const float dodge = ObstacleDodgeDirection(game, 10.0f);
            game.input.moveX = (dodge != 0.0f) ? dodge : ((player.position.x > 0.0f) ? -1.0f : 1.0f);
            if (obsClose && player.grounded) {
                game.input.jumpQueued = true;
                bot.ticksSinceJump = 0;
            }
        } else {
            if (xDiff > 0.2f) game.input.moveX = 1.0f;
            else if (xDiff < -0.2f) game.input.moveX = -1.0f;
        }

        // Jump at gaps or periodically.
        if (player.grounded && (gapNear || bot.ticksSinceJump > 120)) {
            game.input.jumpQueued = true;
            bot.ticksSinceJump = 0;
        }

        // Dash often.
        if (player.grounded && !gapSoon && !obsNear && bot.ticksSinceDash > 180) {
            game.input.dashQueued = true;
            bot.ticksSinceDash = 0;
        }
        break;
    }

    case BotStyle::Random: {
        const float r1 = core::NextFloat01(bot.rng);
        const float r2 = core::NextFloat01(bot.rng);
        const float r3 = core::NextFloat01(bot.rng);

        // Dodge obstacles first.
        if (obsNear) {
            const float dodge = ObstacleDodgeDirection(game, 3.0f);
            game.input.moveX = (dodge != 0.0f) ? dodge : ((r1 > 0.5f) ? 1.0f : -1.0f);
        } else {
            game.input.moveX = (r1 - 0.5f) * 1.5f;
            if (xDiff > 1.0f) game.input.moveX = 0.8f;
            else if (xDiff < -1.0f) game.input.moveX = -0.8f;
        }

        // Jump on gap, or randomly ~5%.
        if (player.grounded && (gapNear || r2 < 0.05f)) {
            game.input.jumpQueued = true;
            bot.ticksSinceJump = 0;
        }

        // Random dash ~3%.
        if (player.grounded && r3 < 0.03f) {
            game.input.dashQueued = true;
            bot.ticksSinceDash = 0;
        }
        break;
    }

    }  // switch
}
