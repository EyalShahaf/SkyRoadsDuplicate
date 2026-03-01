#include "render/Render.hpp"

#include <cmath>
#include <cstdio>

#include "core/Assets.hpp"
#include "core/Config.hpp"
#include "core/Log.hpp"
#include "game/Game.hpp"
#include "render/GateRenderer.hpp"
#include "render/HudWidgets.hpp"
#include "render/Palette.hpp"
#include "render/RenderUtils.hpp"
#include "render/SceneDressing.hpp"
#include "render/SpaceObjects.hpp"
#include "rlgl.h"
#include "sim/Level.hpp"
#include "sim/PowerUp.hpp"

namespace render {

bool g_texturesLoaded = false;
Texture2D g_planetTextures[10] = {};
bool g_backgroundTexturesLoaded = false;
Texture2D g_backgroundTextures[4] = {};

} // namespace render

namespace {

// ─── Ship model
// ───────────────────────────────────────────────────────────────

bool g_shipLoaded = false;
Model g_shipModel = {};

// ─── Texture refs not needed by sub-files ────────────────────────────────────
Texture2D g_inputTilemap = {};
Texture2D g_hudReferenceImage = {};

// ─── Engine exhaust (render-side preallocated pool) ──────────────────────────

struct ExhaustParticle {
  bool active = false;
  Vector3 pos = {};
  Vector3 vel = {};
  float life = 0.0f;
  float maxLife = 0.0f;
};

ExhaustParticle g_exhaust[cfg::kExhaustParticleCount] = {};
int g_exhaustNextIdx = 0;

void SpawnExhaustParticle(const Vector3 &origin, float speedBoost,
                          uint32_t &rng) {
  auto &p = g_exhaust[g_exhaustNextIdx];
  g_exhaustNextIdx = (g_exhaustNextIdx + 1) % cfg::kExhaustParticleCount;
  p.active = true;
  p.pos = origin;
  const float sx =
      (render::HashFloat01(rng) - 0.5f) * 2.0f * cfg::kExhaustSpreadX;
  rng = render::Hash(rng + 1);
  const float sy =
      (render::HashFloat01(rng) - 0.5f) * 2.0f * cfg::kExhaustSpreadY;
  rng = render::Hash(rng + 1);
  p.vel = {sx, sy, -(cfg::kExhaustBaseSpeed + speedBoost * 0.4f)};
  p.life = cfg::kExhaustParticleLife * (0.6f + 0.8f * render::HashFloat01(rng));
  rng = render::Hash(rng + 1);
  p.maxLife = p.life;
}

void UpdateExhaustParticles(float dt) {
  for (auto &p : g_exhaust) {
    if (!p.active)
      continue;
    p.life -= dt;
    if (p.life <= 0.0f) {
      p.active = false;
      continue;
    }
    p.pos.x += p.vel.x * dt;
    p.pos.y += p.vel.y * dt;
    p.pos.z += p.vel.z * dt;
    p.vel.x *= (1.0f - 3.0f * dt);
    p.vel.y *= (1.0f - 3.0f * dt);
  }
}

// ─── Camera
// ───────────────────────────────────────────────────────────────────

void UpdateFollowCamera(Game &game, const Vector3 &playerPos, float renderDt) {
  if (game.runOver)
    return;

  const Vector3 desiredTarget = {playerPos.x, playerPos.y + 0.3f,
                                 playerPos.z + 8.0f};
  const Vector3 desiredPos = {playerPos.x, playerPos.y + 1.2f,
                              playerPos.z - 6.0f};

  const Vector3 clampedTarget = {
      desiredTarget.x, (desiredTarget.y < 0.3f) ? 0.3f : desiredTarget.y,
      desiredTarget.z};
  const Vector3 clampedPos = {
      desiredPos.x, (desiredPos.y < 1.0f) ? 1.0f : desiredPos.y, desiredPos.z};

  const float sf = 1.0f - std::exp(-6.0f * renderDt);
  game.cameraTarget = render::LerpVec3(game.cameraTarget, clampedTarget, sf);
  game.cameraPosition = render::LerpVec3(game.cameraPosition, clampedPos, sf);

  const float desiredRoll =
      -render::Clamp01(std::fabs(game.player.velocity.x) / cfg::kStrafeSpeed) *
      ((game.player.velocity.x >= 0.0f) ? cfg::kCameraRollMaxDeg
                                        : -cfg::kCameraRollMaxDeg);
  const float rollLerp = 1.0f - std::exp(-cfg::kCameraRollSmoothing * renderDt);
  game.cameraRollDeg += (desiredRoll - game.cameraRollDeg) * rollLerp;
  const float rollRad = game.cameraRollDeg * DEG2RAD;

  game.camera.up = {std::sin(rollRad), std::cos(rollRad), 0.0f};
  game.camera.target = game.cameraTarget;
  game.camera.position = game.cameraPosition;
}

Color GetDecoCubeColor(const LevelPalette &p, int idx) {
  if (idx == 0)
    return p.decoCube1;
  if (idx == 1)
    return p.decoCube2;
  return p.decoCube3;
}

} // anonymous namespace

// ─── Public API
// ───────────────────────────────────────────────────────────────

// Kept for backward compatibility (declared in Render.hpp).
void RegenerateSpaceObjects(uint32_t seed) {
  render::RegenerateSpaceObjects(seed);
}

void InitRenderer() {
  if (!g_shipLoaded) {
    g_shipModel = LoadModel(assets::Path("models/craft_speederA.obj"));
    g_shipLoaded = true;
  }
  if (!render::g_texturesLoaded) {
    g_inputTilemap = LoadTexture(
        assets::Path("kenney_input-prompts-pixel-16/Tilemap/tilemap.png"));
    SetTextureFilter(g_inputTilemap, TEXTURE_FILTER_POINT);

    for (int i = 0; i < 10; ++i) {
      char path[64];
      std::snprintf(path, sizeof(path), "kenney_planets/Planets/planet%02d.png",
                    i);
      render::g_planetTextures[i] = LoadTexture(assets::Path(path));
      SetTextureFilter(render::g_planetTextures[i], TEXTURE_FILTER_BILINEAR);
    }

    if (assets::Exists("Gemini_Generated_Image_m497ykm497ykm497.png")) {
      g_hudReferenceImage = LoadTexture(
          assets::Path("Gemini_Generated_Image_m497ykm497ykm497.png"));
      SetTextureFilter(g_hudReferenceImage, TEXTURE_FILTER_POINT);
    }

    const char *bgPaths[] = {
        "background_assets/"
        "4k-double-parallax-retro-abstract-footage-271209015_iconl.webp",
        "background_assets/"
        "PBhKWrGRgaxKia_FMnR4ZEg0CQ3WAxSz1QOc-"
        "UPEfiMrffQD0uR1EO1zuDrDYb9Tbw3mqQwfMHonjoYB_"
        "kEfrA7M3tkCfxnyAqiBp0pD1p0.jpeg",
        "background_assets/"
        "kvj7sm6sLNDy-1PHO2VAShRFDEWruuQUBM5lhrVzuTgfuBfA_"
        "Taw0gdcC99A07b5Q4Mye6_FYcIvdZliziHBTgX_F-aLWgL2JNG5iDNWQ8E.jpeg",
        "background_assets/"
        "IrPw9NZSWxhr5-Cvrk-BCDe_Bvziekmm9kxtUE9BgId2_"
        "TUnibgX3nS91MPn9URl0ZcVcFyoCvLXVH7gZfyaEVNJunx_b7JSTNBCFhVHRLw.jpeg"};
    for (int i = 0; i < 4; ++i) {
      if (assets::Exists(bgPaths[i])) {
        render::g_backgroundTextures[i] = LoadTexture(assets::Path(bgPaths[i]));
        SetTextureFilter(render::g_backgroundTextures[i],
                         TEXTURE_FILTER_BILINEAR);
        render::g_backgroundTexturesLoaded = true;
      }
    }

    render::g_texturesLoaded = true;
  }
}

void CleanupRenderer() {
  if (g_shipLoaded) {
    UnloadModel(g_shipModel);
    g_shipLoaded = false;
  }
  if (render::g_texturesLoaded) {
    UnloadTexture(g_inputTilemap);
    for (int i = 0; i < 10; ++i)
      UnloadTexture(render::g_planetTextures[i]);
    if (g_hudReferenceImage.id != 0)
      UnloadTexture(g_hudReferenceImage);
    if (render::g_backgroundTexturesLoaded) {
      for (int i = 0; i < 4; ++i) {
        if (render::g_backgroundTextures[i].id != 0)
          UnloadTexture(render::g_backgroundTextures[i]);
      }
      render::g_backgroundTexturesLoaded = false;
    }
    render::g_texturesLoaded = false;
  }
}

// ─── Main render frame
// ────────────────────────────────────────────────────────

void RenderFrame(Game &game, float alpha, float renderDt) {
  // One-time init of static scene dressing
  render::InitSceneDressing();

  const LevelPalette &pal = GetPalette(game.paletteIndex);
  const Vector3 playerRenderPos = render::InterpolatePosition(game, alpha);
  UpdateFollowCamera(game, playerRenderPos, renderDt);

  const float planarSpeed =
      std::sqrt(game.player.velocity.x * game.player.velocity.x +
                game.player.velocity.z * game.player.velocity.z);
  const float speedT = render::Clamp01((planarSpeed - cfg::kForwardSpeed) /
                                       cfg::kDashSpeedBoost);
  game.camera.fovy =
      cfg::kCameraBaseFov + (cfg::kCameraMaxFov - cfg::kCameraBaseFov) * speedT;

  const float simTime = static_cast<float>(game.simTicks) * cfg::kFixedDt;

  // Re-seed space objects when run seed changes
  static uint32_t lastRunSeed = 0u;
  if (game.runSeed != lastRunSeed) {
    render::RegenerateSpaceObjects(game.runSeed);
    lastRunSeed = game.runSeed;
  }
  render::UpdateSpaceObjects(renderDt, playerRenderPos);

  BeginDrawing();
  ClearBackground(BLACK);

  // Scrolling background
  static float backgroundScroll = 0.0f;
  backgroundScroll += renderDt * 20.0f;

  int bgTextureIndex = (game.screen == GameScreen::Playing &&
                        game.currentStage >= 1 && game.currentStage <= 10)
                           ? (game.currentStage - 1) % 4
                           : static_cast<int>(game.screen) % 4;

  const int hudStartY = (game.screen == GameScreen::Playing)
                            ? (cfg::kScreenHeight * 2 / 3)
                            : cfg::kScreenHeight;
  const int viewportHeight = hudStartY;
  const int bgHeight = (game.screen == GameScreen::Playing)
                           ? viewportHeight
                           : cfg::kScreenHeight;

  render::DrawBackgroundWithGrid(bgTextureIndex, backgroundScroll, pal,
                                 cfg::kScreenWidth, bgHeight, 0.85f);
  DrawRectangleGradientV(0, 0, cfg::kScreenWidth, bgHeight, Fade(BLACK, 0.0f),
                         Fade(BLACK, 0.3f));

  // Sky gradient
  const bool isPlaying = (game.screen == GameScreen::Playing);
  const Color skyTop = (isPlaying && game.currentStage >= 1)
                           ? render::GetStageBackgroundTop(game.currentStage)
                           : pal.skyTop;
  const Color skyBottom =
      (isPlaying && game.currentStage >= 1)
          ? render::GetStageBackgroundBottom(game.currentStage)
          : pal.skyBottom;
  DrawRectangleGradientV(0, 0, cfg::kScreenWidth, viewportHeight,
                         Fade(skyTop, 0.4f), Fade(skyBottom, 0.5f));
  DrawRectangleGradientV(0, viewportHeight / 2, cfg::kScreenWidth,
                         viewportHeight / 2, Fade(BLACK, 0.0f),
                         Fade(pal.voidTint, 0.3f));

  // Scissor 3D to viewport (exclude cockpit HUD)
  // Both rlViewport and rlScissor use OpenGL coordinates (bottom-left origin)
  // To position at top of screen: y = screenHeight - viewportHeight
  if (game.screen == GameScreen::Playing) {
    const int viewportY = cfg::kScreenHeight - viewportHeight;
    rlViewport(0, viewportY, cfg::kScreenWidth, viewportHeight);
    rlEnableScissorTest();
    rlScissor(0, viewportY, cfg::kScreenWidth, viewportHeight);
  } else {
    rlViewport(0, 0, cfg::kScreenWidth, cfg::kScreenHeight);
  }

  BeginMode3D(game.camera);
  
  // Re-apply viewport and scissor after BeginMode3D in case it was reset
  if (game.screen == GameScreen::Playing) {
    const int viewportY = cfg::kScreenHeight - viewportHeight;
    rlViewport(0, viewportY, cfg::kScreenWidth, viewportHeight);
    rlScissor(0, viewportY, cfg::kScreenWidth, viewportHeight);
  }

  // Space environment
  render::RenderSpaceObjects(game.camera, pal, simTime);

  // Mountains
  render::RenderMountains(pal, playerRenderPos);

  // ── Level geometry ────────────────────────────────────────────────────────
  const Level *lv = game.level;
  const float guideY =
      cfg::kPlatformTopY + 0.02f; // fallback Y for speed streaks

  if (lv) {
    for (int si = 0; si < lv->segmentCount; ++si) {
      const auto &seg = lv->segments[si];
      const float segMidZ = seg.startZ + seg.length * 0.5f;
      if (std::fabs(segMidZ - playerRenderPos.z) > 80.0f)
        continue;

      const float segEndZ = seg.startZ + seg.length;
      const float halfW = seg.width * 0.5f;
      // Safety check: if heightScale is negative (unassigned), use default value
      const float effectiveHeightScale = (seg.heightScale < 0.0f) ? 1.0f : seg.heightScale;
      const float visualH = cfg::kPlatformHeight * effectiveHeightScale;

      // Safety check: clamp colorTint to valid range (0-2)
      const int safeColorTint = (seg.colorTint < 0) ? 0 : (seg.colorTint > 2 ? 2 : seg.colorTint);
      Color platformSideCol =
          render::ApplyColorTint(pal.platformSide, safeColorTint);
      Color platformTopCol =
          render::ApplyColorTint(pal.platformTop, safeColorTint);
      Color platformWireCol =
          render::ApplyColorTint(pal.platformWire, safeColorTint);
      Color neonEdgeCol = pal.neonEdge;
      Color neonGlowCol = pal.neonEdgeGlow;
      float wireAlpha = 0.5f;
      float glowIntensity = 0.15f;
      bool drawGrid = true;

      // Safety check: clamp variantIndex to valid range (0-7)
      const int safeVariantIndex = (seg.variantIndex < 0) ? 0 : (seg.variantIndex > 7 ? 7 : seg.variantIndex);
      switch (safeVariantIndex) {
      case 1:
        wireAlpha = 0.3f;
        break;
      case 2:
        wireAlpha = 0.7f;
        break;
      case 3:
        platformSideCol = render::ApplyColorTint(pal.platformSide, 1);
        break;
      case 4:
        platformSideCol =
            Color{static_cast<unsigned char>(platformSideCol.r * 0.8f),
                  static_cast<unsigned char>(platformSideCol.g * 0.8f),
                  static_cast<unsigned char>(platformSideCol.b * 0.8f), 255};
        break;
      case 5:
        glowIntensity = 0.3f;
        neonEdgeCol = Color{
            static_cast<unsigned char>(std::min(255, pal.neonEdge.r + 40)),
            static_cast<unsigned char>(std::min(255, pal.neonEdge.g + 40)),
            static_cast<unsigned char>(std::min(255, pal.neonEdge.b + 40)),
            255};
        break;
      case 6:
        wireAlpha = 0.0f;
        drawGrid = false;
        platformSideCol =
            Color{static_cast<unsigned char>(platformSideCol.r * 0.7f),
                  static_cast<unsigned char>(platformSideCol.g * 0.7f),
                  static_cast<unsigned char>(platformSideCol.b * 0.7f), 255};
        break;
      default:
        break;
      }

      DrawCubeV(Vector3{seg.xOffset, seg.topY - visualH * 0.5f, segMidZ},
                Vector3{seg.width, visualH, seg.length}, platformSideCol);
      DrawCubeV(Vector3{seg.xOffset, seg.topY - 0.01f, segMidZ},
                Vector3{seg.width, 0.02f, seg.length}, platformTopCol);

      if (wireAlpha > 0.0f) {
        DrawCubeWiresV(Vector3{seg.xOffset, seg.topY - visualH * 0.5f, segMidZ},
                       Vector3{seg.width, visualH, seg.length},
                       Fade(platformWireCol, wireAlpha));
      }

      const float leftEdge = seg.xOffset - halfW;
      const float rightEdge = seg.xOffset + halfW;
      const float edgeY = seg.topY + cfg::kNeonEdgeHeight * 0.5f;
      DrawCubeV(Vector3{leftEdge, edgeY, segMidZ},
                Vector3{cfg::kNeonEdgeWidth, cfg::kNeonEdgeHeight, seg.length},
                neonEdgeCol);
      DrawCubeV(Vector3{rightEdge, edgeY, segMidZ},
                Vector3{cfg::kNeonEdgeWidth, cfg::kNeonEdgeHeight, seg.length},
                neonEdgeCol);
      DrawCubeV(Vector3{leftEdge, edgeY, segMidZ},
                Vector3{cfg::kNeonEdgeWidth * 3.0f, cfg::kNeonEdgeHeight * 2.5f,
                        seg.length},
                Fade(neonGlowCol, glowIntensity));
      DrawCubeV(Vector3{rightEdge, edgeY, segMidZ},
                Vector3{cfg::kNeonEdgeWidth * 3.0f, cfg::kNeonEdgeHeight * 2.5f,
                        seg.length},
                Fade(neonGlowCol, glowIntensity));

      // Striped variant
      if (safeVariantIndex == 7) {
        constexpr int stripeCount = 8;
        for (int st = 0; st < stripeCount; ++st) {
          const float t =
              static_cast<float>(st) / static_cast<float>(stripeCount);
          const Color sc = (st % 2 == 0)
                               ? platformTopCol
                               : render::ApplyColorTint(platformTopCol, 1);
          DrawCubeV(Vector3{seg.xOffset, seg.topY + 0.01f,
                            seg.startZ + t * seg.length},
                    Vector3{seg.width, 0.015f, seg.length / stripeCount},
                    Fade(sc, 0.6f));
        }
      }

      // Grid lines
      if (drawGrid) {
        const float sGuideY = seg.topY + 0.02f;
        for (int gi = 0; gi < cfg::kGridLongitudinalCount; ++gi) {
          const float t = static_cast<float>(gi) /
                          static_cast<float>(cfg::kGridLongitudinalCount - 1);
          const float gx = seg.xOffset - halfW + t * seg.width;
          DrawLine3D(Vector3{gx, sGuideY, seg.startZ},
                     Vector3{gx, sGuideY, segEndZ}, Fade(pal.gridLine, 0.3f));
        }
        if (std::fabs(segMidZ - playerRenderPos.z) < 30.0f) {
          const float latPhase =
              std::fmod(playerRenderPos.z, cfg::kGridLateralSpacing);
          for (int li = -2; li < 14; ++li) {
            const float lz = playerRenderPos.z - 6.0f +
                             static_cast<float>(li) * cfg::kGridLateralSpacing -
                             latPhase;
            if (lz < seg.startZ || lz > segEndZ)
              continue;
            DrawLine3D(Vector3{seg.xOffset - halfW, sGuideY, lz},
                       Vector3{seg.xOffset + halfW, sGuideY, lz},
                       Fade(pal.gridLine, 0.2f));
          }
        }
      }
    }

    // Obstacles
    for (int oi = 0; oi < lv->obstacleCount; ++oi) {
      const auto &ob = lv->obstacles[oi];
      if (std::fabs(ob.z - playerRenderPos.z) > 60.0f)
        continue;

      // Safety checks: clamp colorIndex to valid range (0-2), rotation to valid range
      const int safeColorIndex = (ob.colorIndex < 0) ? 0 : (ob.colorIndex > 2 ? 2 : ob.colorIndex);
      const float safeRotation = (ob.rotation < -360.0f) ? 0.0f : ob.rotation;
      // Safety check: default to Cube if shape is Unset
      const ObstacleShape safeShape = (ob.shape == ObstacleShape::Unset) ? ObstacleShape::Cube : ob.shape;
      const Color obCol = GetDecoCubeColor(pal, safeColorIndex);
      const Vector3 obSize = {ob.sizeX, ob.sizeY, ob.sizeZ};

      rlPushMatrix();
      rlTranslatef(ob.x, ob.y + ob.sizeY * 0.5f, ob.z);
      rlRotatef(safeRotation, 0.0f, 1.0f, 0.0f);

      const Vector3 localCenter = {0.0f, 0.0f, 0.0f};

      switch (safeShape) {
      case ObstacleShape::Cube:
        DrawCubeV(localCenter, obSize, Fade(obCol, 0.4f));
        DrawCubeWiresV(localCenter, obSize, obCol);
        break;
      case ObstacleShape::Cylinder:
        DrawCubeV(localCenter, obSize, Fade(obCol, 0.4f));
        DrawCubeV({0.0f, ob.sizeY * 0.5f - ob.sizeX * 0.3f, 0.0f},
                  {ob.sizeX * 0.9f, ob.sizeX * 0.3f, ob.sizeZ * 0.9f},
                  Fade(obCol, 0.5f));
        DrawCubeV({0.0f, -ob.sizeY * 0.5f + ob.sizeX * 0.3f, 0.0f},
                  {ob.sizeX * 0.9f, ob.sizeX * 0.3f, ob.sizeZ * 0.9f},
                  Fade(obCol, 0.5f));
        DrawCubeWiresV(localCenter, obSize, obCol);
        break;
      case ObstacleShape::Pyramid: {
        const float bH = ob.sizeY * 0.7f;
        DrawCubeV({0.0f, -ob.sizeY * 0.5f + bH * 0.5f, 0.0f},
                  {ob.sizeX, bH, ob.sizeZ}, Fade(obCol, 0.4f));
        DrawCubeV({0.0f, -ob.sizeY * 0.5f + bH + ob.sizeY * 0.15f, 0.0f},
                  {ob.sizeX * 0.5f, ob.sizeY * 0.3f, ob.sizeX * 0.5f},
                  Fade(obCol, 0.5f));
        DrawCubeWiresV({0.0f, -ob.sizeY * 0.5f + bH * 0.5f, 0.0f},
                       {ob.sizeX, bH, ob.sizeZ}, obCol);
        break;
      }
      case ObstacleShape::Spike: {
        const Vector3 spikeSize = {ob.sizeX * 0.6f, ob.sizeY, ob.sizeZ * 0.6f};
        DrawCubeV(localCenter, spikeSize, Fade(obCol, 0.4f));
        DrawCubeV({0.0f, ob.sizeY * 0.5f - ob.sizeX * 0.15f, 0.0f},
                  {ob.sizeX * 0.3f, ob.sizeX * 0.3f, ob.sizeX * 0.3f},
                  Fade(obCol, 0.6f));
        DrawCubeWiresV(localCenter, spikeSize, obCol);
        break;
      }
      case ObstacleShape::Wall: {
        const Vector3 wallSize = {ob.sizeX, ob.sizeY * 0.6f, ob.sizeZ};
        DrawCubeV({0.0f, -ob.sizeY * 0.2f, 0.0f}, wallSize, Fade(obCol, 0.4f));
        DrawCubeWiresV({0.0f, -ob.sizeY * 0.2f, 0.0f}, wallSize, obCol);
        break;
      }
      case ObstacleShape::Sphere: {
        const float avg = (ob.sizeX + ob.sizeY + ob.sizeZ) / 3.0f;
        const Vector3 s = {avg, avg, avg};
        DrawCubeV(localCenter, s, Fade(obCol, 0.4f));
        DrawCubeV(localCenter, {avg * 1.1f, avg * 1.1f, avg * 1.1f},
                  Fade(obCol, 0.15f));
        DrawCubeWiresV(localCenter, s, obCol);
        break;
      }
      default:
        break;
      }
      // Deco base
      DrawCubeV({0.0f, -ob.sizeY * 0.5f + 0.02f, 0.0f},
                {ob.sizeX * 1.5f, 0.01f, ob.sizeZ * 1.5f}, Fade(obCol, 0.15f));

      rlPopMatrix();
    }

    // Power-ups
    for (int pi = 0; pi < lv->powerUpCount; ++pi) {
      const auto &pu = lv->powerUps[pi];
      if (!pu.active) continue;
      if (std::fabs(pu.z - playerRenderPos.z) > 60.0f) continue;
      
      // Find which segment this power-up is on to get ground level
      float groundY = pu.y;
      for (int si = 0; si < lv->segmentCount; ++si) {
        const auto &seg = lv->segments[si];
        if (pu.z >= seg.startZ && pu.z <= seg.startZ + seg.length) {
          if (std::abs(pu.x - seg.xOffset) < seg.width * 0.5f) {
            groundY = seg.topY;
            break;
          }
        }
      }
      
      // Stationary position on ground (no bobbing)
      const float renderY = groundY + 0.2f;  // Small offset above ground
      const float rotSpeed = 30.0f;  // Slower rotation (degrees per second)
      const float currentRot = pu.rotation + simTime * rotSpeed;
      
      // Scale pulsing animation (breathing effect)
      const float scalePulse = 1.0f + 0.15f * std::sin(simTime * 2.5f);
      
      // Determine distinct colors and shapes based on type
      Color puColor;
      Color glowColor;
      Color ringColor;
      float iconSize = 0.6f;
      bool useSphere = false;
      bool usePyramid = false;
      bool useCylinder = false;
      
      switch (pu.type) {
        case PowerUpType::Shield:
          puColor = Color{100, 220, 255, 255};  // Bright cyan
          glowColor = Color{150, 240, 255, 255};
          ringColor = Color{200, 250, 255, 255};  // Very bright for rings
          iconSize = 0.7f;
          useSphere = true;
          break;
        case PowerUpType::ScoreMultiplier:
          puColor = Color{100, 255, 180, 255};  // Bright green
          glowColor = Color{150, 255, 220, 255};
          ringColor = Color{200, 255, 240, 255};
          iconSize = 0.65f;
          useCylinder = true;
          break;
        case PowerUpType::SpeedBoostShield:
          puColor = Color{220, 150, 255, 255};  // Bright purple
          glowColor = Color{240, 180, 255, 255};
          ringColor = Color{250, 220, 255, 255};
          iconSize = 0.75f;
          usePyramid = true;
          break;
        case PowerUpType::SpeedBoostGhost:
          puColor = Color{255, 220, 100, 255};  // Bright orange
          glowColor = Color{255, 240, 150, 255};
          ringColor = Color{255, 250, 200, 255};
          iconSize = 0.7f;
          useSphere = true;
          break;
        case PowerUpType::ObstacleReveal:
          puColor = Color{255, 255, 120, 255};  // Bright yellow
          glowColor = Color{255, 255, 200, 255};
          ringColor = Color{255, 255, 240, 255};
          iconSize = 0.65f;
          useCylinder = true;
          break;
        case PowerUpType::SpeedDrain:
          puColor = Color{255, 100, 100, 255};  // Bright red
          glowColor = Color{255, 180, 180, 255};
          ringColor = Color{255, 220, 220, 255};
          iconSize = 0.7f;
          usePyramid = true;
          break;
        case PowerUpType::ObstacleSurge:
          puColor = Color{255, 140, 0, 255};  // Bright orange-red
          glowColor = Color{255, 200, 100, 255};
          ringColor = Color{255, 230, 150, 255};
          iconSize = 0.75f;
          useSphere = true;
          break;
        default:
          puColor = Color{200, 200, 200, 255};
          glowColor = Color{220, 220, 220, 255};
          ringColor = Color{240, 240, 240, 255};
          break;
      }
      
      // Strong pulsing glow effect (much more intense)
      const float glowPulse = 0.7f + 0.3f * std::sin(simTime * 3.5f);
      const float outerGlowSize = iconSize * scalePulse * (2.8f + 0.5f * glowPulse);
      const float middleGlowSize = iconSize * scalePulse * (2.2f + 0.3f * glowPulse);
      const float innerGlowSize = iconSize * scalePulse * 1.6f;
      const float coreGlowSize = iconSize * scalePulse * 1.2f;
      
      rlPushMatrix();
      rlTranslatef(pu.x, renderY, pu.z);
      
      // Rotating rings (multiple layers for dramatic effect)
      const float ringRot1 = simTime * 60.0f;  // Fast rotation
      const float ringRot2 = simTime * -45.0f;  // Counter-rotation
      const float ringSize1 = iconSize * scalePulse * 1.5f;
      const float ringSize2 = iconSize * scalePulse * 1.8f;
      
      // Outer ring (largest, brightest)
      rlPushMatrix();
      rlRotatef(ringRot1, 0.0f, 1.0f, 0.0f);
      DrawCubeV({0.0f, 0.0f, 0.0f}, {ringSize2, 0.1f, ringSize2},
                ringColor);  // Full opacity ring
      DrawCubeV({0.0f, 0.0f, 0.0f}, {ringSize2 * 1.1f, 0.05f, ringSize2 * 1.1f},
                Fade(ringColor, 0.6f));
      rlPopMatrix();
      
      // Middle ring
      rlPushMatrix();
      rlRotatef(ringRot2, 0.0f, 1.0f, 0.0f);
      DrawCubeV({0.0f, 0.0f, 0.0f}, {ringSize1, 0.08f, ringSize1},
                Fade(ringColor, 0.9f));
      rlPopMatrix();
      
      // Multi-layer glow (4 layers for maximum visibility)
      // Outer glow (largest, most transparent)
      DrawCubeV({0.0f, 0.0f, 0.0f}, {outerGlowSize, outerGlowSize, outerGlowSize},
                Fade(glowColor, 0.7f * glowPulse));
      
      // Middle glow
      DrawCubeV({0.0f, 0.0f, 0.0f}, {middleGlowSize, middleGlowSize, middleGlowSize},
                Fade(glowColor, 0.8f * glowPulse));
      
      // Inner glow
      DrawCubeV({0.0f, 0.0f, 0.0f}, {innerGlowSize, innerGlowSize, innerGlowSize},
                Fade(glowColor, 0.9f * glowPulse));
      
      // Core glow (brightest)
      DrawCubeV({0.0f, 0.0f, 0.0f}, {coreGlowSize, coreGlowSize, coreGlowSize},
                glowColor);  // Full opacity
      
      // Rotate the main icon
      rlRotatef(currentRot, 0.0f, 1.0f, 0.0f);
      
      // Main icon with distinct shapes (fully opaque, no wireframes)
      const float scaledSize = iconSize * scalePulse;
      if (useSphere) {
        // Sphere shape - solid glowing sphere
        DrawCubeV({0.0f, 0.0f, 0.0f}, {scaledSize, scaledSize, scaledSize},
                  puColor);  // Full opacity
        // White highlight for emissive look
        DrawCubeV({0.0f, scaledSize * 0.3f, -scaledSize * 0.3f}, 
                  {scaledSize * 0.4f, scaledSize * 0.4f, scaledSize * 0.4f},
                  Color{255, 255, 255, 200});
      } else if (usePyramid) {
        // Pyramid shape - solid glowing pyramid
        const float baseH = scaledSize * 0.6f;
        DrawCubeV({0.0f, -scaledSize * 0.3f, 0.0f}, {scaledSize, baseH, scaledSize},
                  puColor);
        DrawCubeV({0.0f, scaledSize * 0.2f, 0.0f}, {scaledSize * 0.5f, scaledSize * 0.4f, scaledSize * 0.5f},
                  glowColor);
        // White highlight
        DrawCubeV({0.0f, scaledSize * 0.25f, -scaledSize * 0.2f},
                  {scaledSize * 0.3f, scaledSize * 0.2f, scaledSize * 0.3f},
                  Color{255, 255, 255, 180});
      } else if (useCylinder) {
        // Cylinder shape - solid glowing pill
        DrawCubeV({0.0f, 0.0f, 0.0f}, {scaledSize, scaledSize * 0.8f, scaledSize},
                  puColor);
        DrawCubeV({0.0f, scaledSize * 0.35f, 0.0f}, {scaledSize * 0.9f, scaledSize * 0.2f, scaledSize * 0.9f},
                  glowColor);
        DrawCubeV({0.0f, -scaledSize * 0.35f, 0.0f}, {scaledSize * 0.9f, scaledSize * 0.2f, scaledSize * 0.9f},
                  glowColor);
        // White highlight
        DrawCubeV({0.0f, scaledSize * 0.3f, -scaledSize * 0.3f},
                  {scaledSize * 0.5f, scaledSize * 0.15f, scaledSize * 0.5f},
                  Color{255, 255, 255, 200});
      } else {
        // Default cube - solid glowing cube
        DrawCubeV({0.0f, 0.0f, 0.0f}, {scaledSize, scaledSize, scaledSize},
                  puColor);
        // White highlight
        DrawCubeV({0.0f, scaledSize * 0.3f, -scaledSize * 0.3f},
                  {scaledSize * 0.4f, scaledSize * 0.4f, scaledSize * 0.4f},
                  Color{255, 255, 255, 200});
      }
      
      // Glowing base/pedestal on the ground
      DrawCubeV({0.0f, -renderY + groundY + 0.05f, 0.0f},
                {scaledSize * 1.2f, 0.1f, scaledSize * 1.2f},
                Fade(glowColor, 0.8f * glowPulse));
      
      // Vertical glow beam (makes them stand out even more)
      const float beamHeight = scaledSize * 0.8f;
      DrawCubeV({0.0f, renderY + beamHeight * 0.5f, 0.0f},
                {scaledSize * 0.3f, beamHeight, scaledSize * 0.3f},
                Fade(glowColor, 0.5f * glowPulse));
      
      rlPopMatrix();
      
      // Particle sparkles (simple orbiting particles)
      const int sparkleCount = 8;
      constexpr float kPi = 3.14159265359f;
      for (int s = 0; s < sparkleCount; ++s) {
        const float sparkleAngleRad = simTime * 2.0f + s * (2.0f * kPi / sparkleCount);
        const float sparkleRadius = iconSize * scalePulse * 1.3f;
        const float sparkleX = pu.x + std::cos(sparkleAngleRad) * sparkleRadius;
        const float sparkleZ = pu.z + std::sin(sparkleAngleRad) * sparkleRadius;
        const float sparkleY = renderY + 0.3f + 0.2f * std::sin(simTime * 4.0f + s);
        const float sparkleSize = 0.08f + 0.05f * std::sin(simTime * 5.0f + s);
        
        DrawCubeV({sparkleX, sparkleY, sparkleZ}, {sparkleSize, sparkleSize, sparkleSize},
                  Fade(ringColor, 0.9f));
      }
    }
    
    // Obstacle reveal visualization
    if (game.obstacleRevealActive && lv) {
      const float revealRange = cfg::kObstacleRevealRange;
      const float revealStartZ = playerRenderPos.z;
      const float revealEndZ = playerRenderPos.z + revealRange;
      
      for (int oi = 0; oi < lv->obstacleCount; ++oi) {
        const auto &ob = lv->obstacles[oi];
        if (ob.z < revealStartZ || ob.z > revealEndZ) continue;
        
        // Draw pulsing outline around revealed obstacles
        const float pulse = 0.7f + 0.3f * std::sin(simTime * 4.0f);
        const Color revealColor = Color{255, 255, 100, static_cast<unsigned char>(255 * pulse)};
        const Vector3 obSize = {ob.sizeX * 1.2f, ob.sizeY * 1.2f, ob.sizeZ * 1.2f};
        
        DrawCubeWiresV({ob.x, ob.y + ob.sizeY * 0.5f, ob.z}, obSize, revealColor);
        // Add glow effect
        DrawCubeV({ob.x, ob.y + ob.sizeY * 0.5f, ob.z}, 
                  {ob.sizeX * 1.3f, ob.sizeY * 1.3f, ob.sizeZ * 1.3f},
                  Fade(revealColor, 0.1f * pulse));
      }
    }

    render::RenderStartLine(*lv, playerRenderPos, pal, simTime);
    render::RenderFinishLine(*lv, playerRenderPos, pal, simTime);
  } // if (lv)

  // ── Scrolling track bands ─────────────────────────────────────────────────
  constexpr int kTrackBandCount = 12;
  constexpr float kTrackBandSpacing = 3.2f;
  const float bandPhase = std::fmod(simTime * planarSpeed, kTrackBandSpacing);
  if (lv) {
    const int nearSeg = FindSegmentUnder(
        *lv, playerRenderPos.z, playerRenderPos.x, cfg::kPlayerWidth * 0.5f);
    if (nearSeg >= 0) {
      const auto &seg = lv->segments[nearSeg];
      const float sGuideY = seg.topY + 0.02f;
      for (int i = 0; i < kTrackBandCount; ++i) {
        const float z = playerRenderPos.z - 16.0f +
                        static_cast<float>(i) * kTrackBandSpacing + bandPhase;
        if (z < seg.startZ || z > seg.startZ + seg.length)
          continue;
        const float nearT =
            1.0f - static_cast<float>(i) / static_cast<float>(kTrackBandCount);
        const float a = 0.06f + 0.18f * nearT + 0.18f * speedT;
        DrawCubeV(Vector3{seg.xOffset, sGuideY + 0.01f, z},
                  Vector3{seg.width * 0.85f, 0.015f, 0.2f},
                  Fade(pal.laneGlow, a));
      }
    }
  }

  // ── Speed streaks ─────────────────────────────────────────────────────────
  if (planarSpeed > cfg::kSpeedLineMinSpeed) {
    const float streakLen = 4.0f + 4.0f * speedT;
    const float sHalfW = cfg::kPlatformWidth * 0.5f;
    const int nStreaks = 5 + static_cast<int>(speedT * 4.0f);
    for (int s = 0; s < nStreaks; ++s) {
      const float phase =
          std::fmod(simTime * (cfg::kForwardSpeed + cfg::kDashSpeedBoost) +
                        static_cast<float>(s) * 2.1f,
                    14.0f);
      const float sz = playerRenderPos.z - 10.0f + phase;
      const float xOff = (static_cast<float>(s % 3) - 1.0f) * 0.3f;
      const float alpha = 0.12f + 0.4f * speedT;
      DrawLine3D({-sHalfW + 0.4f + xOff, guideY + 0.02f, sz},
                 {-sHalfW + 0.4f + xOff, guideY + 0.02f, sz + streakLen},
                 Fade(pal.streak, alpha));
      DrawLine3D({sHalfW - 0.4f - xOff, guideY + 0.02f, sz},
                 {sHalfW - 0.4f - xOff, guideY + 0.02f, sz + streakLen},
                 Fade(pal.streak, alpha));
    }
    for (int s = 0; s < 4; ++s) {
      const float phase = std::fmod(
          simTime * planarSpeed * 0.3f + static_cast<float>(s) * 3.5f, 16.0f);
      const float sz = playerRenderPos.z - 6.0f + phase;
      const float sideX = sHalfW + 1.0f + static_cast<float>(s) * 1.5f;
      const float y = 0.5f + static_cast<float>(s % 2) * 1.2f;
      const float len = 2.0f + 2.0f * speedT;
      DrawLine3D({-sideX, y, sz}, {-sideX, y, sz + len},
                 Fade(pal.streak, 0.08f + 0.2f * speedT));
      DrawLine3D({sideX, y, sz}, {sideX, y, sz + len},
                 Fade(pal.streak, 0.08f + 0.2f * speedT));
    }
  }

  // Decorative cubes, ambient dots
  render::RenderDecoCubes(pal, playerRenderPos, simTime);
  render::RenderAmbientDots(pal, playerRenderPos, simTime);

  // ── Player ship ───────────────────────────────────────────────────────────
  // ── Player ship shadow ────────────────────────────────────────────────────
  float groundY = cfg::kPlatformTopY;
  if (lv) {
    const int nearSeg = FindSegmentUnder(
        *lv, playerRenderPos.z, playerRenderPos.x, cfg::kPlayerWidth * 0.5f);
    if (nearSeg >= 0) {
      groundY = lv->segments[nearSeg].topY;
    }
  }

  const float altitude = playerRenderPos.y - cfg::kPlayerHalfHeight - groundY;
  const float shadowAlpha = render::Clamp01(1.0f - altitude / 3.0f);
  if (shadowAlpha > 0.01f) {
    const float shadowBaseScale = 1.0f + altitude * 0.4f;
    // Inner tighter blob
    DrawCubeV(Vector3{playerRenderPos.x, groundY + 0.015f, playerRenderPos.z},
              Vector3{cfg::kPlayerWidth * 2.5f * shadowBaseScale, 0.005f,
                      cfg::kPlayerDepth * 2.5f * shadowBaseScale},
              Fade(BLACK, 0.5f * shadowAlpha));
    // Outer softer blob
    DrawCubeV(Vector3{playerRenderPos.x, groundY + 0.01f, playerRenderPos.z},
              Vector3{cfg::kPlayerWidth * 4.5f * shadowBaseScale, 0.005f,
                      cfg::kPlayerDepth * 4.5f * shadowBaseScale},
              Fade(BLACK, 0.25f * shadowAlpha));
  }

  // ── Player effect glow (power-ups/debuffs) ──────────────────────────────────
  // Calculate glow effect based on active effects
  Color effectGlowColor = {0, 0, 0, 0};  // No glow by default
  float effectGlowAlpha = 0.0f;
  bool hasActiveEffect = false;
  bool isWarningPhase = false;  // About to expire
  
  for (int i = 0; i < game.activeEffectCount; ++i) {
    const auto &effect = game.activeEffects[i];
    if (effect.type == PowerUpType::None || effect.consumed) continue;
    
    // Skip instant effects (ObstacleSurge) - they don't have a duration
    if (effect.type == PowerUpType::ObstacleSurge) continue;
    
    // Check if effect is about to expire (warning phase)
    // Shield has timer = 0.0f but is consumed on hit, so it won't expire by time
    bool effectWarning = false;
    if (effect.timer > 0.0f && effect.timer <= cfg::kPlayerEffectGlowWarningTime) {
      effectWarning = true;
      isWarningPhase = true;
    }
    
    // Determine color based on effect type
    Color effectColor;
    bool isDebuff = IsDebuff(effect.type);
    
    if (isDebuff) {
      effectColor = Color{255, 80, 80, 255};  // Red for debuffs
    } else {
      effectColor = Color{80, 255, 80, 255};  // Green for power-ups
    }
    
    // Calculate glow intensity
    float glowIntensity = 0.0f;
    if (effectWarning) {
      // Blinking effect when about to expire
      // Create 3 blinks pattern (like traffic light)
      const float timeUntilExpiry = effect.timer;
      const float blinkDuration = cfg::kPlayerEffectGlowWarningTime / cfg::kPlayerEffectGlowBlinkCount;
      const float blinkPhase = std::fmod(timeUntilExpiry, blinkDuration) / blinkDuration;
      // Blink pattern: fade in, hold, fade out
      if (blinkPhase < 0.2f) {
        glowIntensity = blinkPhase / 0.2f;  // Fade in
      } else if (blinkPhase < 0.4f) {
        glowIntensity = 1.0f;  // Hold bright
      } else {
        glowIntensity = 1.0f - ((blinkPhase - 0.4f) / 0.6f);  // Fade out
      }
      glowIntensity *= 0.9f;  // Slightly dimmer during warning
    } else {
      // Normal glow when effect is active
      // Shield (timer = 0) shows steady glow, time-based effects pulse
      if (effect.timer <= 0.0f) {
        // Shield or other permanent effects - steady glow
        glowIntensity = 0.8f;
      } else {
        // Time-based effects - gentle pulsing
        glowIntensity = 0.7f + 0.3f * std::sin(simTime * 2.0f);
      }
    }
    
    // Use the most urgent effect (warning takes priority, then brightest)
    if (effectWarning || (!isWarningPhase && glowIntensity > effectGlowAlpha)) {
      effectGlowColor = effectColor;
      effectGlowAlpha = glowIntensity;
      hasActiveEffect = true;
    }
  }
  
  // Render player glow effect if any effects are active
  if (hasActiveEffect && effectGlowAlpha > 0.01f) {
    const float glowSize = cfg::kPlayerEffectGlowSize;
    const Vector3 glowPos = playerRenderPos;
    const Vector3 glowSizeVec = {
      cfg::kPlayerWidth * glowSize,
      cfg::kPlayerHalfHeight * 2.0f * glowSize,
      cfg::kPlayerDepth * glowSize
    };
    
    // Outer glow (larger, more transparent)
    DrawCubeV(glowPos, glowSizeVec, Fade(effectGlowColor, 0.3f * effectGlowAlpha));
    
    // Inner glow (tighter, brighter)
    DrawCubeV(glowPos, {
      glowSizeVec.x * 0.7f,
      glowSizeVec.y * 0.7f,
      glowSizeVec.z * 0.7f
    }, Fade(effectGlowColor, 0.6f * effectGlowAlpha));
    
    // Core glow (brightest)
    DrawCubeV(glowPos, {
      glowSizeVec.x * 0.5f,
      glowSizeVec.y * 0.5f,
      glowSizeVec.z * 0.5f
    }, Fade(effectGlowColor, 0.8f * effectGlowAlpha));
  }

  if (g_shipLoaded) {
    const float scale = cfg::kShipModelScale;
    const Vector3 shipPos = {playerRenderPos.x,
                             playerRenderPos.y - cfg::kPlayerHalfHeight + 0.05f,
                             playerRenderPos.z};
    DrawModelEx(g_shipModel, shipPos, {0.0f, 1.0f, 0.0f}, 180.0f,
                {scale, scale, scale}, pal.playerBody);
    DrawModelWiresEx(g_shipModel, shipPos, {0.0f, 1.0f, 0.0f}, 180.0f,
                     {scale, scale, scale}, Fade(pal.neonEdgeGlow, 0.6f));
  } else {
    DrawCubeV(
        playerRenderPos,
        {cfg::kPlayerWidth, cfg::kPlayerHalfHeight * 2.0f, cfg::kPlayerDepth},
        pal.playerBody);
    DrawCubeWiresV(
        playerRenderPos,
        {cfg::kPlayerWidth, cfg::kPlayerHalfHeight * 2.0f, cfg::kPlayerDepth},
        pal.playerWire);
  }

  // ── Engine exhaust ────────────────────────────────────────────────────────
  {
    static uint32_t exhaustRng = 12345u;
    const Vector3 exhaustOrigin = {
        playerRenderPos.x, playerRenderPos.y - cfg::kPlayerHalfHeight + 0.2f,
        playerRenderPos.z - cfg::kPlayerDepth * 0.5f * cfg::kShipModelScale -
            0.1f};
    const int spawnCount = 2 + static_cast<int>(speedT * 2.0f);
    for (int i = 0; i < spawnCount; ++i) {
      SpawnExhaustParticle(exhaustOrigin, planarSpeed - cfg::kForwardSpeed,
                           exhaustRng);
    }
    UpdateExhaustParticles(renderDt);

    for (const auto &ep : g_exhaust) {
      if (!ep.active)
        continue;
      const float lifeT = render::Clamp01(ep.life / ep.maxLife);
      const float sz = 0.04f + 0.1f * lifeT;
      DrawCubeV(ep.pos, {sz, sz, sz * 1.5f},
                Color{255,
                      static_cast<unsigned char>(140 + 100 * (1.0f - lifeT)),
                      30, static_cast<unsigned char>(220 * lifeT)});
    }
    DrawCubeV({exhaustOrigin.x, exhaustOrigin.y, exhaustOrigin.z - 0.3f},
              {0.4f + 0.2f * speedT, 0.2f, 0.6f + 0.4f * speedT},
              Fade(Color{255, 160, 40, 255}, 0.15f + 0.1f * speedT));
  }

  // ── Landing particles ─────────────────────────────────────────────────────
  for (const auto &p : game.landingParticles) {
    if (!p.active)
      continue;
    const float lifeT = render::Clamp01(p.life / cfg::kLandingParticleLife);
    DrawCubeV(p.position, {0.08f, 0.08f, 0.08f}, Fade(pal.particle, lifeT));
  }

  EndMode3D();

  // Always reset viewport to full screen for 2D rendering after 3D
  rlViewport(0, 0, cfg::kScreenWidth, cfg::kScreenHeight);
  if (game.screen == GameScreen::Playing) {
    rlDisableScissorTest();
  }

  // ── Power-up text labels (2D rendering) ────────────────────────────────────
  if (game.screen == GameScreen::Playing && lv) {
    for (int pi = 0; pi < lv->powerUpCount; ++pi) {
      const auto &pu = lv->powerUps[pi];
      if (!pu.active) continue;
      if (std::fabs(pu.z - playerRenderPos.z) > 60.0f) continue;
      
      const char* label = GetPowerUpLabel(pu.type);
      if (!label || label[0] == '\0') continue;
      
      // Find ground level (same as in 3D rendering)
      float groundY = pu.y;
      for (int si = 0; si < lv->segmentCount; ++si) {
        const auto &seg = lv->segments[si];
        if (pu.z >= seg.startZ && pu.z <= seg.startZ + seg.length) {
          if (std::abs(pu.x - seg.xOffset) < seg.width * 0.5f) {
            groundY = seg.topY;
            break;
          }
        }
      }
      
      // Text position directly above power-up (no bobbing)
      const float textY = groundY + 0.2f + 0.8f;  // Above the icon
      
      // Convert world position to screen coordinates
      Vector3 textPos = {pu.x, textY, pu.z};
      Vector2 screenPos = GetWorldToScreen(textPos, game.camera);
      
      // Only render if on screen and in front of camera
      if (screenPos.x >= 0 && screenPos.x < cfg::kScreenWidth &&
          screenPos.y >= 0 && screenPos.y < cfg::kScreenHeight &&
          textPos.z > playerRenderPos.z - 5.0f) {
        const float textPulse = 1.0f + 0.15f * std::sin(simTime * cfg::kPowerUpTextPulseSpeed);
        const int fontSize = static_cast<int>(18.0f * textPulse);  // Larger text
        
        // Determine color based on type (match the icon colors)
        Color textColor;
        switch (pu.type) {
          case PowerUpType::Shield:
            textColor = Color{100, 200, 255, 255};
            break;
          case PowerUpType::ScoreMultiplier:
            textColor = Color{100, 255, 150, 255};
            break;
          case PowerUpType::SpeedBoostShield:
            textColor = Color{200, 150, 255, 255};
            break;
          case PowerUpType::SpeedBoostGhost:
            textColor = Color{255, 200, 100, 255};
            break;
          case PowerUpType::ObstacleReveal:
            textColor = Color{255, 255, 100, 255};
            break;
          case PowerUpType::SpeedDrain:
            textColor = Color{255, 80, 80, 255};
            break;
          case PowerUpType::ObstacleSurge:
            textColor = Color{255, 120, 0, 255};
            break;
          default:
            textColor = Color{200, 200, 200, 255};
            break;
        }
        
        // Draw text with stronger outline for visibility
        const int outlineWidth = 2;
        for (int ox = -outlineWidth; ox <= outlineWidth; ++ox) {
          for (int oy = -outlineWidth; oy <= outlineWidth; ++oy) {
            if (ox != 0 || oy != 0) {
              DrawText(label, static_cast<int>(screenPos.x) + ox, 
                      static_cast<int>(screenPos.y) + oy, fontSize, BLACK);
            }
          }
        }
        DrawText(label, static_cast<int>(screenPos.x), 
                static_cast<int>(screenPos.y), fontSize, textColor);
      }
    }
  }

  // ── Bloom overlay ─────────────────────────────────────────────────────────
  if (game.bloomEnabled) {
    const int vH = (game.screen == GameScreen::Playing)
                       ? (cfg::kScreenHeight * 2 / 3)
                       : cfg::kScreenHeight;
    DrawRectangleGradientV(0, 0, cfg::kScreenWidth, vH / 3,
                           Fade(pal.uiAccent, cfg::kBloomOverlayAlpha),
                           Fade(BLACK, 0.0f));
    DrawRectangleGradientV(0, vH / 2, cfg::kScreenWidth, vH / 2,
                           Fade(BLACK, 0.0f),
                           Fade(pal.laneGlow, cfg::kBloomOverlayAlpha * 0.65f));
  }

  // ═══════════════════════════════════════════════════════════════════════════
  //  Screen overlays
  // ═══════════════════════════════════════════════════════════════════════════

  const int cx = cfg::kScreenWidth / 2;
  const int cy = cfg::kScreenHeight / 2;

  if (game.screen == GameScreen::MainMenu) {
    DrawRectangle(0, 0, cfg::kScreenWidth, cfg::kScreenHeight,
                  Fade(BLACK, 0.4f));
    DrawText("S K Y R O A D S", cx - 180, cy - 140, 44, pal.uiAccent);
    DrawText("Endless Runner", cx - 90, cy - 90, 20, pal.uiText);
    const char *items[] = {"Start Game", "Endless Mode", "Leaderboard", "Exit"};
    for (int i = 0; i < 4; ++i) {
      const bool sel = (game.menuSelection == i);
      const int y = cy - 40 + i * 40;
      if (sel)
        DrawRectangleRounded({static_cast<float>(cx - 140),
                              static_cast<float>(y - 6), 280.0f, 36.0f},
                             0.15f, 8, Fade(pal.uiAccent, 0.18f));
      DrawText(sel ? ">" : " ", cx - 130, y, 22,
               sel ? pal.uiAccent : pal.uiText);
      DrawText(items[i], cx - 100, y, 22, sel ? pal.uiAccent : pal.uiText);
    }
    DrawText("Use UP/DOWN + ENTER", cx - 105, cy + 110, 16,
             Fade(pal.uiText, 0.6f));

  } else if (game.screen == GameScreen::LevelSelect) {
    DrawRectangle(0, 0, cfg::kScreenWidth, cfg::kScreenHeight,
                  Fade(BLACK, 0.5f));
    DrawText("S E L E C T   L E V E L", cx - 200, 30, 36, pal.uiAccent);
    const int startX = 60, startY = 100, stageW = 110, stageH = 140,
              stageSpacing = 15;
    const int levelSpacing = 32, rowSpacing = 20, stagesPerRow = 5;
    for (int stage = 1; stage <= 10; ++stage) {
      const int row = (stage - 1) / stagesPerRow;
      const int col = (stage - 1) % stagesPerRow;
      const int stageX = startX + col * (stageW + stageSpacing);
      const int stageY = startY + row * (stageH + rowSpacing);
      DrawRectangleGradientV(stageX, stageY, stageW, stageH,
                             render::GetStageBackgroundTop(stage),
                             render::GetStageBackgroundBottom(stage));
      const int previewGrid = 15;
      const Color previewCol = Fade(pal.gridLine, 0.3f);
      for (int gx = stageX; gx <= stageX + stageW; gx += previewGrid)
        DrawLine(gx, stageY, gx, stageY + stageH, previewCol);
      for (int gy = stageY; gy <= stageY + stageH; gy += previewGrid)
        DrawLine(stageX, gy, stageX + stageW, gy, previewCol);
      const bool stageSelected = (game.levelSelectStage == stage);
      DrawRectangleLinesEx(
          {static_cast<float>(stageX - (stageSelected ? 2 : 0)),
           static_cast<float>(stageY - (stageSelected ? 2 : 0)),
           static_cast<float>(stageW + (stageSelected ? 4 : 0)),
           static_cast<float>(stageH + (stageSelected ? 4 : 0))},
          stageSelected ? 3.0f : 2.0f,
          stageSelected ? pal.uiAccent : Fade(pal.uiAccent, 0.5f));
      char stageLabel[32];
      std::snprintf(stageLabel, sizeof(stageLabel), "S%d", stage);
      DrawText(stageLabel, stageX + 5, stageY + 5, 16, pal.uiAccent);
      for (int level = 1; level <= 3; ++level) {
        const int levelIndex = GetLevelIndexFromStageAndLevel(stage, level);
        const bool isImplemented = IsLevelImplemented(levelIndex);
        const bool isSelected =
            (game.levelSelectStage == stage && game.levelSelectLevel == level);
        const int levelY = stageY + 28 + (level - 1) * levelSpacing;
        Color levelBg =
            isSelected ? Fade(pal.uiAccent, 0.4f) : Fade(BLACK, 0.5f);
        if (!isImplemented)
          levelBg = Fade(BLACK, 0.7f);
        DrawRectangleRounded({static_cast<float>(stageX + 5),
                              static_cast<float>(levelY - 3),
                              static_cast<float>(stageW - 10), 26.0f},
                             0.1f, 8, levelBg);
        char levelLabel[32];
        std::snprintf(levelLabel, sizeof(levelLabel), "L%d", level);
        Color levelColor = isSelected ? pal.uiAccent : pal.uiText;
        if (!isImplemented)
          levelColor = Fade(pal.uiText, 0.3f);
        DrawText(levelLabel, stageX + 12, levelY, 14, levelColor);
        if (!isImplemented)
          DrawText("X", stageX + stageW - 20, levelY, 12,
                   Fade(pal.uiText, 0.4f));
      }
    }
    DrawText("ARROWS: Navigate  ENTER: Select  ESC: Back", cx - 200,
             cfg::kScreenHeight - 50, 16, Fade(pal.uiText, 0.7f));

  } else if (game.screen == GameScreen::PlaceholderLevel) {
    DrawRectangle(0, 0, cfg::kScreenWidth, cfg::kScreenHeight,
                  Fade(BLACK, 0.6f));
    const int stage = GetStageFromLevelIndex(game.currentLevelIndex);
    const int level = GetLevelInStageFromLevelIndex(game.currentLevelIndex);
    DrawText("Didn't implement yet", cx - 140, cy - 40, 32, pal.uiAccent);
    char infoText[64];
    std::snprintf(infoText, sizeof(infoText), "Stage %d - Level %d", stage,
                  level);
    DrawText(infoText, cx - 100, cy + 20, 20, pal.uiText);
    DrawText("Press any key to return", cx - 120, cy + 60, 16,
             Fade(pal.uiText, 0.7f));

  } else if (game.screen == GameScreen::Leaderboard) {
    DrawRectangle(0, 0, cfg::kScreenWidth, cfg::kScreenHeight,
                  Fade(BLACK, 0.5f));
    DrawText("L E A D E R B O A R D", cx - 200, 60, 36, pal.uiAccent);
    
    // Get current leaderboard
    int currentIndex = game.currentLeaderboardIndex;
    auto it = game.leaderboards.find(currentIndex);
    int leaderboardCount = 0;
    const std::array<LeaderboardEntry, cfg::kLeaderboardSize>* currentLeaderboard = nullptr;
    
    if (it != game.leaderboards.end()) {
      currentLeaderboard = &it->second;
      auto countIt = game.leaderboardCounts.find(currentIndex);
      leaderboardCount = (countIt != game.leaderboardCounts.end()) ? countIt->second : 0;
    }
    
    // Display leaderboard name
    char leaderboardName[64];
    if (currentIndex == 0) {
      std::snprintf(leaderboardName, sizeof(leaderboardName), "Endless Mode");
    } else {
      int stage = GetStageFromLevelIndex(currentIndex);
      int level = GetLevelInStageFromLevelIndex(currentIndex);
      std::snprintf(leaderboardName, sizeof(leaderboardName), "Stage %d - Level %d", stage, level);
    }
    DrawText(leaderboardName, cx - 120, 100, 20, pal.uiAccent);
    
    // Navigation hint
    DrawText("< LEFT / RIGHT >", cx - 80, 125, 14, Fade(pal.uiText, 0.7f));
    
    if (leaderboardCount == 0 || !currentLeaderboard) {
      DrawText("No scores yet. Go play!", cx - 130, cy - 20, 22, pal.uiText);
    } else {
      DrawText("#   Name                Score       Time", cx - 260, 150, 15,
               Fade(pal.uiText, 0.5f));
      for (int i = 0; i < leaderboardCount; ++i) {
        const auto &e = (*currentLeaderboard)[i];
        char line[128];
        std::snprintf(line, sizeof(line), "%-2d  %-18s  %-10.0f  %.1fs", i + 1,
                      e.name, e.score, e.runTime);
        DrawText(line, cx - 260, 176 + i * 28, 17,
                 (i == 0) ? pal.uiAccent : pal.uiText);
      }
    }
    DrawText("Press ESC or ENTER to go back", cx - 160, cfg::kScreenHeight - 50,
             16, Fade(pal.uiText, 0.6f));

  } else if (game.screen == GameScreen::Paused) {
    DrawRectangle(0, 0, cfg::kScreenWidth, cfg::kScreenHeight,
                  Fade(BLACK, 0.4f));
    DrawText("P A U S E D", cx - 110, 60, 40, pal.uiAccent);
    const int statsX = 60, statsY = 140;
    DrawRectangleRounded({static_cast<float>(statsX - 10),
                          static_cast<float>(statsY - 10), 380.0f, 400.0f},
                         0.08f, 8, pal.uiPanel);
    DrawText("Run Statistics", statsX, statsY, 24, pal.uiAccent);
    char levelLabel[32];
    std::snprintf(levelLabel, sizeof(levelLabel), "Level %d",
                  game.currentLevelIndex);
    DrawText(levelLabel, statsX, statsY + 28, 18, Fade(pal.uiAccent, 0.9f));
    int statY = statsY + 60;
    const int statSpacing = 28;
    char statBuf[128];
    auto stat = [&](const char *fmt, auto val, float a = 1.0f, int sz = 18) {
      std::snprintf(statBuf, sizeof(statBuf), fmt, val);
      DrawText(statBuf, statsX, statY, sz, Fade(pal.uiText, a));
      statY += statSpacing;
    };
    const float currentScore = GetCurrentScore(game);
    const float distance = game.player.position.z - cfg::kPlatformStartZ;
    stat("Score: %.0f", currentScore);
    std::snprintf(statBuf, sizeof(statBuf), "Best Score: %.0f", game.bestScore);
    DrawText(statBuf, statsX, statY, 18, Fade(pal.uiAccent, 0.9f));
    statY += statSpacing;
    stat("Distance: %.1f u", distance);
    stat("Time: %.1f s", game.runTime);
    stat("Speed: %.1f u/s", planarSpeed);
    stat("Multiplier: x%.2f", game.scoreMultiplier);
    stat("Difficulty: %.1f%%", game.difficultyT * 100.0f);
    stat("Speed Bonus: +%.1f u/s", game.diffSpeedBonus, 0.8f);
    std::snprintf(statBuf, sizeof(statBuf), "Seed: 0x%08X", game.runSeed);
    DrawText(statBuf, statsX, statY, 16, Fade(pal.uiText, 0.6f));
    const int menuX = cx + 200, menuY = statsY + 60;
    const char *items[] = {"Resume", "Restart", "Main Menu"};
    for (int i = 0; i < 3; ++i) {
      const bool sel = (game.pauseSelection == i);
      const int y = menuY + i * 50;
      if (sel)
        DrawRectangleRounded({static_cast<float>(menuX - 10),
                              static_cast<float>(y - 6), 200.0f, 36.0f},
                             0.15f, 8, Fade(pal.uiAccent, 0.18f));
      DrawText(sel ? ">" : " ", menuX, y, 22, sel ? pal.uiAccent : pal.uiText);
      DrawText(items[i], menuX + 20, y, 22, sel ? pal.uiAccent : pal.uiText);
    }
    DrawText("ESC/P to resume", cx - 85, cfg::kScreenHeight - 50, 16,
             Fade(pal.uiText, 0.6f));

  } else if (game.screen == GameScreen::ExitConfirm) {
    DrawRectangle(0, 0, cfg::kScreenWidth, cfg::kScreenHeight,
                  Fade(BLACK, 0.5f));
    DrawText("Exit Game?", cx - 100, cy - 60, 40, pal.uiAccent);
    const char *items[] = {"No", "Yes"};
    for (int i = 0; i < 2; ++i) {
      const bool sel = (game.exitConfirmSelection == i);
      const int y = cy - 10 + i * 50;
      if (sel)
        DrawRectangleRounded({static_cast<float>(cx - 80),
                              static_cast<float>(y - 6), 160.0f, 36.0f},
                             0.15f, 8, Fade(pal.uiAccent, 0.18f));
      DrawText(sel ? ">" : " ", cx - 70, y, 22,
               sel ? pal.uiAccent : pal.uiText);
      DrawText(items[i], cx - 50, y, 22, sel ? pal.uiAccent : pal.uiText);
    }
    DrawText("Use UP/DOWN + ENTER", cx - 105, cy + 110, 16,
             Fade(pal.uiText, 0.6f));
    DrawText("ESC to cancel", cx - 70, cy + 135, 16, Fade(pal.uiText, 0.6f));

  } else if (game.screen == GameScreen::NameEntry) {
    DrawRectangle(0, 0, cfg::kScreenWidth, cfg::kScreenHeight,
                  Fade(BLACK, 0.5f));
    DrawText("NEW HIGH SCORE!", cx - 140, cy - 80, 32, pal.neonEdgeGlow);
    char scoreText[96];
    std::snprintf(scoreText, sizeof(scoreText), "Score: %.0f",
                  game.pendingEntry.score);
    DrawText(scoreText, cx - 70, cy - 30, 24, pal.uiText);
    DrawText("Enter your name:", cx - 100, cy + 10, 20, pal.uiText);
    DrawRectangleLinesEx({cx - 100.0f, cy + 40.0f, 200.0f, 35.0f}, 2.0f,
                         pal.uiAccent);
    char displayText[22];
    std::snprintf(displayText, sizeof(displayText), "%s_",
                  game.nameInputBuffer);
    DrawText(displayText, cx - 95, cy + 48, 18, pal.uiText);
    DrawText("ENTER to confirm", cx - 85, cy + 90, 16, Fade(pal.uiText, 0.7f));
    DrawText("ESC to skip", cx - 60, cy + 110, 16, Fade(pal.uiText, 0.6f));

  } else if (game.screen == GameScreen::GameOver) {
    DrawRectangle(0, 0, cfg::kScreenWidth, cfg::kScreenHeight,
                  Fade(BLACK, 0.4f));
    const char *title =
        game.levelComplete ? "L E V E L   C L E A R" : "G A M E   O V E R";
    DrawText(title, cx - 170, cy - 90, 40,
             game.levelComplete ? pal.neonEdgeGlow : pal.uiAccent);
    if (game.levelComplete) {
      char levelText[64];
      std::snprintf(levelText, sizeof(levelText), "Level %d Complete!",
                    game.currentLevelIndex);
      DrawText(levelText, cx - 100, cy - 40, 22, pal.uiAccent);
    }
    char finalScore[96];
    std::snprintf(finalScore, sizeof(finalScore), "Score: %.0f",
                  GetCurrentScore(game));
    DrawText(finalScore, cx - 70, cy - 10, 26, pal.uiText);
    char bestText[96];
    std::snprintf(bestText, sizeof(bestText), "Best: %.0f", game.bestScore);
    DrawText(bestText, cx - 55, cy + 26, 20, Fade(pal.uiAccent, 0.8f));
    int controlsY = cy + 58;
    if (!game.leaderboardStats.scoreQualified && game.leaderboardCount > 0) {
      int yOff = 60;
      DrawText("--- Leaderboard Stats ---", cx - 120, cy + yOff, 18,
               pal.uiAccent);
      yOff += 30;
      if (game.leaderboardStats.rankIfQualified > 0) {
        char rankText[64];
        std::snprintf(rankText, sizeof(rankText), "Would rank: #%d",
                      game.leaderboardStats.rankIfQualified);
        DrawText(rankText, cx - 90, cy + yOff, 16, pal.uiText);
        yOff += 22;
      }
      if (game.leaderboardStats.scoreDifference10th > 0.0f) {
        char pointsText[96];
        std::snprintf(pointsText, sizeof(pointsText),
                      "Need %.0f more points for top 10",
                      game.leaderboardStats.scoreDifference10th);
        DrawText(pointsText, cx - 130, cy + yOff, 16, Fade(pal.uiText, 0.9f));
        yOff += 22;
      }
      if (game.leaderboardStats.scorePercent10th > 0.0f) {
        char percentText[96];
        std::snprintf(percentText, sizeof(percentText),
                      "%.1f%% of 10th place score",
                      game.leaderboardStats.scorePercent10th);
        DrawText(percentText, cx - 100, cy + yOff, 15, Fade(pal.uiText, 0.8f));
        yOff += 22;
      }
      if (game.leaderboardStats.scoreDifference1st > 0.0f) {
        char firstText[96];
        std::snprintf(firstText, sizeof(firstText),
                      "Need %.0f more for 1st place",
                      game.leaderboardStats.scoreDifference1st);
        DrawText(firstText, cx - 110, cy + yOff, 15, Fade(pal.uiAccent, 0.7f));
        yOff += 22;
      }
      if (game.leaderboardStats.timeDifference10th > 5.0f) {
        char timeText[96];
        std::snprintf(timeText, sizeof(timeText),
                      "~%.1fs more needed (estimate)",
                      game.leaderboardStats.timeDifference10th);
        DrawText(timeText, cx - 120, cy + yOff, 14, Fade(pal.uiText, 0.7f));
        yOff += 20;
      }
      yOff += 10;
      DrawText("Keep trying!", cx - 60, cy + yOff, 16,
               Fade(pal.uiAccent, 0.8f));
      controlsY = cy + yOff + 40;
    } else if (game.leaderboardCount == 0) {
      DrawText("First run! Set the bar!", cx - 110, cy + 60, 18, pal.uiAccent);
      controlsY = cy + 100;
    }
    DrawText("R  Retry same seed", cx - 110, controlsY, 18, pal.uiText);
    DrawText("N  New run", cx - 110, controlsY + 26, 18, pal.uiText);
    DrawText("ESC  Main menu", cx - 110, controlsY + 52, 18,
             Fade(pal.uiText, 0.7f));

  } else {
    // Playing — cockpit HUD + minimal score strip
    render::RenderCockpitHUD(game, pal, planarSpeed);
    DrawRectangleRounded({10.0f, 10.0f, 280.0f, 80.0f}, 0.08f, 8,
                         Fade(pal.uiPanel, 0.8f));
    char levelText[32];
    if (game.isEndlessMode) {
      std::snprintf(levelText, sizeof(levelText), "Endless Mode");
    } else {
      std::snprintf(levelText, sizeof(levelText), "Level %d",
                    game.currentLevelIndex);
    }
    DrawText(levelText, 20, 18, 16, Fade(pal.uiAccent, 0.9f));
    char scoreText[96];
    std::snprintf(scoreText, sizeof(scoreText), "Score: %.0f",
                  GetCurrentScore(game));
    DrawText(scoreText, 20, 38, 16, pal.uiText);
    char multText[64];
    std::snprintf(multText, sizeof(multText), "x%.2f", game.scoreMultiplier);
    DrawText(multText, 20, 58, 14, pal.uiAccent);
    
    // Active effects UI (top-right corner)
    if (game.activeEffectCount > 0) {
      const float effectsX = cfg::kScreenWidth - 200.0f;
      float effectsY = 20.0f;
      DrawRectangleRounded({effectsX - 10.0f, effectsY - 10.0f, 190.0f, 
                            static_cast<float>(game.activeEffectCount * 30 + 20)}, 
                           0.08f, 8, Fade(pal.uiPanel, 0.8f));
      
      for (int i = 0; i < game.activeEffectCount; ++i) {
        const auto &effect = game.activeEffects[i];
        if (effect.type == PowerUpType::None) continue;
        
        const char* label = GetPowerUpLabel(effect.type);
        const bool isDebuff = IsDebuff(effect.type);
        Color effectColor = isDebuff ? Color{255, 150, 150, 255} : Color{150, 255, 200, 255};
        
        // Effect label
        DrawText(label, effectsX, effectsY, 14, effectColor);
        
        // Timer/progress
        if (effect.type == PowerUpType::Shield) {
          if (!effect.consumed) {
            DrawText("READY", effectsX + 80, effectsY, 12, Color{100, 255, 100, 255});
          } else {
            DrawText("USED", effectsX + 80, effectsY, 12, Color{200, 200, 200, 200});
          }
        } else if (effect.timer > 0.0f) {
          char timerText[16];
          std::snprintf(timerText, sizeof(timerText), "%.1fs", effect.timer);
          DrawText(timerText, effectsX + 80, effectsY, 12, Fade(pal.uiText, 0.8f));
        }
        
        effectsY += 28.0f;
      }
    }
  }

  // ── Perf overlay ─────────────────────────────────────────────────────────
  {
    const int perfY = cfg::kScreenHeight - 52;
    DrawRectangleRounded({10.0f, static_cast<float>(perfY), 310.0f, 42.0f},
                         0.08f, 8, pal.uiPanel);
    char perfBuf[128];
    std::snprintf(perfBuf, sizeof(perfBuf), "Update: %.2f ms  Render: %.2f ms",
                  game.updateMs, game.renderMs);
    DrawText(perfBuf, 20, perfY + 6, 13, pal.uiText);
    std::snprintf(perfBuf, sizeof(perfBuf), "Allocs: %d",
                  game.updateAllocCount);
    DrawText(perfBuf, 20, perfY + 24, 13,
             game.updateAllocCount > 0 ? pal.uiAccent : pal.uiText);
  }

  // ── Screenshot notification ───────────────────────────────────────────────
  if (game.screenshotNotificationTimer > 0.0f) {
    const float alpha =
        render::Clamp01(game.screenshotNotificationTimer / 0.5f);
    const int notifW = 400;
    const int notifX = (cfg::kScreenWidth - notifW) / 2;
    DrawRectangleRounded(
        {static_cast<float>(notifX), 60.0f, static_cast<float>(notifW), 50.0f},
        0.1f, 8, Fade(pal.uiPanel, alpha * 0.95f));
    DrawText("Screenshot saved!", notifX + 20, 68, 20,
             Fade(pal.uiAccent, alpha));
    DrawText(game.screenshotPath, notifX + 20, 90, 14,
             Fade(pal.uiText, alpha * 0.8f));
  }

  EndDrawing();
}
