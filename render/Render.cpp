#include "render/Render.hpp"

#include <cmath>
#include <cstdio>
#include <cstdint>

#include "core/Assets.hpp"
#include "core/Config.hpp"
#include "game/Game.hpp"
#include "render/Palette.hpp"
#include "sim/Level.hpp"

namespace {

// --- Utility ---

float Clamp01(const float value) {
    if (value < 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f;
    return value;
}

Vector3 LerpVec3(const Vector3& a, const Vector3& b, const float t) {
    const float k = Clamp01(t);
    return Vector3{a.x + (b.x - a.x) * k, a.y + (b.y - a.y) * k, a.z + (b.z - a.z) * k};
}

Vector3 InterpolatePosition(const Game& game, const float alpha) {
    return LerpVec3(game.previousPlayer.position, game.player.position, alpha);
}

// --- Simple deterministic hash for seeded positions ---

uint32_t Hash(uint32_t x) {
    x ^= x >> 16;
    x *= 0x45d9f3bu;
    x ^= x >> 16;
    x *= 0x45d9f3bu;
    x ^= x >> 16;
    return x;
}

float HashFloat01(uint32_t seed) {
    return static_cast<float>(Hash(seed) & 0xFFFFu) / 65535.0f;
}

// Stage background color generation
uint32_t HashStage(uint32_t stage) {
    uint32_t x = stage * 7919u;
    x ^= x >> 16;
    x *= 0x45d9f3bu;
    x ^= x >> 16;
    x *= 0x45d9f3bu;
    x ^= x >> 16;
    return x;
}

float HashFloat01Stage(uint32_t seed) {
    return static_cast<float>(HashStage(seed) & 0xFFFFu) / 65535.0f;
}

Color GetStageBackgroundTop(int stage) {
    const uint32_t h = HashStage(static_cast<uint32_t>(stage));
    const float r = 20.0f + HashFloat01Stage(h) * 60.0f;
    const float g = 20.0f + HashFloat01Stage(h + 1u) * 60.0f;
    const float b = 30.0f + HashFloat01Stage(h + 2u) * 80.0f;
    return Color{static_cast<unsigned char>(r), static_cast<unsigned char>(g), static_cast<unsigned char>(b), 255};
}

Color GetStageBackgroundBottom(int stage) {
    const uint32_t h = HashStage(static_cast<uint32_t>(stage) + 1000u);
    const float r = 5.0f + HashFloat01Stage(h) * 20.0f;
    const float g = 5.0f + HashFloat01Stage(h + 1u) * 20.0f;
    const float b = 8.0f + HashFloat01Stage(h + 2u) * 30.0f;
    return Color{static_cast<unsigned char>(r), static_cast<unsigned char>(g), static_cast<unsigned char>(b), 255};
}

}  // namespace

// --- Enhanced space objects system (stars, planets, asteroids) ---

enum class SpaceObjectType {
    Star,
    Planet,
    Asteroid
};

struct SpaceObject {
    Vector3 basePos;        // Base position (for orbital calculations)
    Vector3 currentPos;     // Current rendered position
    float brightness;
    float size;
    SpaceObjectType type;
    
    // Movement properties
    Vector3 velocity;       // Linear velocity
    float orbitalRadius;    // For circular orbits
    float orbitalSpeed;      // Angular speed
    float orbitalAngle;     // Current angle
    Vector3 orbitalAxis;     // Axis of rotation (for 3D orbits)
    float parallaxFactor;   // How much player movement affects this (0-1)
    
    // Visual properties
    int textureIndex;        // For planets/asteroids
    Color tint;              // Color variation
};

bool g_spaceObjectsInited = false;
uint32_t g_currentSpaceSeed = 0u;
constexpr int kMaxSpaceObjects = cfg::kStarCount + 50;  // Stars + extra objects
SpaceObject g_spaceObjects[kMaxSpaceObjects];
int g_spaceObjectCount = 0;

void RegenerateSpaceObjects(uint32_t seed) {
    g_currentSpaceSeed = seed;
    g_spaceObjectCount = 0;
    
    uint32_t rng = seed;
    
    // Generate stars (most common)
    for (int i = 0; i < cfg::kStarCount; ++i) {
        auto& obj = g_spaceObjects[g_spaceObjectCount++];
        obj.type = SpaceObjectType::Star;
        
        const uint32_t s = static_cast<uint32_t>(i) * 7919u + seed;
        obj.basePos = Vector3{
            (HashFloat01(s) - 0.5f) * 2.0f * cfg::kStarFieldRadius,
            HashFloat01(s + 1u) * cfg::kStarFieldHeight + 2.0f,
            (HashFloat01(s + 2u) - 0.3f) * cfg::kStarFieldDepth
        };
        obj.currentPos = obj.basePos;
        obj.brightness = 0.3f + 0.7f * HashFloat01(s + 3u);
        obj.size = 0.06f + HashFloat01(s + 4u) * 0.12f;  // Vary star sizes
        
        // Some stars have slow drift
        if (HashFloat01(s + 5u) > 0.7f) {
            obj.velocity = Vector3{
                (HashFloat01(s + 6u) - 0.5f) * 0.5f,
                (HashFloat01(s + 7u) - 0.5f) * 0.3f,
                (HashFloat01(s + 8u) - 0.5f) * 0.2f
            };
        } else {
            obj.velocity = Vector3{0.0f, 0.0f, 0.0f};
        }
        
        obj.parallaxFactor = 0.05f + HashFloat01(s + 9u) * 0.1f;  // Stars have minimal parallax
        obj.orbitalRadius = 0.0f;
        obj.orbitalSpeed = 0.0f;
        obj.orbitalAngle = 0.0f;
        obj.orbitalAxis = Vector3{0.0f, 1.0f, 0.0f};
        obj.textureIndex = 0;
        obj.tint = WHITE;
    }
    
    // Generate planets (fewer, larger, with orbital motion)
    const int planetCount = 3 + static_cast<int>(HashFloat01(rng) * 5.0f);
    rng = Hash(rng);
    for (int i = 0; i < planetCount && g_spaceObjectCount < kMaxSpaceObjects; ++i) {
        auto& obj = g_spaceObjects[g_spaceObjectCount++];
        obj.type = SpaceObjectType::Planet;
        
        const uint32_t s = static_cast<uint32_t>(i) * 6271u + seed + 1000u;
        const float dist = 80.0f + HashFloat01(s) * 60.0f;  // Far distance
        const float angle = HashFloat01(s + 1u) * 6.28f;     // Random angle around player
        const float height = 10.0f + HashFloat01(s + 2u) * 25.0f;
        
        obj.basePos = Vector3{
            std::sin(angle) * dist,
            height,
            std::cos(angle) * dist
        };
        obj.currentPos = obj.basePos;
        obj.brightness = 0.8f + HashFloat01(s + 3u) * 0.2f;
        obj.size = 6.0f + HashFloat01(s + 4u) * 4.0f;
        obj.textureIndex = static_cast<int>(Hash(s + 5u) % 3u);
        
        // Orbital motion around a center point
        obj.orbitalRadius = 20.0f + HashFloat01(s + 6u) * 30.0f;
        obj.orbitalSpeed = 0.1f + HashFloat01(s + 7u) * 0.3f;  // Slow orbital speed
        obj.orbitalAngle = HashFloat01(s + 8u) * 6.28f;
        const float axisX = (HashFloat01(s + 9u) - 0.5f) * 0.3f;
        const float axisZ = (HashFloat01(s + 10u) - 0.5f) * 0.3f;
        const float axisLen = std::sqrt(axisX * axisX + 1.0f + axisZ * axisZ);
        obj.orbitalAxis = Vector3{axisX / axisLen, 1.0f / axisLen, axisZ / axisLen};
        
        obj.velocity = Vector3{0.0f, 0.0f, 0.0f};
        obj.parallaxFactor = 0.02f;  // Planets move very little with player
        obj.tint = WHITE;
    }
    
    // Generate asteroids (smaller, faster moving)
    const int asteroidCount = 8 + static_cast<int>(HashFloat01(rng) * 12.0f);
    rng = Hash(rng);
    for (int i = 0; i < asteroidCount && g_spaceObjectCount < kMaxSpaceObjects; ++i) {
        auto& obj = g_spaceObjects[g_spaceObjectCount++];
        obj.type = SpaceObjectType::Asteroid;
        
        const uint32_t s = static_cast<uint32_t>(i) * 4513u + seed + 2000u;
        obj.basePos = Vector3{
            (HashFloat01(s) - 0.5f) * 2.0f * cfg::kStarFieldRadius * 0.8f,
            HashFloat01(s + 1u) * cfg::kStarFieldHeight * 0.6f + 5.0f,
            (HashFloat01(s + 2u) - 0.2f) * cfg::kStarFieldDepth * 0.7f
        };
        obj.currentPos = obj.basePos;
        obj.brightness = 0.4f + HashFloat01(s + 3u) * 0.4f;
        obj.size = 0.3f + HashFloat01(s + 4u) * 0.8f;
        
        // Asteroids have noticeable movement
        obj.velocity = Vector3{
            (HashFloat01(s + 5u) - 0.5f) * 2.0f,
            (HashFloat01(s + 6u) - 0.5f) * 1.5f,
            (HashFloat01(s + 7u) - 0.5f) * 1.0f
        };
        
        // Some asteroids have orbital patterns
        if (HashFloat01(s + 8u) > 0.5f) {
            obj.orbitalRadius = 3.0f + HashFloat01(s + 9u) * 5.0f;
            obj.orbitalSpeed = 0.5f + HashFloat01(s + 10u) * 1.5f;
            obj.orbitalAngle = HashFloat01(s + 11u) * 6.28f;
            const float ax = (HashFloat01(s + 12u) - 0.5f);
            const float ay = (HashFloat01(s + 13u) - 0.5f);
            const float az = (HashFloat01(s + 14u) - 0.5f);
            const float aLen = std::sqrt(ax * ax + ay * ay + az * az);
            if (aLen > 0.001f) {
                obj.orbitalAxis = Vector3{ax / aLen, ay / aLen, az / aLen};
            } else {
                obj.orbitalAxis = Vector3{0.0f, 1.0f, 0.0f};
            }
        } else {
            obj.orbitalRadius = 0.0f;
            obj.orbitalSpeed = 0.0f;
            obj.orbitalAngle = 0.0f;
            obj.orbitalAxis = Vector3{0.0f, 1.0f, 0.0f};
        }
        
        obj.parallaxFactor = 0.15f + HashFloat01(s + 15u) * 0.2f;  // More parallax
        obj.textureIndex = 0;
        obj.tint = Color{
            static_cast<unsigned char>(180 + HashFloat01(s + 16u) * 75),
            static_cast<unsigned char>(160 + HashFloat01(s + 17u) * 95),
            static_cast<unsigned char>(140 + HashFloat01(s + 18u) * 115),
            255
        };
    }
    
    g_spaceObjectsInited = true;
}

void UpdateSpaceObjects(float dt, const Vector3& playerPos) {
    for (int i = 0; i < g_spaceObjectCount; ++i) {
        auto& obj = g_spaceObjects[i];
        
        // Update orbital motion
        if (obj.orbitalRadius > 0.0f) {
            obj.orbitalAngle += obj.orbitalSpeed * dt;
            
            // Calculate orbital offset using cross products for perpendicular vectors
            Vector3 up = Vector3{0.0f, 1.0f, 0.0f};
            // Manual cross product: a × b = (ay*bz - az*by, az*bx - ax*bz, ax*by - ay*bx)
            Vector3 right = Vector3{
                obj.orbitalAxis.y * up.z - obj.orbitalAxis.z * up.y,
                obj.orbitalAxis.z * up.x - obj.orbitalAxis.x * up.z,
                obj.orbitalAxis.x * up.y - obj.orbitalAxis.y * up.x
            };
            float rightLen = std::sqrt(right.x * right.x + right.y * right.y + right.z * right.z);
            if (rightLen < 0.1f) {
                // If axis is parallel to up, use a different approach
                right = Vector3{1.0f, 0.0f, 0.0f};
                rightLen = 1.0f;
            }
            right = Vector3{right.x / rightLen, right.y / rightLen, right.z / rightLen};
            
            // Cross product: right × orbitalAxis
            Vector3 forward = Vector3{
                right.y * obj.orbitalAxis.z - right.z * obj.orbitalAxis.y,
                right.z * obj.orbitalAxis.x - right.x * obj.orbitalAxis.z,
                right.x * obj.orbitalAxis.y - right.y * obj.orbitalAxis.x
            };
            float forwardLen = std::sqrt(forward.x * forward.x + forward.y * forward.y + forward.z * forward.z);
            if (forwardLen > 0.001f) {
                forward = Vector3{forward.x / forwardLen, forward.y / forwardLen, forward.z / forwardLen};
            } else {
                forward = Vector3{0.0f, 0.0f, 1.0f};
            }
            
            Vector3 orbitalOffset = Vector3{
                (right.x * std::cos(obj.orbitalAngle) + forward.x * std::sin(obj.orbitalAngle)) * obj.orbitalRadius,
                (right.y * std::cos(obj.orbitalAngle) + forward.y * std::sin(obj.orbitalAngle)) * obj.orbitalRadius,
                (right.z * std::cos(obj.orbitalAngle) + forward.z * std::sin(obj.orbitalAngle)) * obj.orbitalRadius
            };
            
            obj.currentPos = Vector3{
                obj.basePos.x + orbitalOffset.x,
                obj.basePos.y + orbitalOffset.y,
                obj.basePos.z + orbitalOffset.z
            };
        } else {
            obj.currentPos = obj.basePos;
        }
        
        // Apply linear velocity
        obj.currentPos.x += obj.velocity.x * dt;
        obj.currentPos.y += obj.velocity.y * dt;
        obj.currentPos.z += obj.velocity.z * dt;
        
        // Apply parallax based on player position
        obj.currentPos.x += playerPos.x * obj.parallaxFactor;
        obj.currentPos.z += playerPos.z * obj.parallaxFactor * 2.0f;  // More Z parallax
    }
}

namespace {

// --- Pre-seeded decorative cubes ---

struct DecoCube {
    Vector3 pos;
    float size;
    int colorIndex;  // 0, 1, or 2
    float rotSpeed;
};

bool g_decoInited = false;
DecoCube g_decoCubes[cfg::kDecoCubeCount];

void InitDecoCubes() {
    if (g_decoInited) return;
    for (int i = 0; i < cfg::kDecoCubeCount; ++i) {
        const uint32_t s = static_cast<uint32_t>(i) * 6271u + 42u;
        const float side = (HashFloat01(s) > 0.5f) ? 1.0f : -1.0f;
        g_decoCubes[i].pos = Vector3{
            side * (cfg::kDecoCubeSideOffset + HashFloat01(s + 1u) * 8.0f),
            0.5f + HashFloat01(s + 2u) * 4.0f,
            cfg::kPlatformStartZ + HashFloat01(s + 3u) * cfg::kPlatformLength
        };
        g_decoCubes[i].size = cfg::kDecoCubeMinSize + HashFloat01(s + 4u) * (cfg::kDecoCubeMaxSize - cfg::kDecoCubeMinSize);
        g_decoCubes[i].colorIndex = static_cast<int>(Hash(s + 5u) % 3u);
        g_decoCubes[i].rotSpeed = 20.0f + HashFloat01(s + 6u) * 40.0f;
    }
    g_decoInited = true;
}

// --- Pre-seeded ambient particles ---

struct AmbientDot {
    Vector3 basePos;
    float phase;
    float speed;
};

bool g_ambientInited = false;
AmbientDot g_ambientDots[cfg::kAmbientParticleCount];

void InitAmbientDots() {
    if (g_ambientInited) return;
    for (int i = 0; i < cfg::kAmbientParticleCount; ++i) {
        const uint32_t s = static_cast<uint32_t>(i) * 4513u + 77u;
        g_ambientDots[i].basePos = Vector3{
            (HashFloat01(s) - 0.5f) * 2.0f * cfg::kAmbientParticleRadius,
            0.5f + HashFloat01(s + 1u) * cfg::kAmbientParticleHeight,
            (HashFloat01(s + 2u) - 0.3f) * 60.0f
        };
        g_ambientDots[i].phase = HashFloat01(s + 3u) * 6.28f;
        g_ambientDots[i].speed = 0.5f + HashFloat01(s + 4u) * 1.5f;
    }
    g_ambientInited = true;
}

// --- Pre-seeded mountain silhouettes ---

struct Mountain {
    float angle;
    float height;
    float width;
};

bool g_mountainsInited = false;
Mountain g_mountains[cfg::kMountainCount];

void InitMountains() {
    if (g_mountainsInited) return;
    for (int i = 0; i < cfg::kMountainCount; ++i) {
        const uint32_t s = static_cast<uint32_t>(i) * 3137u + 13u;
        g_mountains[i].angle = static_cast<float>(i) / static_cast<float>(cfg::kMountainCount) * 360.0f + HashFloat01(s) * 30.0f;
        g_mountains[i].height = 4.0f + HashFloat01(s + 1u) * cfg::kMountainMaxHeight;
        g_mountains[i].width = 15.0f + HashFloat01(s + 2u) * 25.0f;
    }
    g_mountainsInited = true;
}

// --- Ship model (loaded once) ---

bool g_shipLoaded = false;
Model g_shipModel{};

// --- UI textures (loaded once) ---

bool g_texturesLoaded = false;
Texture2D g_inputTilemap{};
Texture2D g_planetTextures[3]{};  // planet00, planet01, planet03

// --- Engine exhaust pool (render-side, preallocated) ---

struct ExhaustParticle {
    bool active = false;
    Vector3 pos{};
    Vector3 vel{};
    float life = 0.0f;
    float maxLife = 0.0f;
};

ExhaustParticle g_exhaust[cfg::kExhaustParticleCount]{};
int g_exhaustNextIdx = 0;

void SpawnExhaustParticle(const Vector3& origin, float speedBoost, uint32_t& rng) {
    auto& p = g_exhaust[g_exhaustNextIdx];
    g_exhaustNextIdx = (g_exhaustNextIdx + 1) % cfg::kExhaustParticleCount;
    p.active = true;
    p.pos = origin;
    const float sx = (HashFloat01(rng) - 0.5f) * 2.0f * cfg::kExhaustSpreadX;
    rng = Hash(rng + 1);
    const float sy = (HashFloat01(rng) - 0.5f) * 2.0f * cfg::kExhaustSpreadY;
    rng = Hash(rng + 1);
    p.vel = Vector3{sx, sy, -(cfg::kExhaustBaseSpeed + speedBoost * 0.4f)};
    p.life = cfg::kExhaustParticleLife * (0.6f + 0.8f * HashFloat01(rng));
    rng = Hash(rng + 1);
    p.maxLife = p.life;
}

void UpdateExhaustParticles(float dt) {
    for (auto& p : g_exhaust) {
        if (!p.active) continue;
        p.life -= dt;
        if (p.life <= 0.0f) { p.active = false; continue; }
        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;
        p.pos.z += p.vel.z * dt;
        p.vel.x *= (1.0f - 3.0f * dt);
        p.vel.y *= (1.0f - 3.0f * dt);
    }
}

// --- Camera ---

void UpdateFollowCamera(Game& game, const Vector3& playerPos, const float renderDt) {
    if (game.runOver) return;

    const Vector3 desiredTarget = Vector3{playerPos.x, playerPos.y + 1.0f, playerPos.z + 6.0f};
    const Vector3 desiredPos = Vector3{playerPos.x, playerPos.y + 3.0f, playerPos.z - 10.0f};

    const Vector3 clampedTarget = Vector3{
        desiredTarget.x,
        (desiredTarget.y < 0.8f) ? 0.8f : desiredTarget.y,
        desiredTarget.z
    };
    const Vector3 clampedPos = Vector3{
        desiredPos.x,
        (desiredPos.y < 2.5f) ? 2.5f : desiredPos.y,
        desiredPos.z
    };

    const float sf = 1.0f - std::exp(-6.0f * renderDt);
    game.cameraTarget = LerpVec3(game.cameraTarget, clampedTarget, sf);
    game.cameraPosition = LerpVec3(game.cameraPosition, clampedPos, sf);

    const float desiredRoll = -Clamp01(std::fabs(game.player.velocity.x) / cfg::kStrafeSpeed) *
                              ((game.player.velocity.x >= 0.0f) ? cfg::kCameraRollMaxDeg : -cfg::kCameraRollMaxDeg);
    const float rollLerp = 1.0f - std::exp(-cfg::kCameraRollSmoothing * renderDt);
    game.cameraRollDeg += (desiredRoll - game.cameraRollDeg) * rollLerp;
    const float rollRad = game.cameraRollDeg * DEG2RAD;

    game.camera.up = Vector3{std::sin(rollRad), std::cos(rollRad), 0.0f};
    game.camera.target = game.cameraTarget;
    game.camera.position = game.cameraPosition;
}

Color GetDecoCubeColor(const LevelPalette& p, int idx) {
    if (idx == 0) return p.decoCube1;
    if (idx == 1) return p.decoCube2;
    return p.decoCube3;
}

}  // namespace

// Keep old InitStars for backward compatibility, but make it call RegenerateSpaceObjects
void InitStars() {
    if (!g_spaceObjectsInited || g_currentSpaceSeed == 0u) {
        RegenerateSpaceObjects(12345u);  // Default seed if not set
    }
}

void InitRenderer() {
    if (!g_shipLoaded) {
        g_shipModel = LoadModel(assets::Path("models/craft_speederA.obj"));
        g_shipLoaded = true;
    }
    if (!g_texturesLoaded) {
        // Load input prompts tilemap
        g_inputTilemap = LoadTexture(assets::Path("kenney_input-prompts-pixel-16/Tilemap/tilemap.png"));
        SetTextureFilter(g_inputTilemap, TEXTURE_FILTER_POINT);  // Pixel-perfect
        
        // Load planet textures for background
        g_planetTextures[0] = LoadTexture(assets::Path("kenney_planets/Planets/planet00.png"));
        g_planetTextures[1] = LoadTexture(assets::Path("kenney_planets/Planets/planet01.png"));
        g_planetTextures[2] = LoadTexture(assets::Path("kenney_planets/Planets/planet03.png"));
        for (int i = 0; i < 3; ++i) {
            SetTextureFilter(g_planetTextures[i], TEXTURE_FILTER_BILINEAR);
        }
        g_texturesLoaded = true;
    }
}

void CleanupRenderer() {
    if (g_shipLoaded) {
        UnloadModel(g_shipModel);
        g_shipLoaded = false;
    }
    if (g_texturesLoaded) {
        UnloadTexture(g_inputTilemap);
        for (int i = 0; i < 3; ++i) {
            UnloadTexture(g_planetTextures[i]);
        }
        g_texturesLoaded = false;
    }
}

void RenderFrame(Game& game, const float alpha, const float renderDt) {
    // Lazy-init static scene data (only once, no per-frame alloc).
    InitDecoCubes();
    InitAmbientDots();
    InitMountains();

    const LevelPalette& pal = GetPalette(game.paletteIndex);
    const Vector3 playerRenderPos = InterpolatePosition(game, alpha);
    UpdateFollowCamera(game, playerRenderPos, renderDt);

    const float planarSpeed = std::sqrt(
        (game.player.velocity.x * game.player.velocity.x) +
        (game.player.velocity.z * game.player.velocity.z)
    );
    const float speedT = Clamp01((planarSpeed - cfg::kForwardSpeed) / cfg::kDashSpeedBoost);
    game.camera.fovy = cfg::kCameraBaseFov + (cfg::kCameraMaxFov - cfg::kCameraBaseFov) * speedT;

    const float simTime = static_cast<float>(game.simTicks) * cfg::kFixedDt;

    // Regenerate space objects if seed changed (when level starts)
    static uint32_t lastRunSeed = 0u;
    if (game.runSeed != lastRunSeed) {
        RegenerateSpaceObjects(game.runSeed);
        lastRunSeed = game.runSeed;
    }
    
    // Update space objects with movement and orbital patterns
    UpdateSpaceObjects(renderDt, playerRenderPos);

    BeginDrawing();
    ClearBackground(BLACK);

    // Sky gradient - use stage colors if playing, otherwise use palette
    Color skyTop = pal.skyTop;
    Color skyBottom = pal.skyBottom;
    if (game.screen == GameScreen::Playing && game.currentStage >= 1 && game.currentStage <= 10) {
        skyTop = GetStageBackgroundTop(game.currentStage);
        skyBottom = GetStageBackgroundBottom(game.currentStage);
    }
    DrawRectangleGradientV(0, 0, cfg::kScreenWidth, cfg::kScreenHeight, skyTop, skyBottom);
    DrawRectangleGradientV(0, cfg::kScreenHeight / 2, cfg::kScreenWidth, cfg::kScreenHeight / 2,
                           Fade(BLACK, 0.0f), pal.voidTint);

    BeginMode3D(game.camera);

    // ---- Enhanced Space Objects Rendering ----
    for (int i = 0; i < g_spaceObjectCount; ++i) {
        const auto& obj = g_spaceObjects[i];
        
        if (obj.type == SpaceObjectType::Star) {
            const float twinkle = 0.6f + 0.4f * std::sin(simTime * (1.5f + obj.brightness * 2.0f) + obj.brightness * 20.0f);
            const Color col = (obj.brightness > 0.7f) 
                ? Fade(pal.starBright, twinkle * obj.brightness)
                : Fade(pal.starDim, twinkle * obj.brightness);
            DrawCubeV(obj.currentPos, Vector3{obj.size, obj.size, obj.size}, col);
        }
        else if (obj.type == SpaceObjectType::Planet && g_texturesLoaded) {
            // Render planets as billboards
            DrawBillboardRec(game.camera, g_planetTextures[obj.textureIndex],
                           Rectangle{0.0f, 0.0f, 
                                    static_cast<float>(g_planetTextures[obj.textureIndex].width),
                                    static_cast<float>(g_planetTextures[obj.textureIndex].height)},
                           obj.currentPos, Vector2{obj.size, obj.size}, obj.tint);
        }
        else if (obj.type == SpaceObjectType::Asteroid) {
            // Render asteroids as small rotating cubes
            const Color asteroidCol = Fade(obj.tint, obj.brightness);
            DrawCubeV(obj.currentPos, Vector3{obj.size, obj.size, obj.size}, asteroidCol);
            DrawCubeWiresV(obj.currentPos, Vector3{obj.size, obj.size, obj.size}, 
                          Fade(asteroidCol, 0.6f));
        }
    }

    // ---- Mountain silhouettes ----
    for (int i = 0; i < cfg::kMountainCount; ++i) {
        const Mountain& m = g_mountains[i];
        const float rad = m.angle * DEG2RAD;
        const Vector3 base = Vector3{
            std::sin(rad) * cfg::kMountainDistance + playerRenderPos.x * 0.02f,
            -2.0f,
            std::cos(rad) * cfg::kMountainDistance + playerRenderPos.z * 0.05f
        };
        DrawCubeV(base, Vector3{m.width, m.height, m.width * 0.5f}, pal.mountainSilhouette);
    }

    // ---- Level segments ----
    const Level* lv = game.level;
    const float guideY = cfg::kPlatformTopY + 0.02f;  // fallback; per-segment below

    if (lv) {
        for (int si = 0; si < lv->segmentCount; ++si) {
            const auto& seg = lv->segments[si];
            // Distance culling — only draw segments near the player.
            const float segMidZ = seg.startZ + seg.length * 0.5f;
            if (std::fabs(segMidZ - playerRenderPos.z) > 80.0f) continue;

            const float segEndZ = seg.startZ + seg.length;
            const float halfW = seg.width * 0.5f;

            // Platform body.
            DrawCubeV(Vector3{seg.xOffset, seg.topY - cfg::kPlatformHeight * 0.5f, segMidZ},
                      Vector3{seg.width, cfg::kPlatformHeight, seg.length}, pal.platformSide);
            // Top surface.
            DrawCubeV(Vector3{seg.xOffset, seg.topY - 0.01f, segMidZ},
                      Vector3{seg.width, 0.02f, seg.length}, pal.platformTop);
            // Wireframe.
            DrawCubeWiresV(Vector3{seg.xOffset, seg.topY - cfg::kPlatformHeight * 0.5f, segMidZ},
                           Vector3{seg.width, cfg::kPlatformHeight, seg.length}, Fade(pal.platformWire, 0.5f));

            // Neon edges.
            const float leftEdge = seg.xOffset - halfW;
            const float rightEdge = seg.xOffset + halfW;
            const float edgeY = seg.topY + cfg::kNeonEdgeHeight * 0.5f;
            DrawCubeV(Vector3{leftEdge, edgeY, segMidZ},
                      Vector3{cfg::kNeonEdgeWidth, cfg::kNeonEdgeHeight, seg.length}, pal.neonEdge);
            DrawCubeV(Vector3{rightEdge, edgeY, segMidZ},
                      Vector3{cfg::kNeonEdgeWidth, cfg::kNeonEdgeHeight, seg.length}, pal.neonEdge);
            DrawCubeV(Vector3{leftEdge, edgeY, segMidZ},
                      Vector3{cfg::kNeonEdgeWidth * 3.0f, cfg::kNeonEdgeHeight * 2.5f, seg.length},
                      Fade(pal.neonEdgeGlow, 0.15f));
            DrawCubeV(Vector3{rightEdge, edgeY, segMidZ},
                      Vector3{cfg::kNeonEdgeWidth * 3.0f, cfg::kNeonEdgeHeight * 2.5f, seg.length},
                      Fade(pal.neonEdgeGlow, 0.15f));

            // Grid lines on this segment.
            const float sGuideY = seg.topY + 0.02f;
            for (int gi = 0; gi < cfg::kGridLongitudinalCount; ++gi) {
                const float t = static_cast<float>(gi) / static_cast<float>(cfg::kGridLongitudinalCount - 1);
                const float gx = seg.xOffset - halfW + t * seg.width;
                DrawLine3D(Vector3{gx, sGuideY, seg.startZ}, Vector3{gx, sGuideY, segEndZ}, Fade(pal.gridLine, 0.3f));
            }
            // Lateral grid (only near player for perf).
            if (std::fabs(segMidZ - playerRenderPos.z) < 30.0f) {
                const float latPhase = std::fmod(playerRenderPos.z, cfg::kGridLateralSpacing);
                for (int li = -2; li < 14; ++li) {
                    const float lz = playerRenderPos.z - 6.0f + static_cast<float>(li) * cfg::kGridLateralSpacing - latPhase;
                    if (lz < seg.startZ || lz > segEndZ) continue;
                    DrawLine3D(Vector3{seg.xOffset - halfW, sGuideY, lz},
                               Vector3{seg.xOffset + halfW, sGuideY, lz}, Fade(pal.gridLine, 0.2f));
                }
            }
        }

        // ---- Level obstacles ----
        for (int oi = 0; oi < lv->obstacleCount; ++oi) {
            const auto& ob = lv->obstacles[oi];
            if (std::fabs(ob.z - playerRenderPos.z) > 60.0f) continue;
            const Vector3 obCenter = Vector3{ob.x, ob.y + ob.sizeY * 0.5f, ob.z};
            const Vector3 obSize = Vector3{ob.sizeX, ob.sizeY, ob.sizeZ};
            const Color obCol = GetDecoCubeColor(pal, ob.colorIndex);
            DrawCubeV(obCenter, obSize, Fade(obCol, 0.4f));
            DrawCubeWiresV(obCenter, obSize, obCol);
            // Glow on ground.
            DrawCubeV(Vector3{ob.x, ob.y + 0.02f, ob.z},
                      Vector3{ob.sizeX * 1.5f, 0.01f, ob.sizeZ * 1.5f}, Fade(obCol, 0.15f));
        }
    }

    // ---- Scrolling track bands (motion feel, drawn on nearest segment) ----
    constexpr int kTrackBandCount = 12;
    constexpr float kTrackBandSpacing = 3.2f;
    const float bandPhase = std::fmod(simTime * planarSpeed, kTrackBandSpacing);
    if (lv) {
        const int nearSeg = FindSegmentUnder(*lv, playerRenderPos.z, playerRenderPos.x, cfg::kPlayerWidth * 0.5f);
        if (nearSeg >= 0) {
            const auto& seg = lv->segments[nearSeg];
            const float sGuideY = seg.topY + 0.02f;
            for (int i = 0; i < kTrackBandCount; ++i) {
                const float z = playerRenderPos.z - 16.0f + static_cast<float>(i) * kTrackBandSpacing + bandPhase;
                if (z < seg.startZ || z > seg.startZ + seg.length) continue;
                const float nearT = 1.0f - static_cast<float>(i) / static_cast<float>(kTrackBandCount);
                const float a = 0.06f + 0.18f * nearT + 0.18f * speedT;
                DrawCubeV(Vector3{seg.xOffset, sGuideY + 0.01f, z},
                          Vector3{seg.width * 0.85f, 0.015f, 0.2f}, Fade(pal.laneGlow, a));
            }
        }
    }

    // ---- Speed streaks (only above threshold, more numerous) ----
    if (planarSpeed > cfg::kSpeedLineMinSpeed) {
        const float streakLen = 4.0f + 4.0f * speedT;
        const float sHalfW = cfg::kPlatformWidth * 0.5f;
        const float leftX = -sHalfW + 0.4f;
        const float rightX = sHalfW - 0.4f;
        const int streakCount = 5 + static_cast<int>(speedT * 4.0f);
        for (int s = 0; s < streakCount; ++s) {
            const float phase = std::fmod(simTime * (cfg::kForwardSpeed + cfg::kDashSpeedBoost) + static_cast<float>(s) * 2.1f, 14.0f);
            const float sz = playerRenderPos.z - 10.0f + phase;
            const float xOff = (static_cast<float>(s % 3) - 1.0f) * 0.3f;
            DrawLine3D(Vector3{leftX + xOff, guideY + 0.02f, sz}, Vector3{leftX + xOff, guideY + 0.02f, sz + streakLen},
                       Fade(pal.streak, 0.12f + 0.4f * speedT));
            DrawLine3D(Vector3{rightX - xOff, guideY + 0.02f, sz}, Vector3{rightX - xOff, guideY + 0.02f, sz + streakLen},
                       Fade(pal.streak, 0.12f + 0.4f * speedT));
        }
        // Side speed lines (off-track, like the screenshots).
        for (int s = 0; s < 4; ++s) {
            const float phase = std::fmod(simTime * planarSpeed * 0.3f + static_cast<float>(s) * 3.5f, 16.0f);
            const float sz = playerRenderPos.z - 6.0f + phase;
            const float sideX = sHalfW + 1.0f + static_cast<float>(s) * 1.5f;
            const float y = 0.5f + static_cast<float>(s % 2) * 1.2f;
            const float len = 2.0f + 2.0f * speedT;
            DrawLine3D(Vector3{-sideX, y, sz}, Vector3{-sideX, y, sz + len}, Fade(pal.streak, 0.08f + 0.2f * speedT));
            DrawLine3D(Vector3{ sideX, y, sz}, Vector3{ sideX, y, sz + len}, Fade(pal.streak, 0.08f + 0.2f * speedT));
        }
    }

    // ---- Decorative floating cubes with glow halos ----
    for (int i = 0; i < cfg::kDecoCubeCount; ++i) {
        const DecoCube& dc = g_decoCubes[i];
        if (std::fabs(dc.pos.z - playerRenderPos.z) > 60.0f) continue;

        const float bob = std::sin(simTime * dc.rotSpeed * 0.03f + dc.pos.x) * 0.4f;
        const Vector3 pos = Vector3{dc.pos.x, dc.pos.y + bob, dc.pos.z};
        const Color col = GetDecoCubeColor(pal, dc.colorIndex);
        // Glow halo on ground beneath cube.
        DrawCubeV(Vector3{dc.pos.x, cfg::kPlatformTopY + 0.03f, dc.pos.z},
                  Vector3{dc.size * 1.6f, 0.01f, dc.size * 1.6f}, Fade(col, 0.15f));
        DrawCubeWiresV(pos, Vector3{dc.size, dc.size, dc.size}, col);
        DrawCubeV(pos, Vector3{dc.size * 0.7f, dc.size * 0.7f, dc.size * 0.7f}, Fade(col, 0.25f));
    }

    // ---- Ambient floating particles ----
    for (int i = 0; i < cfg::kAmbientParticleCount; ++i) {
        const AmbientDot& d = g_ambientDots[i];
        const float drift = std::sin(simTime * d.speed + d.phase) * 2.0f;
        const Vector3 pos = Vector3{
            d.basePos.x + playerRenderPos.x * 0.08f + drift,
            d.basePos.y + std::sin(simTime * 0.8f + d.phase) * 1.0f,
            d.basePos.z + playerRenderPos.z * 0.15f
        };
        if (std::fabs(pos.z - playerRenderPos.z) > 50.0f) continue;
        DrawCubeV(pos, Vector3{0.05f, 0.05f, 0.05f}, Fade(pal.ambientParticle, 0.4f + 0.3f * std::sin(simTime + d.phase)));
    }

    // ---- Player ship ----
    // Glow halo under ship.
    DrawCubeV(Vector3{playerRenderPos.x, cfg::kPlatformTopY + 0.03f, playerRenderPos.z},
              Vector3{cfg::kPlayerWidth * 2.2f, 0.01f, cfg::kPlayerDepth * 2.2f}, Fade(pal.playerGlow, 0.35f));
    // Wider soft glow layer.
    DrawCubeV(Vector3{playerRenderPos.x, cfg::kPlatformTopY + 0.025f, playerRenderPos.z},
              Vector3{cfg::kPlayerWidth * 3.5f, 0.005f, cfg::kPlayerDepth * 3.5f}, Fade(pal.playerGlow, 0.12f));

    if (g_shipLoaded) {
        // The Kenney model faces -Z by default; we rotate 180 so it faces +Z (our forward).
        // Model center is at Y=0.4, offset so bottom sits on platform.
        const float scale = cfg::kShipModelScale;
        const Vector3 shipPos = Vector3{
            playerRenderPos.x,
            playerRenderPos.y - cfg::kPlayerHalfHeight + 0.05f,
            playerRenderPos.z
        };
        DrawModelEx(g_shipModel, shipPos, Vector3{0.0f, 1.0f, 0.0f}, 180.0f,
                    Vector3{scale, scale, scale}, pal.playerBody);
        DrawModelWiresEx(g_shipModel, shipPos, Vector3{0.0f, 1.0f, 0.0f}, 180.0f,
                         Vector3{scale, scale, scale}, Fade(pal.neonEdgeGlow, 0.6f));
    } else {
        // Fallback cube if model failed to load.
        DrawCubeV(playerRenderPos,
                  Vector3{cfg::kPlayerWidth, cfg::kPlayerHalfHeight * 2.0f, cfg::kPlayerDepth}, pal.playerBody);
        DrawCubeWiresV(playerRenderPos,
                       Vector3{cfg::kPlayerWidth, cfg::kPlayerHalfHeight * 2.0f, cfg::kPlayerDepth}, pal.playerWire);
    }

    // ---- Engine exhaust trail ----
    {
        static uint32_t exhaustRng = 12345u;
        const Vector3 exhaustOrigin = Vector3{
            playerRenderPos.x,
            playerRenderPos.y - cfg::kPlayerHalfHeight + 0.2f,
            playerRenderPos.z - cfg::kPlayerDepth * 0.5f * cfg::kShipModelScale - 0.1f
        };
        // Spawn 2-4 particles per frame depending on speed.
        const int spawnCount = 2 + static_cast<int>(speedT * 2.0f);
        for (int i = 0; i < spawnCount; ++i) {
            SpawnExhaustParticle(exhaustOrigin, planarSpeed - cfg::kForwardSpeed, exhaustRng);
        }
        UpdateExhaustParticles(renderDt);

        for (const auto& ep : g_exhaust) {
            if (!ep.active) continue;
            const float lifeT = Clamp01(ep.life / ep.maxLife);
            const float sz = 0.04f + 0.1f * lifeT;
            // Orange core -> yellow -> fade.
            const Color core = Color{255, static_cast<unsigned char>(140 + 100 * (1.0f - lifeT)), 30, static_cast<unsigned char>(220 * lifeT)};
            DrawCubeV(ep.pos, Vector3{sz, sz, sz * 1.5f}, core);
        }
        // Wider exhaust glow cone.
        DrawCubeV(Vector3{exhaustOrigin.x, exhaustOrigin.y, exhaustOrigin.z - 0.3f},
                  Vector3{0.4f + 0.2f * speedT, 0.2f, 0.6f + 0.4f * speedT},
                  Fade(Color{255, 160, 40, 255}, 0.15f + 0.1f * speedT));
    }

    // ---- Landing particles ----
    for (const auto& p : game.landingParticles) {
        if (!p.active) continue;
        const float lifeT = Clamp01(p.life / cfg::kLandingParticleLife);
        DrawCubeV(p.position, Vector3{0.08f, 0.08f, 0.08f}, Fade(pal.particle, lifeT));
    }

    EndMode3D();

    // ---- Bloom overlay ----
    if (game.bloomEnabled) {
        DrawRectangleGradientV(0, 0, cfg::kScreenWidth, cfg::kScreenHeight / 3,
                               Fade(pal.uiAccent, cfg::kBloomOverlayAlpha), Fade(BLACK, 0.0f));
        DrawRectangleGradientV(0, cfg::kScreenHeight / 2, cfg::kScreenWidth, cfg::kScreenHeight / 2,
                               Fade(BLACK, 0.0f), Fade(pal.laneGlow, cfg::kBloomOverlayAlpha * 0.65f));
    }

    // ======== SCREEN-SPECIFIC OVERLAYS ========

    const int cx = cfg::kScreenWidth / 2;
    const int cy = cfg::kScreenHeight / 2;

    if (game.screen == GameScreen::MainMenu) {
        // Dim background.
        DrawRectangle(0, 0, cfg::kScreenWidth, cfg::kScreenHeight, Fade(BLACK, 0.72f));

        DrawText("S K Y R O A D S", cx - 180, cy - 140, 44, pal.uiAccent);
        DrawText("Endless Runner", cx - 90, cy - 90, 20, pal.uiText);

        const char* items[] = {"Start Game", "Leaderboard", "Exit"};
        for (int i = 0; i < 3; ++i) {
            const bool sel = (game.menuSelection == i);
            const int y = cy - 20 + i * 40;
            if (sel) {
                DrawRectangleRounded(Rectangle{static_cast<float>(cx - 140), static_cast<float>(y - 6), 280.0f, 36.0f},
                                     0.15f, 8, Fade(pal.uiAccent, 0.18f));
            }
            DrawText(sel ? ">" : " ", cx - 130, y, 22, sel ? pal.uiAccent : pal.uiText);
            DrawText(items[i], cx - 100, y, 22, sel ? pal.uiAccent : pal.uiText);
        }

        DrawText("Use UP/DOWN + ENTER", cx - 105, cy + 110, 16, Fade(pal.uiText, 0.6f));

    } else if (game.screen == GameScreen::LevelSelect) {
        DrawRectangle(0, 0, cfg::kScreenWidth, cfg::kScreenHeight, Fade(BLACK, 0.75f));
        DrawText("S E L E C T   L E V E L", cx - 200, 30, 36, pal.uiAccent);

        // Draw stage/level grid - 2 rows of 5 stages each
        const int startX = 60;
        const int startY = 100;
        const int stageWidth = 110;
        const int stageHeight = 140;
        const int stageSpacing = 15;
        const int levelSpacing = 32;
        const int rowSpacing = 20;
        const int stagesPerRow = 5;

        for (int stage = 1; stage <= 10; ++stage) {
            const int row = (stage - 1) / stagesPerRow;
            const int col = (stage - 1) % stagesPerRow;
            const int stageX = startX + col * (stageWidth + stageSpacing);
            const int stageY = startY + row * (stageHeight + rowSpacing);

            // Stage background preview
            const Color stageTop = GetStageBackgroundTop(stage);
            const Color stageBottom = GetStageBackgroundBottom(stage);
            DrawRectangleGradientV(stageX, stageY, stageWidth, stageHeight, stageTop, stageBottom);
            
            // Highlight selected stage
            if (game.levelSelectStage == stage) {
                DrawRectangleLinesEx(Rectangle{static_cast<float>(stageX - 2), static_cast<float>(stageY - 2),
                                             static_cast<float>(stageWidth + 4), static_cast<float>(stageHeight + 4)},
                                   3.0f, pal.uiAccent);
            } else {
                DrawRectangleLinesEx(Rectangle{static_cast<float>(stageX), static_cast<float>(stageY),
                                             static_cast<float>(stageWidth), static_cast<float>(stageHeight)},
                                   2.0f, Fade(pal.uiAccent, 0.5f));
            }

            // Stage label
            char stageLabel[32];
            std::snprintf(stageLabel, sizeof(stageLabel), "S%d", stage);
            DrawText(stageLabel, stageX + 5, stageY + 5, 16, pal.uiAccent);

            // Level buttons
            for (int level = 1; level <= 3; ++level) {
                const int levelIndex = GetLevelIndexFromStageAndLevel(stage, level);
                const bool isImplemented = IsLevelImplemented(levelIndex);
                const bool isSelected = (game.levelSelectStage == stage && game.levelSelectLevel == level);
                const int levelY = stageY + 28 + (level - 1) * levelSpacing;

                // Level button background
                Color levelBg = isSelected ? Fade(pal.uiAccent, 0.4f) : Fade(BLACK, 0.5f);
                if (!isImplemented) {
                    levelBg = Fade(BLACK, 0.7f);
                }
                DrawRectangleRounded(Rectangle{static_cast<float>(stageX + 5), static_cast<float>(levelY - 3),
                                               static_cast<float>(stageWidth - 10), 26.0f},
                                     0.1f, 8, levelBg);

                // Level label
                char levelLabel[32];
                std::snprintf(levelLabel, sizeof(levelLabel), "L%d", level);
                Color levelColor = isSelected ? pal.uiAccent : pal.uiText;
                if (!isImplemented) {
                    levelColor = Fade(pal.uiText, 0.3f);
                }
                DrawText(levelLabel, stageX + 12, levelY, 14, levelColor);

                // Lock icon for unimplemented
                if (!isImplemented) {
                    DrawText("X", stageX + stageWidth - 20, levelY, 12, Fade(pal.uiText, 0.4f));
                }
            }
        }

        // Instructions
        DrawText("ARROWS: Navigate  ENTER: Select  ESC: Back", cx - 200, cfg::kScreenHeight - 50, 16, Fade(pal.uiText, 0.7f));

    } else if (game.screen == GameScreen::PlaceholderLevel) {
        // Black screen with message
        DrawRectangle(0, 0, cfg::kScreenWidth, cfg::kScreenHeight, BLACK);
        
        const int stage = GetStageFromLevelIndex(game.currentLevelIndex);
        const int level = GetLevelInStageFromLevelIndex(game.currentLevelIndex);
        
        DrawText("Didn't implement yet", cx - 140, cy - 40, 32, pal.uiAccent);
        
        char infoText[64];
        std::snprintf(infoText, sizeof(infoText), "Stage %d - Level %d", stage, level);
        DrawText(infoText, cx - 100, cy + 20, 20, pal.uiText);
        
        DrawText("Press any key to return", cx - 120, cy + 60, 16, Fade(pal.uiText, 0.7f));

    } else if (game.screen == GameScreen::Leaderboard) {
        DrawRectangle(0, 0, cfg::kScreenWidth, cfg::kScreenHeight, Fade(BLACK, 0.78f));
        DrawText("L E A D E R B O A R D", cx - 200, 60, 36, pal.uiAccent);

        if (game.leaderboardCount == 0) {
            DrawText("No scores yet. Go play!", cx - 130, cy - 20, 22, pal.uiText);
        } else {
            DrawText("#   Name                Score       Time", cx - 260, 120, 15, Fade(pal.uiText, 0.5f));
            for (int i = 0; i < game.leaderboardCount; ++i) {
                const auto& e = game.leaderboard[i];
                char line[128];
                std::snprintf(line, sizeof(line), "%-2d  %-18s  %-10.0f  %.1fs",
                              i + 1, e.name, e.score, e.runTime);
                const int y = 146 + i * 28;
                DrawText(line, cx - 260, y, 17, (i == 0) ? pal.uiAccent : pal.uiText);
            }
        }

        DrawText("Press ESC or ENTER to go back", cx - 160, cfg::kScreenHeight - 50, 16, Fade(pal.uiText, 0.6f));

    } else if (game.screen == GameScreen::Paused) {
        DrawRectangle(0, 0, cfg::kScreenWidth, cfg::kScreenHeight, Fade(BLACK, 0.65f));
        DrawText("P A U S E D", cx - 110, 60, 40, pal.uiAccent);

        // Statistics panel (left side)
        const int statsX = 60;
        const int statsY = 140;
        DrawRectangleRounded(Rectangle{static_cast<float>(statsX - 10), static_cast<float>(statsY - 10),
                            380.0f, 400.0f}, 0.08f, 8, pal.uiPanel);
        DrawText("Run Statistics", statsX, statsY, 24, pal.uiAccent);
        
        char levelLabel[32];
        std::snprintf(levelLabel, sizeof(levelLabel), "Level %d", game.currentLevelIndex);
        DrawText(levelLabel, statsX, statsY + 28, 18, Fade(pal.uiAccent, 0.9f));
        
        const float distance = game.player.position.z - cfg::kPlatformStartZ;
        const float currentScore = GetCurrentScore(game);
        
        int statY = statsY + 60;
        const int statSpacing = 28;
        
        char statBuf[128];
        
        std::snprintf(statBuf, sizeof(statBuf), "Score: %.0f", currentScore);
        DrawText(statBuf, statsX, statY, 18, pal.uiText);
        statY += statSpacing;
        
        std::snprintf(statBuf, sizeof(statBuf), "Best Score: %.0f", game.bestScore);
        DrawText(statBuf, statsX, statY, 18, Fade(pal.uiAccent, 0.9f));
        statY += statSpacing;
        
        std::snprintf(statBuf, sizeof(statBuf), "Distance: %.1f u", distance);
        DrawText(statBuf, statsX, statY, 18, pal.uiText);
        statY += statSpacing;
        
        std::snprintf(statBuf, sizeof(statBuf), "Time: %.1f s", game.runTime);
        DrawText(statBuf, statsX, statY, 18, pal.uiText);
        statY += statSpacing;
        
        std::snprintf(statBuf, sizeof(statBuf), "Speed: %.1f u/s", planarSpeed);
        DrawText(statBuf, statsX, statY, 18, pal.uiText);
        statY += statSpacing;
        
        std::snprintf(statBuf, sizeof(statBuf), "Multiplier: x%.2f", game.scoreMultiplier);
        DrawText(statBuf, statsX, statY, 18, pal.uiText);
        statY += statSpacing;
        
        std::snprintf(statBuf, sizeof(statBuf), "Difficulty: %.1f%%", game.difficultyT * 100.0f);
        DrawText(statBuf, statsX, statY, 18, pal.uiText);
        statY += statSpacing;
        
        std::snprintf(statBuf, sizeof(statBuf), "Speed Bonus: +%.1f u/s", game.diffSpeedBonus);
        DrawText(statBuf, statsX, statY, 18, Fade(pal.uiText, 0.8f));
        statY += statSpacing;
        
        std::snprintf(statBuf, sizeof(statBuf), "Seed: 0x%08X", game.runSeed);
        DrawText(statBuf, statsX, statY, 16, Fade(pal.uiText, 0.6f));

        // Menu (right side)
        const int menuX = cx + 200;
        const int menuY = statsY + 60;  // Start menu below the title
        const char* items[] = {"Resume", "Restart", "Main Menu"};
        for (int i = 0; i < 3; ++i) {
            const bool sel = (game.pauseSelection == i);
            const int y = menuY + i * 50;
            if (sel) {
                DrawRectangleRounded(Rectangle{static_cast<float>(menuX - 10), static_cast<float>(y - 6), 200.0f, 36.0f},
                                     0.15f, 8, Fade(pal.uiAccent, 0.18f));
            }
            DrawText(sel ? ">" : " ", menuX, y, 22, sel ? pal.uiAccent : pal.uiText);
            DrawText(items[i], menuX + 20, y, 22, sel ? pal.uiAccent : pal.uiText);
        }

        DrawText("ESC/P to resume", cx - 85, cfg::kScreenHeight - 50, 16, Fade(pal.uiText, 0.6f));

    } else if (game.screen == GameScreen::ExitConfirm) {
        DrawRectangle(0, 0, cfg::kScreenWidth, cfg::kScreenHeight, Fade(BLACK, 0.80f));
        DrawText("Exit Game?", cx - 100, cy - 60, 40, pal.uiAccent);

        const char* items[] = {"No", "Yes"};
        for (int i = 0; i < 2; ++i) {
            const bool sel = (game.exitConfirmSelection == i);
            const int y = cy - 10 + i * 50;
            if (sel) {
                DrawRectangleRounded(Rectangle{static_cast<float>(cx - 80), static_cast<float>(y - 6), 160.0f, 36.0f},
                                     0.15f, 8, Fade(pal.uiAccent, 0.18f));
            }
            DrawText(sel ? ">" : " ", cx - 70, y, 22, sel ? pal.uiAccent : pal.uiText);
            DrawText(items[i], cx - 50, y, 22, sel ? pal.uiAccent : pal.uiText);
        }

        DrawText("Use UP/DOWN + ENTER", cx - 105, cy + 110, 16, Fade(pal.uiText, 0.6f));
        DrawText("ESC to cancel", cx - 70, cy + 135, 16, Fade(pal.uiText, 0.6f));

    } else if (game.screen == GameScreen::NameEntry) {
        DrawRectangle(0, 0, cfg::kScreenWidth, cfg::kScreenHeight, Fade(BLACK, 0.75f));

        DrawText("NEW HIGH SCORE!", cx - 140, cy - 80, 32, pal.neonEdgeGlow);
        
        char scoreText[96];
        std::snprintf(scoreText, sizeof(scoreText), "Score: %.0f", game.pendingEntry.score);
        DrawText(scoreText, cx - 70, cy - 30, 24, pal.uiText);

        DrawText("Enter your name:", cx - 100, cy + 10, 20, pal.uiText);
        
        // Draw input box
        Rectangle inputBox = {cx - 100.0f, cy + 40.0f, 200.0f, 35.0f};
        DrawRectangleLinesEx(inputBox, 2.0f, pal.uiAccent);
        
        // Draw current input with cursor
        char displayText[22];
        std::snprintf(displayText, sizeof(displayText), "%s_", game.nameInputBuffer);
        DrawText(displayText, cx - 95, cy + 48, 18, pal.uiText);

        DrawText("ENTER to confirm", cx - 85, cy + 90, 16, Fade(pal.uiText, 0.7f));
        DrawText("ESC to skip", cx - 60, cy + 110, 16, Fade(pal.uiText, 0.6f));

    } else if (game.screen == GameScreen::GameOver) {
        DrawRectangle(0, 0, cfg::kScreenWidth, cfg::kScreenHeight, Fade(BLACK, 0.65f));

        const char* overTitle = game.levelComplete ? "L E V E L   C L E A R" : "G A M E   O V E R";
        DrawText(overTitle, cx - 170, cy - 90, 40, game.levelComplete ? pal.neonEdgeGlow : pal.uiAccent);

        if (game.levelComplete) {
            char levelText[64];
            std::snprintf(levelText, sizeof(levelText), "Level %d Complete!", game.currentLevelIndex);
            DrawText(levelText, cx - 100, cy - 40, 22, pal.uiAccent);
        }

        char finalScore[96];
        std::snprintf(finalScore, sizeof(finalScore), "Score: %.0f", GetCurrentScore(game));
        DrawText(finalScore, cx - 70, cy - 10, 26, pal.uiText);

        char bestText[96];
        std::snprintf(bestText, sizeof(bestText), "Best: %.0f", game.bestScore);
        DrawText(bestText, cx - 55, cy + 26, 20, Fade(pal.uiAccent, 0.8f));

        // Statistics section (if didn't qualify)
        int controlsY = cy + 58;
        if (!game.leaderboardStats.scoreQualified && game.leaderboardCount > 0) {
            int yOffset = 60;
            
            DrawText("--- Leaderboard Stats ---", cx - 120, cy + yOffset, 18, pal.uiAccent);
            yOffset += 30;
            
            // Rank information
            if (game.leaderboardStats.rankIfQualified > 0) {
                char rankText[64];
                std::snprintf(rankText, sizeof(rankText), "Would rank: #%d", 
                             game.leaderboardStats.rankIfQualified);
                DrawText(rankText, cx - 90, cy + yOffset, 16, pal.uiText);
                yOffset += 22;
            }
            
            // Points needed for 10th place
            if (game.leaderboardStats.scoreDifference10th > 0.0f) {
                char pointsText[96];
                std::snprintf(pointsText, sizeof(pointsText), 
                             "Need %.0f more points for top 10", 
                             game.leaderboardStats.scoreDifference10th);
                DrawText(pointsText, cx - 130, cy + yOffset, 16, Fade(pal.uiText, 0.9f));
                yOffset += 22;
            }
            
            // Percentage of 10th place
            if (game.leaderboardStats.scorePercent10th > 0.0f) {
                char percentText[96];
                std::snprintf(percentText, sizeof(percentText), 
                             "%.1f%% of 10th place score", 
                             game.leaderboardStats.scorePercent10th);
                DrawText(percentText, cx - 100, cy + yOffset, 15, Fade(pal.uiText, 0.8f));
                yOffset += 22;
            }
            
            // Points needed for 1st place
            if (game.leaderboardStats.scoreDifference1st > 0.0f) {
                char firstText[96];
                std::snprintf(firstText, sizeof(firstText), 
                             "Need %.0f more for 1st place", 
                             game.leaderboardStats.scoreDifference1st);
                DrawText(firstText, cx - 110, cy + yOffset, 15, Fade(pal.uiAccent, 0.7f));
                yOffset += 22;
            }
            
            // Time estimate (if meaningful)
            if (game.leaderboardStats.timeDifference10th > 5.0f) {
                char timeText[96];
                std::snprintf(timeText, sizeof(timeText), 
                             "~%.1fs more needed (estimate)", 
                             game.leaderboardStats.timeDifference10th);
                DrawText(timeText, cx - 120, cy + yOffset, 14, Fade(pal.uiText, 0.7f));
                yOffset += 20;
            }
            
            // Motivational message
            yOffset += 10;
            DrawText("Keep trying!", cx - 60, cy + yOffset, 16, Fade(pal.uiAccent, 0.8f));
            controlsY = cy + yOffset + 40;
        } else if (game.leaderboardCount == 0) {
            // Empty leaderboard - encouraging message
            DrawText("First run! Set the bar!", cx - 110, cy + 60, 18, pal.uiAccent);
            controlsY = cy + 100;
        }

        DrawText("R  Retry same seed", cx - 110, controlsY, 18, pal.uiText);
        DrawText("N  New run",         cx - 110, controlsY + 26, 18, pal.uiText);
        DrawText("ESC  Main menu",     cx - 110, controlsY + 52, 18, Fade(pal.uiText, 0.7f));

    } else {
        // ---- Playing: HUD ----
        DrawRectangleRounded(Rectangle{10.0f, 10.0f, 360.0f, 140.0f}, 0.08f, 8, pal.uiPanel);
        DrawText("SkyRoads", 20, 18, 22, pal.uiText);
        char levelText[32];
        std::snprintf(levelText, sizeof(levelText), "Level %d", game.currentLevelIndex);
        DrawText(levelText, 20, 40, 16, Fade(pal.uiAccent, 0.8f));
        DrawFPS(20, 60);

        char speedText[64];
        std::snprintf(speedText, sizeof(speedText), "Speed: %.1f u/s", planarSpeed);
        DrawText(speedText, 20, 84, 16, pal.uiText);

        char scoreText[96];
        std::snprintf(scoreText, sizeof(scoreText), "Score: %.0f   Best: %.0f", GetCurrentScore(game), game.bestScore);
        DrawText(scoreText, 20, 108, 18, pal.uiAccent);

        char multText[64];
        std::snprintf(multText, sizeof(multText), "x%.2f", game.scoreMultiplier);
        DrawText(multText, 300, 108, 18, pal.uiAccent);

        DrawText("ESC pause  TAB palette  B bloom  O screenshot", 20, 134, 14, Fade(pal.uiText, 0.6f));

        // Controls mini-HUD (top-right) with key icons.
        const bool leftHeld = IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT);
        const bool rightHeld = IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT);
        const bool jumpHeld = IsKeyDown(KEY_SPACE);
        const bool dashHeld = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
        const bool pauseHeld = IsKeyDown(KEY_ESCAPE);
        const int hudW = 200;
        const int hudX = cfg::kScreenWidth - hudW - 12;
        const int hudY = 10;
        DrawRectangleRounded(Rectangle{static_cast<float>(hudX), static_cast<float>(hudY),
                             static_cast<float>(hudW), 120.0f}, 0.08f, 8, pal.uiPanel);
        
        if (g_texturesLoaded) {
            // Tilemap: 34 cols × 24 rows, tiles are 16×16 with 1px spacing (17×17 total)
            const int tileSize = 16;
            const int iconScale = 2;  // 2x scale for visibility
            const int iconSize = tileSize * iconScale;
            
            // Helper to get tile source rectangle
            auto GetTileRect = [](int row, int col, bool pressed) -> Rectangle {
                const int tileSize = 16;
                const int tileSpacing = 17;  // 16px tile + 1px spacing
                const int yOffset = pressed ? 6 : 0;  // Pressed variants are 6 rows down
                return Rectangle{
                    static_cast<float>(col * tileSpacing),
                    static_cast<float>((row + yOffset) * tileSpacing),
                    static_cast<float>(tileSize),
                    static_cast<float>(tileSize)
                };
            };
            
            // A key: row 5, col 17
            DrawTextureRec(g_inputTilemap, GetTileRect(5, 17, leftHeld),
                          Vector2{static_cast<float>(hudX + 10), static_cast<float>(hudY + 10)},
                          leftHeld ? pal.uiAccent : WHITE);
            DrawText("Left", hudX + 10 + iconSize + 6, hudY + 12, 14, leftHeld ? pal.uiAccent : pal.uiText);
            
            // D key: row 5, col 19
            DrawTextureRec(g_inputTilemap, GetTileRect(5, 19, rightHeld),
                          Vector2{static_cast<float>(hudX + 10), static_cast<float>(hudY + 30)},
                          rightHeld ? pal.uiAccent : WHITE);
            DrawText("Right", hudX + 10 + iconSize + 6, hudY + 32, 14, rightHeld ? pal.uiAccent : pal.uiText);
            
            // Space key: row 6, col 12 (estimated, wide key)
            DrawTextureRec(g_inputTilemap, GetTileRect(6, 12, jumpHeld),
                          Vector2{static_cast<float>(hudX + 10), static_cast<float>(hudY + 50)},
                          jumpHeld ? pal.uiAccent : WHITE);
            DrawText("Jump", hudX + 10 + iconSize + 6, hudY + 52, 14, jumpHeld ? pal.uiAccent : pal.uiText);
            
            // Shift key: row 6, col 20 (estimated)
            DrawTextureRec(g_inputTilemap, GetTileRect(6, 20, dashHeld),
                          Vector2{static_cast<float>(hudX + 10), static_cast<float>(hudY + 70)},
                          dashHeld ? pal.uiAccent : WHITE);
            DrawText("Dash", hudX + 10 + iconSize + 6, hudY + 72, 14, dashHeld ? pal.uiAccent : pal.uiText);
            
            // P/ESC key: row 5, col 15 (estimated for P key, or use ESC if available)
            // For now, we'll use text for P/ESC since it's not a standard key icon
            DrawText("P/ESC", hudX + 10, hudY + 90, 12, pauseHeld ? pal.uiAccent : pal.uiText);
            DrawText("Pause", hudX + 10 + iconSize + 6, hudY + 92, 14, pauseHeld ? pal.uiAccent : pal.uiText);
        } else {
            // Fallback to text if textures not loaded
            DrawText("A/<- Left",   hudX + 10, hudY + 10,  14, leftHeld  ? pal.uiAccent : pal.uiText);
            DrawText("D/-> Right",  hudX + 10, hudY + 30,  14, rightHeld ? pal.uiAccent : pal.uiText);
            DrawText("SPACE Jump",  hudX + 10, hudY + 50,  14, jumpHeld  ? pal.uiAccent : pal.uiText);
            DrawText("SHIFT Dash",  hudX + 10, hudY + 70,  14, dashHeld  ? pal.uiAccent : pal.uiText);
            DrawText("P/ESC Pause", hudX + 10, hudY + 90,  14, pauseHeld ? pal.uiAccent : pal.uiText);
        }
    }

    // ---- Perf overlay (bottom-left, always visible) ----
    {
        const int perfY = cfg::kScreenHeight - 52;
        DrawRectangleRounded(Rectangle{10.0f, static_cast<float>(perfY), 310.0f, 42.0f}, 0.08f, 8, pal.uiPanel);
        char perfBuf[128];
        std::snprintf(perfBuf, sizeof(perfBuf), "Update: %.2f ms  Render: %.2f ms", game.updateMs, game.renderMs);
        DrawText(perfBuf, 20, perfY + 6, 13, pal.uiText);
        std::snprintf(perfBuf, sizeof(perfBuf), "Allocs: %d", game.updateAllocCount);
        DrawText(perfBuf, 20, perfY + 24, 13, game.updateAllocCount > 0 ? pal.uiAccent : pal.uiText);
    }

    // ---- Screenshot notification (top-center, when active) ----
    if (game.screenshotNotificationTimer > 0.0f) {
        const float alpha = Clamp01(game.screenshotNotificationTimer / 0.5f);  // Fade out in last 0.5s
        const int notifY = 60;
        const int notifW = 400;
        const int notifX = (cfg::kScreenWidth - notifW) / 2;
        DrawRectangleRounded(Rectangle{static_cast<float>(notifX), static_cast<float>(notifY),
                             static_cast<float>(notifW), 50.0f}, 0.1f, 8, Fade(pal.uiPanel, alpha * 0.95f));
        DrawText("Screenshot saved!", notifX + 20, notifY + 8, 20, Fade(pal.uiAccent, alpha));
        DrawText(game.screenshotPath, notifX + 20, notifY + 30, 14, Fade(pal.uiText, alpha * 0.8f));
    }

    EndDrawing();
}
