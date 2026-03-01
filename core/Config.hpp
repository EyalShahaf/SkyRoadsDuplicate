#pragma once

namespace cfg {
constexpr int kScreenWidth = 1280;
constexpr int kScreenHeight = 720;
constexpr int kPaletteCount = 3;
constexpr int kLandingParticlePoolSize = 64;

constexpr float kFixedDt = 1.0f / 120.0f;
constexpr float kMaxFrameTime = 0.25f;

constexpr float kForwardSpeed = 18.0f;
constexpr float kStrafeSpeed = 9.0f;
constexpr float kStrafeAccel = 28.0f;
constexpr float kAirControlFactor = 0.45f;
constexpr float kGravity = -34.0f;
constexpr float kJumpForce = 11.0f;
constexpr float kJumpBufferTime = 0.1f;
constexpr float kCoyoteTime = 0.1f;
constexpr float kDashSpeedBoost = 14.0f;
constexpr float kDashDuration = 0.14f;
constexpr float kDashCooldown = 0.5f;

// Throttle system
constexpr float kThrottleMin = 0.0f;
constexpr float kThrottleMax = 1.0f;
constexpr float kThrottleChangeRate =
    2.5f; // How fast throttle changes per second
constexpr float kThrottleSpeedMin = 12.0f; // Minimum speed at throttle 0
constexpr float kThrottleSpeedMax = 32.0f; // Maximum speed at throttle 1

constexpr float kCameraBaseFov = 70.0f; // Increased to fill screen better
constexpr float kCameraMaxFov = 82.0f;  // Increased to fill screen better
constexpr float kCameraRollMaxDeg = 7.0f;
constexpr float kCameraRollSmoothing = 8.0f;

constexpr float kSpeedLineMinSpeed = 20.0f;
constexpr float kEdgeGlowOffset = 0.05f;
constexpr float kEdgeGlowLift = 0.03f;

constexpr int kLandingBurstCount = 14;
constexpr float kLandingParticleLife = 0.28f;
constexpr float kLandingParticleSpeedMin = 2.0f;
constexpr float kLandingParticleSpeedMax = 6.0f;
constexpr float kLandingParticleRiseSpeed = 3.0f;
constexpr float kLandingParticleDrag = 5.5f;

constexpr float kBloomOverlayAlpha = 0.18f;

// --- Visual scene dressing (render-only, no sim impact) ---
constexpr int kStarCount = 400;
constexpr float kStarFieldRadius = 120.0f;
constexpr float kStarFieldHeight = 80.0f;
constexpr float kStarFieldDepth = 200.0f;

constexpr int kCometCount = 4;
constexpr int kNebulaCount = 6;
constexpr float kFarFieldRadius = 300.0f;
constexpr float kFarFieldHeight = 150.0f;

constexpr int kDecoCubeCount = 28;
constexpr float kDecoCubeSpacing = 18.0f;
constexpr float kDecoCubeMinSize = 0.4f;
constexpr float kDecoCubeMaxSize = 1.6f;
constexpr float kDecoCubeSideOffset = 6.0f;

constexpr int kGridLateralLines = 22; // cross-hatch lines visible around player
constexpr float kGridLateralSpacing = 2.0f; // Z spacing of lateral grid lines
constexpr int kGridLongitudinalCount = 7;   // longitudinal lines across width

constexpr float kNeonEdgeWidth = 0.18f;
constexpr float kNeonEdgeHeight = 0.09f;

constexpr float kShipModelScale = 0.7f;

constexpr int kExhaustParticleCount = 48;
constexpr float kExhaustParticleLife = 0.35f;
constexpr float kExhaustSpreadX = 0.15f;
constexpr float kExhaustSpreadY = 0.1f;
constexpr float kExhaustBaseSpeed = 6.0f;

constexpr int kAmbientParticleCount = 80;
constexpr float kAmbientParticleRadius = 25.0f;
constexpr float kAmbientParticleHeight = 18.0f;

constexpr int kMountainCount = 8;
constexpr float kMountainDistance = 90.0f;
constexpr float kMountainMaxHeight = 15.0f;

constexpr float kPlayerHalfHeight = 0.5f;
constexpr float kPlayerWidth = 0.8f;
constexpr float kPlayerDepth = 1.2f;

constexpr float kPlatformWidth = 8.0f;
constexpr float kPlatformHeight = 1.0f;
constexpr float kPlatformLength = 1000.0f;
constexpr float kPlatformStartZ = 0.0f;
constexpr float kPlatformTopY = 0.0f;

constexpr float kFailKillY = -8.0f;
constexpr float kScoreDistancePerUnit = 1.0f;
constexpr float kScoreDashStylePerSecond = 35.0f;
constexpr float kScoreMultiplierMin = 1.0f;
constexpr float kScoreMultiplierMax = 2.5f;

// --- Dynamic difficulty ---
constexpr float kDifficultyRampRate =
    0.012f; // difficulty units per second of run time
constexpr float kDifficultyMaxCap = 1.0f; // difficulty T clamped to [0, 1]
constexpr float kDiffSpeedBonus =
    10.0f; // extra forward speed at max difficulty
constexpr float kDiffHazardProbMin =
    0.10f; // gap/narrow chunk probability at difficulty 0
constexpr float kDiffHazardProbMax =
    0.55f; // gap/narrow chunk probability at max difficulty

// --- Leaderboard ---
constexpr int kLeaderboardSize = 10;

// --- Power-up system ---
// Power-up spawn rates
constexpr float kPowerUpSpawnBaseProb = 0.08f;
constexpr float kPowerUpSpawnMaxProb = 0.12f;
constexpr float kPowerUpMinSpacing = 10.0f;
constexpr float kPowerUpSpawnHeightMin = 1.5f;
constexpr float kPowerUpSpawnHeightMax = 2.0f;

// Effect durations (seconds)
constexpr float kShieldDuration = 0.0f;  // Until consumed
constexpr float kScoreMultiplierDuration = 10.0f;
constexpr float kSpeedBoostDuration = 10.0f;
constexpr float kGhostModeDuration = 8.0f;
constexpr float kObstacleRevealDuration = 12.0f;
constexpr float kSpeedDrainDuration = 10.0f;

// Effect magnitudes
constexpr float kSpeedBoostAmount = 10.0f;
constexpr float kSpeedDrainAmount = 8.0f;
constexpr float kScoreMultiplierBoost = 1.5f;
constexpr float kObstacleRevealRange = 15.0f;
constexpr float kObstacleSurgeMultiplier = 1.5f;

// Power-up text rendering
constexpr float kPowerUpTextHeight = 0.18f;  // Text size in world units
constexpr float kPowerUpTextOffset = 0.35f;   // Height above icon
constexpr float kPowerUpTextPulseSpeed = 3.0f; // Animation speed for text pulse

// Player effect glow
constexpr float kPlayerEffectGlowWarningTime = 2.5f;  // Start blinking this many seconds before expiry
constexpr float kPlayerEffectGlowBlinkCount = 3.0f;   // Number of blinks before expiry
constexpr float kPlayerEffectGlowSize = 1.5f;         // Glow size multiplier around player

// --- Controls ---
struct KeyConfig {
  int left = 263;         // KEY_LEFT
  int leftAlt = 65;       // KEY_A
  int right = 262;        // KEY_RIGHT
  int rightAlt = 68;      // KEY_D
  int up = 265;           // KEY_UP
  int upAlt = 87;         // KEY_W
  int down = 264;         // KEY_DOWN
  int downAlt = 83;       // KEY_S
  int jump = 32;          // KEY_SPACE
  int jumpAlt = 75;       // KEY_K
  int dash = 340;         // KEY_LEFT_SHIFT
  int dashAlt = 76;       // KEY_L
  int pause = 80;         // KEY_P
  int back = 256;         // KEY_ESCAPE
  int confirm = 257;      // KEY_ENTER
  int restartSame = 82;   // KEY_R
  int restartNew = 78;    // KEY_N
  int cyclePalette = 291; // KEY_F2
  int toggleBloom = 292;  // KEY_F3
  int screenshot = 301;   // KEY_F12
  int backspace = 259;    // KEY_BACKSPACE
};

extern KeyConfig keys;

} // namespace cfg
