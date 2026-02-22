#include "render/GateRenderer.hpp"

#include <cmath>

#include "render/Palette.hpp"
#include "render/RenderUtils.hpp"
#include "sim/Level.hpp"

namespace render {

// ─── Finish line
// ──────────────────────────────────────────────────────────────

void RenderFinishLine(const Level &level, const Vector3 &playerRenderPos,
                      const LevelPalette &pal, float simTime) {
  const auto &finish = level.finish;
  if (finish.style == FinishStyle::None)
    return;

  const float finishMidZ = (finish.startZ + finish.endZ) * 0.5f;
  if (std::fabs(finishMidZ - playerRenderPos.z) > 80.0f)
    return;

  const float halfW = finish.width * 0.5f;
  const float leftEdge = finish.xOffset - halfW;
  const float rightEdge = finish.xOffset + halfW;
  const float finishDepth = finish.endZ - finish.startZ;
  const float finishMidZAct = finishMidZ;

  // Ground glow
  DrawCubeV(Vector3{finish.xOffset, finish.topY + 0.01f, finishMidZAct},
            Vector3{finish.width, 0.02f, finishDepth},
            Fade(pal.neonEdgeGlow, 0.2f * finish.glowIntensity));

  switch (finish.style) {
  case FinishStyle::NeonGate: {
    const float gateH = 4.0f, gateZ = finish.startZ + finishDepth * 0.5f;
    const float postW = 0.25f, postD = 0.3f;

    auto drawPost = [&](float x) {
      DrawCubeV(Vector3{x, finish.topY + gateH * 0.5f, gateZ},
                Vector3{postW, gateH, postD}, pal.neonEdge);
      DrawCubeV(Vector3{x, finish.topY + gateH * 0.5f, gateZ},
                Vector3{postW * 3.0f, gateH * 1.2f, postD * 2.0f},
                Fade(pal.neonEdgeGlow, 0.4f * finish.glowIntensity));
    };
    drawPost(leftEdge);
    drawPost(rightEdge);

    // Beam
    const float beamY = finish.topY + gateH - 0.2f;
    DrawCubeV(Vector3{finish.xOffset, beamY, gateZ},
              Vector3{finish.width, 0.15f, 0.2f}, pal.neonEdge);
    DrawCubeV(Vector3{finish.xOffset, beamY, gateZ},
              Vector3{finish.width * 1.1f, 0.3f, 0.4f},
              Fade(pal.neonEdgeGlow, 0.3f * finish.glowIntensity));

    if (finish.hasRunway) {
      constexpr int mc = 8;
      for (int i = 0; i < mc; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(mc - 1);
        const float pulse = 0.8f + 0.2f * std::sin(simTime * 3.0f + t * 2.0f);
        DrawCubeV(Vector3{finish.xOffset, finish.topY + 0.05f,
                          finish.startZ + t * finishDepth},
                  Vector3{finish.width * 0.6f, 0.03f, 0.15f},
                  Fade(pal.laneGlow, 0.4f * pulse * finish.glowIntensity));
      }
    }
    break;
  }

  case FinishStyle::SegmentedPylons: {
    const float pylonH = 3.5f;
    constexpr int pylonCount = 5;
    for (int i = 0; i < pylonCount; ++i) {
      const float t =
          static_cast<float>(i) / static_cast<float>(pylonCount - 1);
      const float pz = finish.startZ + t * finishDepth;
      const float offset = finish.xOffset + (i % 2 == 0 ? -0.8f : 0.8f);
      constexpr int segs = 3;
      for (int s = 0; s < segs; ++s) {
        const float segY = finish.topY +
                           static_cast<float>(s) * (pylonH / segs) +
                           (pylonH / segs) * 0.5f;
        const float pulse =
            0.7f +
            0.3f * std::sin(simTime * 2.5f + static_cast<float>(i + s) * 0.5f);
        DrawCubeV(Vector3{offset, segY, pz},
                  Vector3{0.2f, pylonH / segs * 0.9f, 0.25f},
                  Fade(pal.neonEdge, pulse));
        DrawCubeV(Vector3{offset, segY, pz},
                  Vector3{0.5f, pylonH / segs * 1.1f, 0.5f},
                  Fade(pal.neonEdgeGlow, 0.25f * pulse * finish.glowIntensity));
      }
    }
    if (finish.hasRunway) {
      constexpr int mc = 10;
      for (int i = 0; i < mc; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(mc - 1);
        const float pulse = 0.75f + 0.25f * std::sin(simTime * 4.0f + t * 3.0f);
        DrawCubeV(Vector3{finish.xOffset, finish.topY + 0.05f,
                          finish.startZ + t * finishDepth},
                  Vector3{finish.width * 0.5f, 0.03f, 0.12f},
                  Fade(pal.laneGlow, 0.35f * pulse * finish.glowIntensity));
      }
    }
    break;
  }

  case FinishStyle::PrecisionCorridor: {
    const float chevH = 2.5f;
    constexpr int chevN = 6;
    const float chevSpacing = finishDepth / static_cast<float>(chevN);
    for (int i = 0; i < chevN; ++i) {
      const float cz = finish.startZ + static_cast<float>(i) * chevSpacing +
                       chevSpacing * 0.5f;
      const float phase =
          std::fmod(simTime * 2.0f + static_cast<float>(i) * 0.3f, 1.0f);
      const float chevY = finish.topY + chevH * 0.5f;
      const float halfWC = finish.width * (0.3f + 0.1f * phase);
      DrawLine3D(Vector3{finish.xOffset - halfWC, chevY, cz},
                 Vector3{finish.xOffset, chevY + chevH * 0.5f, cz},
                 Fade(pal.neonEdge, 0.9f * finish.glowIntensity));
      DrawLine3D(Vector3{finish.xOffset + halfWC, chevY, cz},
                 Vector3{finish.xOffset, chevY + chevH * 0.5f, cz},
                 Fade(pal.neonEdge, 0.9f * finish.glowIntensity));
      DrawCubeV(Vector3{finish.xOffset, chevY + chevH * 0.25f, cz},
                Vector3{halfWC * 0.6f, chevH * 0.5f, 0.15f},
                Fade(pal.neonEdgeGlow, 0.2f * finish.glowIntensity));
    }
    DrawCubeV(Vector3{leftEdge, finish.topY + 1.0f, finishMidZAct},
              Vector3{0.15f, 2.0f, finishDepth}, Fade(pal.neonEdge, 0.6f));
    DrawCubeV(Vector3{rightEdge, finish.topY + 1.0f, finishMidZAct},
              Vector3{0.15f, 2.0f, finishDepth}, Fade(pal.neonEdge, 0.6f));
    if (finish.hasRunway) {
      constexpr int mc = 12;
      for (int i = 0; i < mc; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(mc - 1);
        const float pulse = 0.8f + 0.2f * std::sin(simTime * 5.0f + t * 4.0f);
        DrawCubeV(Vector3{finish.xOffset, finish.topY + 0.05f,
                          finish.startZ + t * finishDepth},
                  Vector3{finish.width * 0.4f, 0.03f, 0.1f},
                  Fade(pal.laneGlow, 0.4f * pulse * finish.glowIntensity));
      }
    }
    break;
  }

  case FinishStyle::MultiRingPortal: {
    const float ringH = 5.0f;
    const int ringN = static_cast<int>(finish.ringCount);
    const float rSpacing = finishDepth / static_cast<float>(ringN + 1);
    constexpr int rSegs = 16;
    for (int i = 0; i < ringN; ++i) {
      const float rz = finish.startZ + static_cast<float>(i + 1) * rSpacing;
      const float phase =
          std::fmod(simTime * 1.5f + static_cast<float>(i) * 0.4f, 1.0f);
      const float scale = 0.8f + 0.2f * std::sin(phase * kPi);
      const float ringY = finish.topY + ringH * 0.5f;
      const float outerR = finish.width * 0.5f * scale;
      const float innerR = outerR * 0.6f;

      for (int s = 0; s < rSegs; ++s) {
        const float a1 =
            static_cast<float>(s) / static_cast<float>(rSegs) * 2.0f * kPi;
        const float a2 =
            static_cast<float>(s + 1) / static_cast<float>(rSegs) * 2.0f * kPi;
        // Outer ring
        DrawLine3D(Vector3{finish.xOffset + std::cos(a1) * outerR, ringY,
                           rz + std::sin(a1) * outerR * 0.3f},
                   Vector3{finish.xOffset + std::cos(a2) * outerR, ringY,
                           rz + std::sin(a2) * outerR * 0.3f},
                   Fade(pal.neonEdge, 0.9f * finish.glowIntensity));
        // Inner ring
        DrawLine3D(Vector3{finish.xOffset + std::cos(a1) * innerR, ringY,
                           rz + std::sin(a1) * innerR * 0.3f},
                   Vector3{finish.xOffset + std::cos(a2) * innerR, ringY,
                           rz + std::sin(a2) * innerR * 0.3f},
                   Fade(pal.neonEdgeGlow, 0.7f * finish.glowIntensity));
      }
      DrawCubeV(Vector3{finish.xOffset, ringY, rz},
                Vector3{outerR * 2.0f, ringH * 0.8f, outerR * 0.6f},
                Fade(pal.neonEdgeGlow, 0.15f * scale * finish.glowIntensity));
    }
    if (finish.hasRunway) {
      constexpr int mc = 14;
      for (int i = 0; i < mc; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(mc - 1);
        const float pulse = 0.85f + 0.15f * std::sin(simTime * 3.5f + t * 2.5f);
        DrawCubeV(Vector3{finish.xOffset, finish.topY + 0.05f,
                          finish.startZ + t * finishDepth},
                  Vector3{finish.width * 0.7f, 0.04f, 0.18f},
                  Fade(pal.laneGlow, 0.5f * pulse * finish.glowIntensity));
      }
    }
    break;
  }

  default:
    break;
  }
}

// ─── Start line
// ───────────────────────────────────────────────────────────────

void RenderStartLine(const Level &level, const Vector3 &playerRenderPos,
                     const LevelPalette &pal, float simTime) {
  const auto &start = level.start;
  if (start.style == StartStyle::None)
    return;
  if (playerRenderPos.z - start.gateZ > 30.0f)
    return;

  const float halfW = start.width * 0.5f;
  const float leftEdge = start.xOffset - halfW;
  const float rightEdge = start.xOffset + halfW;

  DrawCubeV(Vector3{start.xOffset, start.topY + 0.01f, start.gateZ},
            Vector3{start.width, 0.02f, start.zoneDepth},
            Fade(pal.neonEdgeGlow, 0.15f * start.glowIntensity));

  switch (start.style) {
  case StartStyle::NeonGate: {
    const float gateH = 3.5f, postW = 0.2f, postD = 0.25f;
    auto drawPost = [&](float x) {
      DrawCubeV(Vector3{x, start.topY + gateH * 0.5f, start.gateZ},
                Vector3{postW, gateH, postD}, pal.neonEdge);
      DrawCubeV(Vector3{x, start.topY + gateH * 0.5f, start.gateZ},
                Vector3{postW * 3.0f, gateH * 1.1f, postD * 2.0f},
                Fade(pal.neonEdgeGlow, 0.35f * start.glowIntensity));
    };
    drawPost(leftEdge);
    drawPost(rightEdge);
    const float beamY = start.topY + gateH - 0.15f;
    DrawCubeV(Vector3{start.xOffset, beamY, start.gateZ},
              Vector3{start.width, 0.12f, 0.15f}, pal.neonEdge);
    DrawCubeV(Vector3{start.xOffset, beamY, start.gateZ},
              Vector3{start.width * 1.05f, 0.25f, 0.3f},
              Fade(pal.neonEdgeGlow, 0.25f * start.glowIntensity));
    for (int i = 0; i < start.stripeCount; ++i) {
      const float t =
          static_cast<float>(i) / static_cast<float>(start.stripeCount - 1);
      const float sz =
          start.gateZ - start.zoneDepth * 0.5f + t * start.zoneDepth;
      const float pulse = 0.7f + 0.3f * std::sin(simTime * 3.0f + t * 2.0f);
      DrawCubeV(Vector3{start.xOffset, start.topY + 0.04f, sz},
                Vector3{start.width * 0.5f, 0.025f, 0.1f},
                Fade(pal.laneGlow, 0.3f * pulse * start.glowIntensity));
    }
    break;
  }

  case StartStyle::IndustrialPylons: {
    const float pylonH = 3.0f;
    const int pylonN =
        static_cast<int>(start.zoneDepth / start.pylonSpacing) + 1;
    for (int i = 0; i < pylonN; ++i) {
      const float pz = start.gateZ - start.zoneDepth * 0.5f +
                       static_cast<float>(i) * start.pylonSpacing;
      const float offset = start.xOffset + (i % 2 == 0 ? -0.6f : 0.6f);
      constexpr int segs = 4;
      for (int s = 0; s < segs; ++s) {
        const float segY = start.topY +
                           static_cast<float>(s) * (pylonH / segs) +
                           (pylonH / segs) * 0.5f;
        const float pulse =
            0.65f +
            0.35f * std::sin(simTime * 2.0f + static_cast<float>(i + s) * 0.4f);
        DrawCubeV(Vector3{offset, segY, pz},
                  Vector3{0.18f, pylonH / segs * 0.85f, 0.2f},
                  Fade(pal.neonEdge, pulse));
        DrawCubeV(Vector3{offset, segY, pz},
                  Vector3{0.4f, pylonH / segs * 1.05f, 0.36f},
                  Fade(pal.neonEdgeGlow, 0.2f * pulse * start.glowIntensity));
      }
    }
    for (int i = 0; i < start.stripeCount; ++i) {
      const float t =
          static_cast<float>(i) / static_cast<float>(start.stripeCount - 1);
      const float sz =
          start.gateZ - start.zoneDepth * 0.5f + t * start.zoneDepth;
      const float pulse = 0.7f + 0.3f * std::sin(simTime * 3.5f + t * 2.5f);
      DrawCubeV(Vector3{start.xOffset, start.topY + 0.04f, sz},
                Vector3{start.width * 0.45f, 0.025f, 0.08f},
                Fade(pal.laneGlow, 0.28f * pulse * start.glowIntensity));
    }
    break;
  }

  case StartStyle::PrecisionCorridor: {
    const float barrierH = 2.2f;
    DrawCubeV(Vector3{leftEdge, start.topY + barrierH * 0.5f, start.gateZ},
              Vector3{0.12f, barrierH, start.zoneDepth},
              Fade(pal.neonEdge, 0.55f));
    DrawCubeV(Vector3{rightEdge, start.topY + barrierH * 0.5f, start.gateZ},
              Vector3{0.12f, barrierH, start.zoneDepth},
              Fade(pal.neonEdge, 0.55f));
    const float markerSpacing = start.zoneDepth / static_cast<float>(6 + 1);
    for (int i = 0; i < 6; ++i) {
      const float mz = start.gateZ - start.zoneDepth * 0.5f +
                       static_cast<float>(i + 1) * markerSpacing;
      const float phase =
          std::fmod(simTime * 1.8f + static_cast<float>(i) * 0.25f, 1.0f);
      const float mHalfW = start.width * (0.2f + 0.075f * phase);
      DrawLine3D(Vector3{start.xOffset - mHalfW, start.topY + 0.3f, mz},
                 Vector3{start.xOffset, start.topY + 0.5f, mz},
                 Fade(pal.neonEdge, 0.85f * start.glowIntensity));
      DrawLine3D(Vector3{start.xOffset + mHalfW, start.topY + 0.3f, mz},
                 Vector3{start.xOffset, start.topY + 0.5f, mz},
                 Fade(pal.neonEdge, 0.85f * start.glowIntensity));
    }
    for (int i = 0; i < start.stripeCount; ++i) {
      const float t =
          static_cast<float>(i) / static_cast<float>(start.stripeCount - 1);
      const float sz =
          start.gateZ - start.zoneDepth * 0.5f + t * start.zoneDepth;
      const float pulse = 0.75f + 0.25f * std::sin(simTime * 4.0f + t * 3.0f);
      DrawCubeV(Vector3{start.xOffset, start.topY + 0.04f, sz},
                Vector3{start.width * 0.35f, 0.025f, 0.07f},
                Fade(pal.laneGlow, 0.35f * pulse * start.glowIntensity));
    }
    break;
  }

  case StartStyle::RingedLaunch: {
    const float ringH = 4.5f;
    const int ringN = static_cast<int>(start.ringCount);
    const float rSpacing = start.zoneDepth / static_cast<float>(ringN + 1);
    constexpr int rSegs = 16;
    for (int i = 0; i < ringN; ++i) {
      const float rz = start.gateZ - start.zoneDepth * 0.5f +
                       static_cast<float>(i + 1) * rSpacing;
      const float phase =
          std::fmod(simTime * 1.2f + static_cast<float>(i) * 0.35f, 1.0f);
      const float scale = 0.75f + 0.25f * std::sin(phase * kPi);
      const float ringY = start.topY + ringH * 0.5f;
      const float outerR = start.width * 0.5f * scale;
      const float innerR = outerR * 0.55f;

      for (int s = 0; s < rSegs; ++s) {
        const float a1 =
            static_cast<float>(s) / static_cast<float>(rSegs) * 2.0f * kPi;
        const float a2 =
            static_cast<float>(s + 1) / static_cast<float>(rSegs) * 2.0f * kPi;
        DrawLine3D(Vector3{start.xOffset + std::cos(a1) * outerR, ringY,
                           rz + std::sin(a1) * outerR * 0.25f},
                   Vector3{start.xOffset + std::cos(a2) * outerR, ringY,
                           rz + std::sin(a2) * outerR * 0.25f},
                   Fade(pal.neonEdge, 0.85f * start.glowIntensity));
        DrawLine3D(Vector3{start.xOffset + std::cos(a1) * innerR, ringY,
                           rz + std::sin(a1) * innerR * 0.25f},
                   Vector3{start.xOffset + std::cos(a2) * innerR, ringY,
                           rz + std::sin(a2) * innerR * 0.25f},
                   Fade(pal.neonEdgeGlow, 0.65f * start.glowIntensity));
      }
      DrawCubeV(Vector3{start.xOffset, ringY, rz},
                Vector3{outerR * 1.8f, ringH * 0.7f, outerR * 0.5f},
                Fade(pal.neonEdgeGlow, 0.12f * scale * start.glowIntensity));
    }
    for (int i = 0; i < start.stripeCount; ++i) {
      const float t =
          static_cast<float>(i) / static_cast<float>(start.stripeCount - 1);
      const float sz =
          start.gateZ - start.zoneDepth * 0.5f + t * start.zoneDepth;
      const float pulse = 0.8f + 0.2f * std::sin(simTime * 3.0f + t * 2.0f);
      DrawCubeV(Vector3{start.xOffset, start.topY + 0.04f, sz},
                Vector3{start.width * 0.6f, 0.03f, 0.12f},
                Fade(pal.laneGlow, 0.4f * pulse * start.glowIntensity));
    }
    break;
  }

  default:
    break;
  }
}

} // namespace render
