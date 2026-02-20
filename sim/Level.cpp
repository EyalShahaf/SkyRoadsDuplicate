#include "sim/Level.hpp"

#include <cmath>

namespace {

// Forward declaration for helper
void AssignVariantsHelper(Level& lv);

Level BuildLevel1() {
    Level lv{};

    auto addSeg = [&](float startZ, float length, float topY, float width, float xOffset) {
        if (lv.segmentCount >= kMaxSegments) return;
        auto& s = lv.segments[lv.segmentCount++];
        s.startZ = startZ;
        s.length = length;
        s.topY = topY;
        s.width = width;
        s.xOffset = xOffset;
    };

    auto addObs = [&](float z, float x, float y, float sx, float sy, float sz, int col) {
        if (lv.obstacleCount >= kMaxObstacles) return;
        auto& o = lv.obstacles[lv.obstacleCount++];
        o.z = z; o.x = x; o.y = y;
        o.sizeX = sx; o.sizeY = sy; o.sizeZ = sz;
        o.colorIndex = col;
    };

    // ===== LEVEL 1: "Neon Highway" =====
    // Design: ~600 units long, progressive difficulty.
    // Starts easy (wide, flat), introduces gaps, narrows, height changes, obstacles.

    float z = 0.0f;

    // --- Section 1: Wide intro runway (easy) ---
    addSeg(z, 40.0f, 0.0f, 8.0f, 0.0f);   z += 40.0f;
    // Small gap (2 units — very forgiving, can just run over at speed)
    z += 2.0f;
    addSeg(z, 30.0f, 0.0f, 8.0f, 0.0f);   z += 30.0f;

    // --- Section 2: First real gap (needs a hop) ---
    z += 4.0f;  // gap
    addSeg(z, 25.0f, 0.0f, 8.0f, 0.0f);   z += 25.0f;
    // Obstacle: side blocker (teaches dodging — NOT center, so player can pass on either side)
    addObs(z - 12.0f, 2.0f, 0.0f, 1.2f, 1.8f, 1.2f, 0);

    // --- Section 3: Two quick gaps ---
    z += 5.0f;  // gap
    addSeg(z, 15.0f, 0.0f, 7.0f, 0.0f);   z += 15.0f;
    z += 4.0f;  // gap
    addSeg(z, 20.0f, 0.0f, 8.0f, 0.0f);   z += 20.0f;

    // --- Section 4: Slight height change + narrow ---
    z += 5.0f;  // gap
    addSeg(z, 20.0f, 0.5f, 6.0f, 0.0f);   z += 20.0f;
    // Side obstacles
    addObs(z - 10.0f, -2.2f, 0.5f, 0.8f, 2.0f, 0.8f, 1);
    addObs(z - 10.0f,  2.2f, 0.5f, 0.8f, 2.0f, 0.8f, 1);

    // --- Section 5: Stepped platforms ---
    z += 6.0f;  // gap
    addSeg(z, 12.0f, 1.0f, 6.0f, 0.0f);   z += 12.0f;
    z += 4.0f;
    addSeg(z, 12.0f, 1.5f, 6.0f, 0.0f);   z += 12.0f;
    z += 4.0f;
    addSeg(z, 12.0f, 1.0f, 7.0f, 0.0f);   z += 12.0f;

    // --- Section 6: Zigzag (offset platforms, no obstacles — pure navigation) ---
    z += 5.0f;
    addSeg(z, 16.0f, 0.5f, 5.5f, -2.0f);  z += 16.0f;
    z += 5.0f;
    addSeg(z, 16.0f, 0.5f, 5.5f,  2.0f);  z += 16.0f;
    z += 5.0f;
    addSeg(z, 16.0f, 0.5f, 5.5f, -1.5f);  z += 16.0f;

    // --- Section 7: Long corridor with obstacle slalom ---
    z += 6.0f;
    addSeg(z, 55.0f, 0.0f, 8.0f, 0.0f);   z += 55.0f;
    // Alternating obstacles (well-spaced, dodgeable)
    addObs(z - 46.0f, -2.5f, 0.0f, 0.8f, 1.8f, 0.8f, 0);
    addObs(z - 36.0f,  2.5f, 0.0f, 0.8f, 1.8f, 0.8f, 1);
    addObs(z - 26.0f, -2.0f, 0.0f, 0.8f, 1.8f, 0.8f, 2);
    addObs(z - 16.0f,  2.0f, 0.0f, 0.8f, 1.8f, 0.8f, 0);
    addObs(z - 6.0f,  -1.0f, 0.0f, 0.8f, 2.2f, 0.8f, 1);

    // --- Section 8: Big gap (needs dash-jump) ---
    z += 8.0f;  // big gap
    addSeg(z, 18.0f, 0.0f, 8.0f, 0.0f);   z += 18.0f;

    // --- Section 9: Narrow winding path ---
    z += 5.0f;
    addSeg(z, 12.0f, 0.0f, 4.0f,  1.5f);  z += 12.0f;
    z += 3.0f;
    addSeg(z, 12.0f, 0.0f, 4.0f, -1.5f);  z += 12.0f;
    z += 3.0f;
    addSeg(z, 12.0f, 0.0f, 4.0f,  0.0f);  z += 12.0f;

    // --- Section 10: Elevated finale (just platforms, no obstacles — the gaps ARE the challenge) ---
    z += 7.0f;
    addSeg(z, 14.0f, 2.0f, 7.0f, 0.0f);   z += 14.0f;
    z += 6.0f;
    addSeg(z, 14.0f, 2.5f, 6.0f, 1.0f);   z += 14.0f;
    z += 6.0f;
    addSeg(z, 14.0f, 2.0f, 6.0f, -1.0f);  z += 14.0f;
    z += 5.0f;

    // --- Victory platform (wide, safe) ---
    addSeg(z, 30.0f, 0.0f, 8.0f, 0.0f);   z += 30.0f;

    lv.totalLength = z;
    
    // Start zone: Level 1 - Clean/simple intro gate, wide lane
    lv.start.spawnZ = 2.0f;
    lv.start.gateZ = 0.0f;
    lv.start.zoneDepth = 8.0f;
    lv.start.style = StartStyle::NeonGate;
    lv.start.width = 8.0f;
    lv.start.xOffset = 0.0f;
    lv.start.topY = 0.0f;
    lv.start.pylonSpacing = 2.5f;
    lv.start.glowIntensity = 1.0f;
    lv.start.stripeCount = 4;
    
    // Finish zone: Level 1 - Simple neon gate, wide forgiving lane
    lv.finish.startZ = z - 25.0f;  // Start 25 units before end
    lv.finish.endZ = z - 5.0f;     // Complete 5 units before totalLength
    lv.finish.style = FinishStyle::NeonGate;
    lv.finish.width = 8.0f;
    lv.finish.xOffset = 0.0f;
    lv.finish.topY = 0.0f;
    lv.finish.glowIntensity = 1.0f;
    lv.finish.hasRunway = true;
    
    AssignVariantsHelper(lv);
    return lv;
}

const Level g_level1 = BuildLevel1();

Level BuildLevel2() {
    Level lv{};

    auto addSeg = [&](float startZ, float length, float topY, float width, float xOffset) {
        if (lv.segmentCount >= kMaxSegments) return;
        auto& s = lv.segments[lv.segmentCount++];
        s.startZ = startZ;
        s.length = length;
        s.topY = topY;
        s.width = width;
        s.xOffset = xOffset;
    };

    auto addObs = [&](float z, float x, float y, float sx, float sy, float sz, int col) {
        if (lv.obstacleCount >= kMaxObstacles) return;
        auto& o = lv.obstacles[lv.obstacleCount++];
        o.z = z; o.x = x; o.y = y;
        o.sizeX = sx; o.sizeY = sy; o.sizeZ = sz;
        o.colorIndex = col;
    };

    // ===== LEVEL 2: "Sky Labyrinth" =====
    // Design: ~700 units long, more challenging than Level 1.
    // Features: Narrower platforms, more obstacles, complex height changes, tighter gaps.

    float z = 0.0f;

    // --- Section 1: Quick warm-up (shorter than L1) ---
    addSeg(z, 25.0f, 0.0f, 7.0f, 0.0f);   z += 25.0f;
    z += 3.0f;  // small gap
    addSeg(z, 20.0f, 0.0f, 7.0f, 0.0f);   z += 20.0f;

    // --- Section 2: Early obstacle introduction (side obstacles, easier to dodge) ---
    z += 5.0f;
    addSeg(z, 30.0f, 0.0f, 7.0f, 0.0f);   z += 30.0f;
    // Side obstacle (easier to dodge than center)
    addObs(z - 15.0f, -2.5f, 0.0f, 0.9f, 1.9f, 0.9f, 0);

    // --- Section 3: Narrowing platforms with side obstacles ---
    z += 6.0f;
    addSeg(z, 18.0f, 0.0f, 5.5f, 0.0f);   z += 18.0f;
    addObs(z - 9.0f, -2.5f, 0.0f, 0.9f, 1.9f, 0.9f, 1);
    addObs(z - 9.0f,  2.5f, 0.0f, 0.9f, 1.9f, 0.9f, 1);
    z += 5.0f;
    addSeg(z, 18.0f, 0.0f, 5.0f, 0.0f);   z += 18.0f;

    // --- Section 4: Stepped path (easier - wider platforms) ---
    z += 6.0f;
    addSeg(z, 16.0f, 0.5f, 5.5f, -0.5f);  z += 16.0f;
    z += 5.0f;
    addSeg(z, 16.0f, 1.0f, 5.5f, 0.5f);   z += 16.0f;
    z += 5.0f;
    addSeg(z, 16.0f, 0.5f, 5.5f, 0.0f);  z += 16.0f;
    z += 5.0f;
    addSeg(z, 16.0f, 0.0f, 6.0f, 0.0f);   z += 16.0f;

    // --- Section 5: Wide corridor with obstacle maze (easier - fewer obstacles) ---
    z += 6.0f;
    addSeg(z, 50.0f, 0.0f, 7.5f, 0.0f);   z += 50.0f;
    // Less dense pattern - more space between obstacles
    addObs(z - 45.0f, -2.8f, 0.0f, 0.85f, 1.8f, 0.85f, 0);
    addObs(z - 35.0f,  2.8f, 0.0f, 0.85f, 1.8f, 0.85f, 1);
    addObs(z - 25.0f, -2.5f, 0.0f, 0.85f, 1.8f, 0.85f, 2);
    addObs(z - 15.0f,  2.5f, 0.0f, 0.85f, 1.8f, 0.85f, 0);
    addObs(z - 5.0f,  -2.0f, 0.0f, 0.85f, 1.8f, 0.85f, 1);

    // --- Section 6: Gap sequence (easier - smaller gaps) ---
    z += 5.0f;  // smaller gap
    addSeg(z, 15.0f, 0.0f, 6.5f, 0.0f);   z += 15.0f;
    z += 6.0f;  // gap
    addSeg(z, 15.0f, 0.0f, 6.5f, 0.0f);   z += 15.0f;
    z += 6.0f;  // gap
    addSeg(z, 18.0f, 0.0f, 7.0f, 0.0f);   z += 18.0f;

    // --- Section 7: Elevated narrow platforms with obstacles ---
    z += 6.0f;
    addSeg(z, 16.0f, 1.5f, 4.0f, -1.5f);  z += 16.0f;
    addObs(z - 8.0f, -1.5f, 1.5f, 0.7f, 1.7f, 0.7f, 1);
    z += 5.0f;
    addSeg(z, 16.0f, 2.0f, 4.0f, 1.5f);   z += 16.0f;
    addObs(z - 8.0f, 1.5f, 2.0f, 0.7f, 1.7f, 0.7f, 2);
    z += 5.0f;
    addSeg(z, 16.0f, 1.5f, 4.0f, 0.0f);   z += 16.0f;

    // --- Section 8: Descending platforms (back to ground) ---
    z += 6.0f;
    addSeg(z, 14.0f, 1.0f, 5.0f, 0.0f);   z += 14.0f;
    z += 5.0f;
    addSeg(z, 14.0f, 0.5f, 5.5f, 0.0f);   z += 14.0f;
    z += 5.0f;
    addSeg(z, 14.0f, 0.0f, 6.0f, 0.0f);   z += 14.0f;

    // --- Section 9: Final challenge (easier - wider platforms) ---
    z += 6.0f;
    addSeg(z, 16.0f, 0.0f, 5.5f, -1.5f);  z += 16.0f;
    z += 5.0f;
    addSeg(z, 16.0f, 0.0f, 5.5f,  1.5f);  z += 16.0f;
    z += 5.0f;
    addSeg(z, 16.0f, 0.0f, 5.5f, -1.0f);  z += 16.0f;
    z += 5.0f;
    addSeg(z, 16.0f, 0.0f, 5.5f,  1.0f);  z += 16.0f;
    z += 5.0f;
    addSeg(z, 16.0f, 0.0f, 6.0f,  0.0f);  z += 16.0f;

    // --- Section 10: Final corridor with mixed obstacles (easier - less dense) ---
    z += 6.0f;
    addSeg(z, 40.0f, 0.0f, 7.0f, 0.0f);   z += 40.0f;
    // Less dense pattern
    addObs(z - 35.0f, -2.5f, 0.0f, 0.8f, 1.8f, 0.8f, 0);
    addObs(z - 25.0f, 0.0f, 0.0f, 1.0f, 2.0f, 1.0f, 1);
    addObs(z - 15.0f, 2.5f, 0.0f, 0.8f, 1.8f, 0.8f, 2);
    addObs(z - 5.0f, -2.0f, 0.0f, 0.8f, 1.8f, 0.8f, 0);

    // --- Victory platform (wide, safe) ---
    z += 6.0f;
    addSeg(z, 35.0f, 0.0f, 8.0f, 0.0f);   z += 35.0f;

    lv.totalLength = z;
    
    // Start zone: Level 2 - Denser industrial pylons, offset accents
    lv.start.spawnZ = 2.0f;
    lv.start.gateZ = 0.0f;
    lv.start.zoneDepth = 10.0f;
    lv.start.style = StartStyle::IndustrialPylons;
    lv.start.width = 7.0f;
    lv.start.xOffset = 0.3f;  // Slight offset accent
    lv.start.topY = 0.0f;
    lv.start.pylonSpacing = 1.8f;  // Denser spacing
    lv.start.glowIntensity = 1.1f;
    lv.start.stripeCount = 6;
    
    // Finish zone: Level 2 - Segmented pylons + mild lateral offset
    lv.finish.startZ = z - 30.0f;
    lv.finish.endZ = z - 5.0f;
    lv.finish.style = FinishStyle::SegmentedPylons;
    lv.finish.width = 7.0f;
    lv.finish.xOffset = 0.5f;  // Mild lateral offset
    lv.finish.topY = 0.0f;
    lv.finish.glowIntensity = 1.2f;
    lv.finish.hasRunway = true;
    
    AssignVariantsHelper(lv);
    return lv;
}

const Level g_level2 = BuildLevel2();

Level BuildLevel3() {
    Level lv{};

    auto addSeg = [&](float startZ, float length, float topY, float width, float xOffset) {
        if (lv.segmentCount >= kMaxSegments) return;
        auto& s = lv.segments[lv.segmentCount++];
        s.startZ = startZ;
        s.length = length;
        s.topY = topY;
        s.width = width;
        s.xOffset = xOffset;
    };

    auto addObs = [&](float z, float x, float y, float sx, float sy, float sz, int col) {
        if (lv.obstacleCount >= kMaxObstacles) return;
        auto& o = lv.obstacles[lv.obstacleCount++];
        o.z = z; o.x = x; o.y = y;
        o.sizeX = sx; o.sizeY = sy; o.sizeZ = sz;
        o.colorIndex = col;
    };

    // ===== LEVEL 3: "Precision Gauntlet" =====
    // Design: ~750 units long, focuses on precision jumping and timing.
    // Features: Tighter gaps, more height variation, complex obstacle patterns.

    float z = 0.0f;

    // --- Section 1: Fast intro (easier, no early obstacles) ---
    addSeg(z, 50.0f, 0.0f, 8.0f, 0.0f);   z += 50.0f;
    z += 3.0f;
    addSeg(z, 40.0f, 0.0f, 7.5f, 0.0f);   z += 40.0f;
    z += 3.0f;
    addSeg(z, 35.0f, 0.0f, 7.0f, 0.0f);   z += 35.0f;
    // No early obstacles - let player get comfortable

    // --- Section 2: Precision gap sequence ---
    z += 5.0f;
    addSeg(z, 12.0f, 0.0f, 6.0f, 0.0f);   z += 12.0f;
    z += 6.0f;  // gap
    addSeg(z, 12.0f, 0.0f, 6.0f, 0.0f);   z += 12.0f;
    z += 6.0f;  // gap
    addSeg(z, 12.0f, 0.0f, 5.5f, 0.0f);   z += 12.0f;
    z += 7.0f;  // gap
    addSeg(z, 12.0f, 0.0f, 5.5f, 0.0f);   z += 12.0f;

    // --- Section 3: Height variation with obstacles ---
    z += 6.0f;
    addSeg(z, 18.0f, 0.5f, 6.0f, 0.0f);   z += 18.0f;
    addObs(z - 9.0f, -2.0f, 0.5f, 0.9f, 1.9f, 0.9f, 1);
    z += 5.0f;
    addSeg(z, 18.0f, 1.2f, 5.5f, 1.0f);   z += 18.0f;
    addObs(z - 9.0f, 1.0f, 1.2f, 0.9f, 1.9f, 0.9f, 2);
    z += 5.0f;
    addSeg(z, 18.0f, 0.5f, 5.5f, -1.0f);  z += 18.0f;
    z += 5.0f;
    addSeg(z, 18.0f, 0.0f, 6.0f, 0.0f);   z += 18.0f;

    // --- Section 4: Zigzag (easier - wider platforms, fewer obstacles) ---
    z += 6.0f;
    addSeg(z, 16.0f, 0.0f, 5.5f, -1.5f);  z += 16.0f;
    z += 5.0f;
    addSeg(z, 16.0f, 0.0f, 5.5f, 1.5f);   z += 16.0f;
    addObs(z - 8.0f, 1.5f, 0.0f, 0.8f, 1.8f, 0.8f, 1);
    z += 5.0f;
    addSeg(z, 16.0f, 0.0f, 5.5f, -1.0f);  z += 16.0f;
    z += 5.0f;
    addSeg(z, 16.0f, 0.0f, 5.5f, 1.0f);   z += 16.0f;
    z += 5.0f;
    addSeg(z, 16.0f, 0.0f, 6.0f, 0.0f);   z += 16.0f;

    // --- Section 5: Elevated platform sequence ---
    z += 6.0f;
    addSeg(z, 16.0f, 1.5f, 5.0f, 0.0f);   z += 16.0f;
    z += 6.0f;
    addSeg(z, 16.0f, 2.0f, 4.5f, 0.0f);   z += 16.0f;
    addObs(z - 8.0f, 0.0f, 2.0f, 1.0f, 2.0f, 1.0f, 0);
    z += 6.0f;
    addSeg(z, 16.0f, 1.5f, 5.0f, 0.0f);   z += 16.0f;
    z += 6.0f;
    addSeg(z, 16.0f, 1.0f, 5.5f, 0.0f);   z += 16.0f;

    // --- Section 6: Complex obstacle maze (easier - less dense) ---
    z += 6.0f;
    addSeg(z, 60.0f, 0.0f, 7.0f, 0.0f);   z += 60.0f;
    // Less dense pattern - more spacing
    addObs(z - 55.0f, -2.5f, 0.0f, 0.85f, 1.8f, 0.85f, 0);
    addObs(z - 45.0f, 0.0f, 0.0f, 1.0f, 2.0f, 1.0f, 1);
    addObs(z - 35.0f, 2.5f, 0.0f, 0.85f, 1.8f, 0.85f, 2);
    addObs(z - 25.0f, -2.0f, 0.0f, 0.85f, 1.8f, 0.85f, 0);
    addObs(z - 15.0f, 2.0f, 0.0f, 0.85f, 1.8f, 0.85f, 1);
    addObs(z - 5.0f, 0.0f, 0.0f, 1.0f, 2.0f, 1.0f, 2);

    // --- Section 7: Big gap challenge ---
    z += 7.0f;  // gap
    addSeg(z, 14.0f, 0.0f, 5.5f, 0.0f);   z += 14.0f;
    z += 8.0f;  // gap
    addSeg(z, 14.0f, 0.0f, 5.5f, 0.0f);   z += 14.0f;
    z += 9.0f;  // bigger gap
    addSeg(z, 16.0f, 0.0f, 5.5f, 0.0f);   z += 16.0f;

    // --- Section 8: Final precision sequence (easier - wider) ---
    z += 6.0f;
    addSeg(z, 16.0f, 0.5f, 5.5f, -1.0f);  z += 16.0f;
    z += 5.0f;
    addSeg(z, 16.0f, 1.0f, 5.5f, 1.0f);   z += 16.0f;
    addObs(z - 8.0f, 1.0f, 1.0f, 0.7f, 1.7f, 0.7f, 2);
    z += 5.0f;
    addSeg(z, 16.0f, 0.5f, 5.5f, 0.0f);  z += 16.0f;
    z += 5.0f;
    addSeg(z, 16.0f, 0.0f, 6.0f, 0.0f);   z += 16.0f;

    // --- Section 9: Final corridor ---
    z += 6.0f;
    addSeg(z, 45.0f, 0.0f, 7.0f, 0.0f);   z += 45.0f;
    // Final obstacle pattern
    addObs(z - 40.0f, -2.0f, 0.0f, 0.8f, 1.8f, 0.8f, 0);
    addObs(z - 40.0f, 2.0f, 0.0f, 0.8f, 1.8f, 0.8f, 0);
    addObs(z - 30.0f, 0.0f, 0.0f, 1.0f, 2.0f, 1.0f, 1);
    addObs(z - 20.0f, -2.5f, 0.0f, 0.8f, 1.8f, 0.8f, 2);
    addObs(z - 20.0f, 2.5f, 0.0f, 0.8f, 1.8f, 0.8f, 2);
    addObs(z - 10.0f, 0.0f, 0.0f, 1.0f, 2.0f, 1.0f, 0);

    // --- Victory platform ---
    z += 6.0f;
    addSeg(z, 35.0f, 0.0f, 8.0f, 0.0f);   z += 35.0f;

    lv.totalLength = z;
    
    // Start zone: Level 3 - Precision-focused corridor-like start
    lv.start.spawnZ = 2.0f;
    lv.start.gateZ = 0.0f;
    lv.start.zoneDepth = 9.0f;
    lv.start.style = StartStyle::PrecisionCorridor;
    lv.start.width = 5.5f;  // Tighter corridor
    lv.start.xOffset = 0.0f;
    lv.start.topY = 0.0f;
    lv.start.pylonSpacing = 2.2f;
    lv.start.glowIntensity = 1.3f;
    lv.start.stripeCount = 7;  // More stripes for precision feel
    
    // Finish zone: Level 3 - Tighter precision corridor + animated chevrons
    lv.finish.startZ = z - 28.0f;
    lv.finish.endZ = z - 5.0f;
    lv.finish.style = FinishStyle::PrecisionCorridor;
    lv.finish.width = 5.5f;  // Tighter than previous levels
    lv.finish.xOffset = 0.0f;
    lv.finish.topY = 0.0f;
    lv.finish.glowIntensity = 1.4f;
    lv.finish.hasRunway = true;
    
    AssignVariantsHelper(lv);
    return lv;
}

const Level g_level3 = BuildLevel3();

Level BuildLevel4() {
    Level lv{};

    auto addSeg = [&](float startZ, float length, float topY, float width, float xOffset) {
        if (lv.segmentCount >= kMaxSegments) return;
        auto& s = lv.segments[lv.segmentCount++];
        s.startZ = startZ;
        s.length = length;
        s.topY = topY;
        s.width = width;
        s.xOffset = xOffset;
    };

    auto addObs = [&](float z, float x, float y, float sx, float sy, float sz, int col) {
        if (lv.obstacleCount >= kMaxObstacles) return;
        auto& o = lv.obstacles[lv.obstacleCount++];
        o.z = z; o.x = x; o.y = y;
        o.sizeX = sx; o.sizeY = sy; o.sizeZ = sz;
        o.colorIndex = col;
    };

    // ===== LEVEL 4: "Stage 2 - Level 1" =====
    // Design: ~800 units long, introduces new challenges for Stage 2.
    // Features: Wider variety of platform sizes, more complex patterns.

    float z = 0.0f;

    // --- Section 1: Stage 2 intro (easier start) ---
    addSeg(z, 40.0f, 0.0f, 8.0f, 0.0f);   z += 40.0f;
    z += 4.0f;
    addSeg(z, 30.0f, 0.0f, 7.5f, 0.0f);   z += 30.0f;
    // Single side obstacle (easier than both sides)
    addObs(z - 15.0f, -2.5f, 0.0f, 0.9f, 1.9f, 0.9f, 0);

    // --- Section 2: Variable width platforms ---
    z += 6.0f;
    addSeg(z, 20.0f, 0.0f, 6.0f, 0.0f);   z += 20.0f;
    z += 5.0f;
    addSeg(z, 20.0f, 0.0f, 5.0f, 0.0f);   z += 20.0f;
    addObs(z - 10.0f, 0.0f, 0.0f, 1.0f, 2.0f, 1.0f, 0);
    z += 5.0f;
    addSeg(z, 20.0f, 0.0f, 4.5f, 0.0f);   z += 20.0f;
    z += 5.0f;
    addSeg(z, 20.0f, 0.0f, 5.5f, 0.0f);   z += 20.0f;

    // --- Section 3: Height variation sequence ---
    z += 6.0f;
    addSeg(z, 16.0f, 0.5f, 6.0f, 0.0f);   z += 16.0f;
    z += 5.0f;
    addSeg(z, 16.0f, 1.2f, 5.5f, 0.0f);   z += 16.0f;
    addObs(z - 8.0f, -2.0f, 1.2f, 0.8f, 1.8f, 0.8f, 1);
    z += 5.0f;
    addSeg(z, 16.0f, 1.8f, 5.0f, 0.0f);   z += 16.0f;
    z += 5.0f;
    addSeg(z, 16.0f, 1.2f, 5.5f, 0.0f);   z += 16.0f;
    z += 5.0f;
    addSeg(z, 16.0f, 0.5f, 6.0f, 0.0f);   z += 16.0f;
    z += 5.0f;
    addSeg(z, 16.0f, 0.0f, 6.5f, 0.0f);   z += 16.0f;

    // --- Section 4: Offset platform challenge (easier - wider) ---
    z += 6.0f;
    addSeg(z, 16.0f, 0.0f, 5.5f, -2.0f);  z += 16.0f;
    z += 5.0f;
    addSeg(z, 16.0f, 0.0f, 5.5f, 2.0f);   z += 16.0f;
    addObs(z - 8.0f, 2.0f, 0.0f, 0.8f, 1.8f, 0.8f, 2);
    z += 5.0f;
    addSeg(z, 16.0f, 0.0f, 5.5f, -1.5f);  z += 16.0f;
    z += 5.0f;
    addSeg(z, 16.0f, 0.0f, 5.5f, 1.5f);   z += 16.0f;
    z += 5.0f;
    addSeg(z, 16.0f, 0.0f, 6.0f, 0.0f);   z += 16.0f;

    // --- Section 5: Long corridor with mixed obstacles ---
    z += 6.0f;
    addSeg(z, 65.0f, 0.0f, 7.5f, 0.0f);   z += 65.0f;
    // Complex pattern
    addObs(z - 60.0f, 0.0f, 0.0f, 1.0f, 2.0f, 1.0f, 0);
    addObs(z - 52.0f, -2.5f, 0.0f, 0.85f, 1.8f, 0.85f, 1);
    addObs(z - 52.0f, 2.5f, 0.0f, 0.85f, 1.8f, 0.85f, 1);
    addObs(z - 44.0f, 0.0f, 0.0f, 1.0f, 2.0f, 1.0f, 2);
    addObs(z - 36.0f, -2.0f, 0.0f, 0.85f, 1.8f, 0.85f, 0);
    addObs(z - 36.0f, 2.0f, 0.0f, 0.85f, 1.8f, 0.85f, 0);
    addObs(z - 28.0f, 0.0f, 0.0f, 1.0f, 2.0f, 1.0f, 1);
    addObs(z - 20.0f, -2.5f, 0.0f, 0.85f, 1.8f, 0.85f, 2);
    addObs(z - 20.0f, 2.5f, 0.0f, 0.85f, 1.8f, 0.85f, 2);
    addObs(z - 12.0f, 0.0f, 0.0f, 1.0f, 2.0f, 1.0f, 0);
    addObs(z - 4.0f, -2.0f, 0.0f, 0.85f, 1.8f, 0.85f, 1);
    addObs(z - 4.0f, 2.0f, 0.0f, 0.85f, 1.8f, 0.85f, 1);

    // --- Section 6: Gap sequence ---
    z += 7.0f;  // gap
    addSeg(z, 15.0f, 0.0f, 6.0f, 0.0f);   z += 15.0f;
    z += 7.0f;  // gap
    addSeg(z, 15.0f, 0.0f, 5.5f, 0.0f);   z += 15.0f;
    z += 8.0f;  // gap
    addSeg(z, 15.0f, 0.0f, 5.5f, 0.0f);   z += 15.0f;
    z += 9.0f;  // bigger gap
    addSeg(z, 18.0f, 0.0f, 6.0f, 0.0f);   z += 18.0f;

    // --- Section 7: Elevated finale ---
    z += 6.0f;
    addSeg(z, 18.0f, 1.5f, 5.5f, 0.0f);   z += 18.0f;
    addObs(z - 9.0f, 0.0f, 1.5f, 1.0f, 2.0f, 1.0f, 0);
    z += 5.0f;
    addSeg(z, 18.0f, 2.2f, 5.0f, 0.0f);   z += 18.0f;
    z += 5.0f;
    addSeg(z, 18.0f, 1.5f, 5.5f, 0.0f);   z += 18.0f;
    z += 5.0f;
    addSeg(z, 18.0f, 0.8f, 6.0f, 0.0f);   z += 18.0f;
    z += 5.0f;
    addSeg(z, 18.0f, 0.0f, 6.5f, 0.0f);   z += 18.0f;

    // --- Section 8: Final challenge ---
    z += 6.0f;
    addSeg(z, 50.0f, 0.0f, 7.0f, 0.0f);   z += 50.0f;
    // Final obstacle pattern
    addObs(z - 45.0f, -2.5f, 0.0f, 0.8f, 1.8f, 0.8f, 0);
    addObs(z - 45.0f, 2.5f, 0.0f, 0.8f, 1.8f, 0.8f, 0);
    addObs(z - 35.0f, 0.0f, 0.0f, 1.0f, 2.0f, 1.0f, 1);
    addObs(z - 25.0f, -2.0f, 0.0f, 0.8f, 1.8f, 0.8f, 2);
    addObs(z - 25.0f, 2.0f, 0.0f, 0.8f, 1.8f, 0.8f, 2);
    addObs(z - 15.0f, 0.0f, 0.0f, 1.0f, 2.0f, 1.0f, 0);
    addObs(z - 5.0f, -2.5f, 0.0f, 0.8f, 1.8f, 0.8f, 1);
    addObs(z - 5.0f, 2.5f, 0.0f, 0.8f, 1.8f, 0.8f, 1);

    // --- Victory platform ---
    z += 6.0f;
    addSeg(z, 40.0f, 0.0f, 8.0f, 0.0f);   z += 40.0f;

    lv.totalLength = z;
    
    // Start zone: Level 4 - Portal-esque or ringed launch frame
    lv.start.spawnZ = 2.0f;
    lv.start.gateZ = 0.0f;
    lv.start.zoneDepth = 12.0f;
    lv.start.style = StartStyle::RingedLaunch;
    lv.start.width = 8.0f;
    lv.start.xOffset = 0.0f;
    lv.start.topY = 0.0f;
    lv.start.pylonSpacing = 2.0f;
    lv.start.glowIntensity = 1.6f;  // Strong visual identity
    lv.start.stripeCount = 5;
    lv.start.ringCount = 3.0f;
    
    // Finish zone: Level 4 - Multi-ring/portal-style finale with strongest visual payoff
    lv.finish.startZ = z - 35.0f;
    lv.finish.endZ = z - 5.0f;
    lv.finish.style = FinishStyle::MultiRingPortal;
    lv.finish.width = 8.0f;
    lv.finish.xOffset = 0.0f;
    lv.finish.topY = 0.0f;
    lv.finish.ringCount = 4.0f;
    lv.finish.glowIntensity = 1.8f;  // Strongest glow
    lv.finish.hasRunway = true;
    
    AssignVariantsHelper(lv);
    return lv;
}

const Level g_level4 = BuildLevel4();

Level BuildLevel5() {
    Level lv{};

    auto addSeg = [&](float startZ, float length, float topY, float width, float xOffset) {
        if (lv.segmentCount >= kMaxSegments) return;
        auto& s = lv.segments[lv.segmentCount++];
        s.startZ = startZ;
        s.length = length;
        s.topY = topY;
        s.width = width;
        s.xOffset = xOffset;
    };

    auto addObs = [&](float z, float x, float y, float sx, float sy, float sz, int col) {
        if (lv.obstacleCount >= kMaxObstacles) return;
        auto& o = lv.obstacles[lv.obstacleCount++];
        o.z = z; o.x = x; o.y = y;
        o.sizeX = sx; o.sizeY = sy; o.sizeZ = sz;
        o.colorIndex = col;
    };

    // ===== LEVEL 5: "Stage 2 - Level 2" =====
    // Design: ~850 units long, builds on Stage 2 Level 1.
    // Features: More complex patterns, tighter navigation, increased difficulty.

    float z = 0.0f;

    // --- Section 1: Quick start with early challenge ---
    addSeg(z, 30.0f, 0.0f, 7.5f, 0.0f);   z += 30.0f;
    z += 4.0f;
    addSeg(z, 22.0f, 0.0f, 7.0f, 0.0f);   z += 22.0f;
    // Early center obstacle
    addObs(z - 11.0f, 0.0f, 0.0f, 1.1f, 2.1f, 1.1f, 0);

    // --- Section 2: Narrowing sequence (easier - fewer obstacles) ---
    z += 5.0f;
    addSeg(z, 20.0f, 0.0f, 6.5f, 0.0f);   z += 20.0f;
    z += 5.0f;
    addSeg(z, 20.0f, 0.0f, 6.0f, 0.0f);   z += 20.0f;
    addObs(z - 10.0f, -2.5f, 0.0f, 0.9f, 1.9f, 0.9f, 1);
    z += 5.0f;
    addSeg(z, 20.0f, 0.0f, 5.5f, 0.0f);   z += 20.0f;
    z += 5.0f;
    addSeg(z, 20.0f, 0.0f, 5.0f, 0.0f);   z += 20.0f;

    // --- Section 3: Height variation with side obstacles ---
    z += 6.0f;
    addSeg(z, 16.0f, 0.5f, 5.5f, 0.0f);   z += 16.0f;
    addObs(z - 8.0f, -2.2f, 0.5f, 0.85f, 1.85f, 0.85f, 1);
    z += 5.0f;
    addSeg(z, 16.0f, 1.2f, 5.0f, 1.0f);   z += 16.0f;
    addObs(z - 8.0f, 1.0f, 1.2f, 0.85f, 1.85f, 0.85f, 2);
    z += 5.0f;
    addSeg(z, 16.0f, 1.8f, 4.5f, -1.0f);  z += 16.0f;
    z += 5.0f;
    addSeg(z, 16.0f, 1.2f, 5.0f, 1.0f);   z += 16.0f;
    z += 5.0f;
    addSeg(z, 16.0f, 0.5f, 5.5f, 0.0f);   z += 16.0f;
    z += 5.0f;
    addSeg(z, 16.0f, 0.0f, 6.0f, 0.0f);   z += 16.0f;

    // --- Section 4: Complex zigzag pattern ---
    z += 6.0f;
    addSeg(z, 14.0f, 0.0f, 4.5f, -2.5f);  z += 14.0f;
    addObs(z - 7.0f, -2.5f, 0.0f, 0.8f, 1.8f, 0.8f, 0);
    z += 5.0f;
    addSeg(z, 14.0f, 0.0f, 4.5f, 2.5f);   z += 14.0f;
    addObs(z - 7.0f, 2.5f, 0.0f, 0.8f, 1.8f, 0.8f, 1);
    z += 5.0f;
    addSeg(z, 14.0f, 0.0f, 4.5f, -2.0f);  z += 14.0f;
    z += 5.0f;
    addSeg(z, 14.0f, 0.0f, 4.5f, 2.0f);   z += 14.0f;
    addObs(z - 7.0f, 2.0f, 0.0f, 0.8f, 1.8f, 0.8f, 2);
    z += 5.0f;
    addSeg(z, 14.0f, 0.0f, 4.5f, -1.5f);  z += 14.0f;
    z += 5.0f;
    addSeg(z, 14.0f, 0.0f, 4.5f, 1.5f);   z += 14.0f;
    z += 5.0f;
    addSeg(z, 14.0f, 0.0f, 5.0f, 0.0f);   z += 14.0f;

    // --- Section 5: Long corridor with obstacles (easier - less dense) ---
    z += 6.0f;
    addSeg(z, 70.0f, 0.0f, 7.0f, 0.0f);   z += 70.0f;
    // Less dense pattern
    addObs(z - 65.0f, -2.5f, 0.0f, 0.85f, 1.8f, 0.85f, 0);
    addObs(z - 55.0f, 0.0f, 0.0f, 1.0f, 2.0f, 1.0f, 1);
    addObs(z - 45.0f, 2.5f, 0.0f, 0.85f, 1.8f, 0.85f, 2);
    addObs(z - 35.0f, -2.0f, 0.0f, 0.85f, 1.8f, 0.85f, 0);
    addObs(z - 25.0f, 0.0f, 0.0f, 1.0f, 2.0f, 1.0f, 1);
    addObs(z - 15.0f, 2.5f, 0.0f, 0.85f, 1.8f, 0.85f, 2);
    addObs(z - 5.0f, -2.0f, 0.0f, 0.85f, 1.8f, 0.85f, 0);

    // --- Section 6: Gap sequence with varying difficulty ---
    z += 7.0f;  // gap
    addSeg(z, 15.0f, 0.0f, 5.5f, 0.0f);   z += 15.0f;
    z += 7.0f;  // gap
    addSeg(z, 15.0f, 0.0f, 5.0f, 0.0f);   z += 15.0f;
    z += 8.0f;  // gap
    addSeg(z, 15.0f, 0.0f, 5.0f, 0.0f);   z += 15.0f;
    z += 9.0f;  // bigger gap
    addSeg(z, 18.0f, 0.0f, 5.5f, 0.0f);   z += 18.0f;

    // --- Section 7: Elevated finale sequence ---
    z += 6.0f;
    addSeg(z, 18.0f, 1.5f, 5.0f, 0.0f);   z += 18.0f;
    addObs(z - 9.0f, 0.0f, 1.5f, 1.0f, 2.0f, 1.0f, 0);
    z += 5.0f;
    addSeg(z, 18.0f, 2.2f, 4.5f, 0.0f);   z += 18.0f;
    z += 5.0f;
    addSeg(z, 18.0f, 1.5f, 5.0f, 0.0f);   z += 18.0f;
    z += 5.0f;
    addSeg(z, 18.0f, 0.8f, 5.5f, 0.0f);   z += 18.0f;
    z += 5.0f;
    addSeg(z, 18.0f, 0.0f, 6.0f, 0.0f);   z += 18.0f;

    // --- Section 8: Final challenge corridor ---
    z += 6.0f;
    addSeg(z, 55.0f, 0.0f, 7.0f, 0.0f);   z += 55.0f;
    // Final obstacle pattern
    addObs(z - 50.0f, -2.5f, 0.0f, 0.8f, 1.8f, 0.8f, 0);
    addObs(z - 50.0f, 2.5f, 0.0f, 0.8f, 1.8f, 0.8f, 0);
    addObs(z - 40.0f, 0.0f, 0.0f, 1.0f, 2.0f, 1.0f, 1);
    addObs(z - 30.0f, -2.0f, 0.0f, 0.8f, 1.8f, 0.8f, 2);
    addObs(z - 30.0f, 2.0f, 0.0f, 0.8f, 1.8f, 0.8f, 2);
    addObs(z - 20.0f, 0.0f, 0.0f, 1.0f, 2.0f, 1.0f, 0);
    addObs(z - 10.0f, -2.5f, 0.0f, 0.8f, 1.8f, 0.8f, 1);
    addObs(z - 10.0f, 2.5f, 0.0f, 0.8f, 1.8f, 0.8f, 1);

    // --- Victory platform ---
    z += 6.0f;
    addSeg(z, 40.0f, 0.0f, 8.0f, 0.0f);   z += 40.0f;

    lv.totalLength = z;
    return lv;
}

const Level g_level5 = BuildLevel5();

Level BuildLevel6() {
    Level lv{};

    auto addSeg = [&](float startZ, float length, float topY, float width, float xOffset) {
        if (lv.segmentCount >= kMaxSegments) return;
        auto& s = lv.segments[lv.segmentCount++];
        s.startZ = startZ;
        s.length = length;
        s.topY = topY;
        s.width = width;
        s.xOffset = xOffset;
    };

    auto addObs = [&](float z, float x, float y, float sx, float sy, float sz, int col) {
        if (lv.obstacleCount >= kMaxObstacles) return;
        auto& o = lv.obstacles[lv.obstacleCount++];
        o.z = z; o.x = x; o.y = y;
        o.sizeX = sx; o.sizeY = sy; o.sizeZ = sz;
        o.colorIndex = col;
    };

    // ===== LEVEL 6: "Stage 2 - Level 3" =====
    // Design: ~900 units long, final level of Stage 2.
    // Features: Most challenging level of Stage 2, combines all previous mechanics.

    float z = 0.0f;

    // --- Section 1: Stage 2 finale intro (easier start) ---
    addSeg(z, 38.0f, 0.0f, 7.5f, 0.0f);   z += 38.0f;
    z += 4.0f;
    addSeg(z, 28.0f, 0.0f, 7.0f, 0.0f);   z += 28.0f;
    // Single side obstacle (easier than multiple)
    addObs(z - 14.0f, -2.5f, 0.0f, 0.9f, 1.9f, 0.9f, 0);

    // --- Section 2: Progressive narrowing with obstacles ---
    z += 5.0f;
    addSeg(z, 18.0f, 0.0f, 6.5f, 0.0f);   z += 18.0f;
    addObs(z - 9.0f, -2.5f, 0.0f, 0.9f, 1.9f, 0.9f, 0);
    z += 5.0f;
    addSeg(z, 18.0f, 0.0f, 6.0f, 0.0f);   z += 18.0f;
    addObs(z - 9.0f, 2.5f, 0.0f, 0.9f, 1.9f, 0.9f, 1);
    z += 5.0f;
    addSeg(z, 18.0f, 0.0f, 5.5f, 0.0f);   z += 18.0f;
    addObs(z - 9.0f, 0.0f, 0.0f, 1.0f, 2.0f, 1.0f, 2);
    z += 5.0f;
    addSeg(z, 18.0f, 0.0f, 5.0f, 0.0f);   z += 18.0f;
    addObs(z - 9.0f, -2.0f, 0.0f, 0.9f, 1.9f, 0.9f, 0);
    addObs(z - 9.0f, 2.0f, 0.0f, 0.9f, 1.9f, 0.9f, 1);
    z += 5.0f;
    addSeg(z, 18.0f, 0.0f, 4.5f, 0.0f);   z += 18.0f;

    // --- Section 3: Complex height variation ---
    z += 6.0f;
    addSeg(z, 16.0f, 0.5f, 5.5f, 0.0f);   z += 16.0f;
    addObs(z - 8.0f, -2.2f, 0.5f, 0.85f, 1.85f, 0.85f, 1);
    z += 5.0f;
    addSeg(z, 16.0f, 1.2f, 5.0f, 1.0f);   z += 16.0f;
    addObs(z - 8.0f, 1.0f, 1.2f, 0.85f, 1.85f, 0.85f, 2);
    z += 5.0f;
    addSeg(z, 16.0f, 1.8f, 4.5f, -1.0f);  z += 16.0f;
    addObs(z - 8.0f, -1.0f, 1.8f, 0.85f, 1.85f, 0.85f, 0);
    z += 5.0f;
    addSeg(z, 16.0f, 2.2f, 4.0f, 1.0f);   z += 16.0f;
    z += 5.0f;
    addSeg(z, 16.0f, 1.5f, 4.5f, 0.0f);   z += 16.0f;
    z += 5.0f;
    addSeg(z, 16.0f, 0.8f, 5.0f, 0.0f);   z += 16.0f;
    z += 5.0f;
    addSeg(z, 16.0f, 0.0f, 5.5f, 0.0f);   z += 16.0f;

    // --- Section 4: Advanced zigzag with obstacles ---
    z += 6.0f;
    addSeg(z, 14.0f, 0.0f, 4.5f, -2.5f);  z += 14.0f;
    addObs(z - 7.0f, -2.5f, 0.0f, 0.8f, 1.8f, 0.8f, 0);
    z += 5.0f;
    addSeg(z, 14.0f, 0.0f, 4.5f, 2.5f);   z += 14.0f;
    addObs(z - 7.0f, 2.5f, 0.0f, 0.8f, 1.8f, 0.8f, 1);
    z += 5.0f;
    addSeg(z, 14.0f, 0.0f, 4.5f, -2.0f);  z += 14.0f;
    addObs(z - 7.0f, -2.0f, 0.0f, 0.8f, 1.8f, 0.8f, 2);
    z += 5.0f;
    addSeg(z, 14.0f, 0.0f, 4.5f, 2.0f);   z += 14.0f;
    addObs(z - 7.0f, 2.0f, 0.0f, 0.8f, 1.8f, 0.8f, 0);
    z += 5.0f;
    addSeg(z, 14.0f, 0.0f, 4.5f, -1.5f);  z += 14.0f;
    z += 5.0f;
    addSeg(z, 14.0f, 0.0f, 4.5f, 1.5f);   z += 14.0f;
    addObs(z - 7.0f, 1.5f, 0.0f, 0.8f, 1.8f, 0.8f, 1);
    z += 5.0f;
    addSeg(z, 14.0f, 0.0f, 5.0f, 0.0f);   z += 14.0f;

    // --- Section 5: Very long corridor with very dense obstacles ---
    z += 6.0f;
    addSeg(z, 80.0f, 0.0f, 7.0f, 0.0f);   z += 80.0f;
    // Very dense pattern - ultimate challenge
    addObs(z - 75.0f, 0.0f, 0.0f, 1.0f, 2.0f, 1.0f, 0);
    addObs(z - 70.0f, -2.5f, 0.0f, 0.85f, 1.8f, 0.85f, 1);
    addObs(z - 70.0f, 2.5f, 0.0f, 0.85f, 1.8f, 0.85f, 1);
    addObs(z - 65.0f, 0.0f, 0.0f, 1.0f, 2.0f, 1.0f, 2);
    addObs(z - 60.0f, -2.0f, 0.0f, 0.85f, 1.8f, 0.85f, 0);
    addObs(z - 60.0f, 2.0f, 0.0f, 0.85f, 1.8f, 0.85f, 0);
    addObs(z - 55.0f, 0.0f, 0.0f, 1.0f, 2.0f, 1.0f, 1);
    addObs(z - 50.0f, -2.5f, 0.0f, 0.85f, 1.8f, 0.85f, 2);
    addObs(z - 50.0f, 2.5f, 0.0f, 0.85f, 1.8f, 0.85f, 2);
    addObs(z - 45.0f, 0.0f, 0.0f, 1.0f, 2.0f, 1.0f, 0);
    addObs(z - 40.0f, -2.0f, 0.0f, 0.85f, 1.8f, 0.85f, 1);
    addObs(z - 40.0f, 2.0f, 0.0f, 0.85f, 1.8f, 0.85f, 1);
    addObs(z - 35.0f, 0.0f, 0.0f, 1.0f, 2.0f, 1.0f, 2);
    addObs(z - 30.0f, -2.5f, 0.0f, 0.85f, 1.8f, 0.85f, 0);
    addObs(z - 30.0f, 2.5f, 0.0f, 0.85f, 1.8f, 0.85f, 0);
    addObs(z - 25.0f, 0.0f, 0.0f, 1.0f, 2.0f, 1.0f, 1);
    addObs(z - 20.0f, -2.0f, 0.0f, 0.85f, 1.8f, 0.85f, 2);
    addObs(z - 20.0f, 2.0f, 0.0f, 0.85f, 1.8f, 0.85f, 2);
    addObs(z - 15.0f, 0.0f, 0.0f, 1.0f, 2.0f, 1.0f, 0);
    addObs(z - 10.0f, -2.5f, 0.0f, 0.85f, 1.8f, 0.85f, 1);
    addObs(z - 10.0f, 2.5f, 0.0f, 0.85f, 1.8f, 0.85f, 1);
    addObs(z - 5.0f, 0.0f, 0.0f, 1.0f, 2.0f, 1.0f, 2);

    // --- Section 6: Challenging gap sequence ---
    z += 7.0f;  // gap
    addSeg(z, 15.0f, 0.0f, 5.5f, 0.0f);   z += 15.0f;
    z += 8.0f;  // gap
    addSeg(z, 15.0f, 0.0f, 5.0f, 0.0f);   z += 15.0f;
    z += 9.0f;  // gap
    addSeg(z, 15.0f, 0.0f, 5.0f, 0.0f);   z += 15.0f;
    z += 10.0f; // bigger gap
    addSeg(z, 18.0f, 0.0f, 5.5f, 0.0f);   z += 18.0f;

    // --- Section 7: Ultimate elevated finale ---
    z += 6.0f;
    addSeg(z, 18.0f, 1.5f, 5.0f, 0.0f);   z += 18.0f;
    addObs(z - 9.0f, 0.0f, 1.5f, 1.0f, 2.0f, 1.0f, 0);
    z += 5.0f;
    addSeg(z, 18.0f, 2.2f, 4.5f, 0.0f);   z += 18.0f;
    addObs(z - 9.0f, -2.0f, 2.2f, 0.85f, 1.85f, 0.85f, 1);
    addObs(z - 9.0f, 2.0f, 2.2f, 0.85f, 1.85f, 0.85f, 2);
    z += 5.0f;
    addSeg(z, 18.0f, 1.5f, 5.0f, 0.0f);   z += 18.0f;
    z += 5.0f;
    addSeg(z, 18.0f, 0.8f, 5.5f, 0.0f);   z += 18.0f;
    z += 5.0f;
    addSeg(z, 18.0f, 0.0f, 6.0f, 0.0f);   z += 18.0f;

    // --- Section 8: Final challenge corridor ---
    z += 6.0f;
    addSeg(z, 60.0f, 0.0f, 7.0f, 0.0f);   z += 60.0f;
    // Final obstacle pattern - most challenging
    addObs(z - 55.0f, -2.5f, 0.0f, 0.8f, 1.8f, 0.8f, 0);
    addObs(z - 55.0f, 2.5f, 0.0f, 0.8f, 1.8f, 0.8f, 0);
    addObs(z - 45.0f, 0.0f, 0.0f, 1.0f, 2.0f, 1.0f, 1);
    addObs(z - 35.0f, -2.0f, 0.0f, 0.8f, 1.8f, 0.8f, 2);
    addObs(z - 35.0f, 2.0f, 0.0f, 0.8f, 1.8f, 0.8f, 2);
    addObs(z - 25.0f, 0.0f, 0.0f, 1.0f, 2.0f, 1.0f, 0);
    addObs(z - 15.0f, -2.5f, 0.0f, 0.8f, 1.8f, 0.8f, 1);
    addObs(z - 15.0f, 2.5f, 0.0f, 0.8f, 1.8f, 0.8f, 1);
    addObs(z - 5.0f, 0.0f, 0.0f, 1.0f, 2.0f, 1.0f, 2);

    // --- Victory platform ---
    z += 6.0f;
    addSeg(z, 45.0f, 0.0f, 8.0f, 0.0f);   z += 45.0f;

    lv.totalLength = z;
    return lv;
}

const Level g_level6 = BuildLevel6();

// Placeholder level (empty level for unimplemented levels)
Level BuildPlaceholderLevel() {
    Level lv{};
    // Empty level - just a starting platform
    lv.segments[0].startZ = 0.0f;
    lv.segments[0].length = 10.0f;
    lv.segments[0].topY = 0.0f;
    lv.segments[0].width = 8.0f;
    lv.segments[0].xOffset = 0.0f;
    lv.segmentCount = 1;
    lv.obstacleCount = 0;
    lv.totalLength = 10.0f;
    return lv;
}

const Level g_placeholderLevel = BuildPlaceholderLevel();

// Helper function inside namespace to assign variants
void AssignVariantsHelper(Level& lv) {
    // Assign segment variants
    for (int i = 0; i < lv.segmentCount; ++i) {
        auto& seg = lv.segments[i];
        
        // Create deterministic hash from segment properties
        // Using integer conversion to ensure consistent hashing
        uint32_t hash = static_cast<uint32_t>(seg.startZ * 10.0f) ^ 
                       static_cast<uint32_t>(seg.width * 100.0f) ^
                       static_cast<uint32_t>(seg.topY * 50.0f) ^
                       static_cast<uint32_t>(seg.length * 5.0f) ^
                       static_cast<uint32_t>(i * 17u);
        
        // Variant selection: 0=Standard, 1=Thin, 2=Thick, 3=Wide, 4=Narrow, 5=Glowing, 6=Matte, 7=Striped
        // Use width and height to influence variant choice
        if (seg.width < 5.0f) {
            seg.variantIndex = 4; // Narrow variant for narrow segments
        } else if (seg.width > 7.0f) {
            seg.variantIndex = 3; // Wide variant for wide segments
        } else if (seg.topY > 1.0f) {
            seg.variantIndex = 2; // Thick variant for elevated segments
        } else {
            seg.variantIndex = (hash % 8); // Cycle through all variants
        }
        
        // Height scale: vary visual height (0.7 to 1.2)
        seg.heightScale = 0.7f + ((hash / 8) % 6) * 0.1f;
        
        // Color tint: 0-2 for slight color variations
        seg.colorTint = (hash / 48) % 3;
    }
    
    // Assign obstacle variants
    for (int i = 0; i < lv.obstacleCount; ++i) {
        auto& obs = lv.obstacles[i];
        
        // Create deterministic hash from obstacle properties
        uint32_t hash = static_cast<uint32_t>(obs.z * 10.0f) ^
                       static_cast<uint32_t>(obs.x * 100.0f) ^
                       static_cast<uint32_t>(obs.y * 50.0f) ^
                       static_cast<uint32_t>(obs.sizeY * 30.0f) ^
                       static_cast<uint32_t>(i * 23u);
        
        // Shape selection based on size and position
        if (obs.sizeY > 2.0f) {
            obs.shape = ObstacleShape::Spike; // Tall obstacles = spikes
        } else if (obs.sizeX > obs.sizeZ * 1.5f) {
            obs.shape = ObstacleShape::Wall; // Wide obstacles = walls
        } else if (obs.sizeX < obs.sizeZ * 0.7f) {
            obs.shape = ObstacleShape::Cylinder; // Pill-shaped for narrow obstacles
        } else {
            // Cycle through shapes based on hash
            obs.shape = static_cast<ObstacleShape>((hash % 6));
        }
        
        // Rotation: 0, 45, 90, or 135 degrees
        obs.rotation = static_cast<float>((hash / 6) % 4) * 45.0f;
    }
}

}  // namespace

// Public interface - deterministic variant assignment based on segment/obstacle properties
// Same properties = same variant (ensures all players see identical visuals)
void AssignVariants(Level& lv) {
    // Implementation is in anonymous namespace above
    AssignVariantsHelper(lv);
}

const Level& GetLevel1() {
    return g_level1;
}

const Level& GetLevel2() {
    return g_level2;
}

const Level& GetLevel3() {
    return g_level3;
}

const Level& GetLevel4() {
    return g_level4;
}

const Level& GetLevel5() {
    return g_level5;
}

const Level& GetLevel6() {
    return g_level6;
}

int GetStageFromLevelIndex(int levelIndex) {
    if (levelIndex < 1 || levelIndex > 30) return 1;
    return ((levelIndex - 1) / 3) + 1;
}

int GetLevelInStageFromLevelIndex(int levelIndex) {
    if (levelIndex < 1 || levelIndex > 30) return 1;
    return ((levelIndex - 1) % 3) + 1;
}

int GetLevelIndexFromStageAndLevel(int stage, int levelInStage) {
    if (stage < 1 || stage > 10) return 1;
    if (levelInStage < 1 || levelInStage > 3) return 1;
    return (stage - 1) * 3 + levelInStage;
}

bool IsLevelImplemented(int levelIndex) {
    return levelIndex >= 1 && levelIndex <= 6;
}

const Level& GetLevelByIndex(int levelIndex) {
    if (levelIndex == 1) return GetLevel1();
    if (levelIndex == 2) return GetLevel2();
    if (levelIndex == 3) return GetLevel3();
    if (levelIndex == 4) return GetLevel4();
    if (levelIndex == 5) return GetLevel5();
    if (levelIndex == 6) return GetLevel6();
    // Return placeholder for unimplemented levels
    return g_placeholderLevel;
}

int FindSegmentUnder(const Level& level, const float playerZ, const float playerX, const float playerHalfW) {
    for (int i = 0; i < level.segmentCount; ++i) {
        const auto& s = level.segments[i];
        const float endZ = s.startZ + s.length;
        if (playerZ < s.startZ || playerZ > endZ) continue;
        const float segLeft = s.xOffset - s.width * 0.5f;
        const float segRight = s.xOffset + s.width * 0.5f;
        if (playerX + playerHalfW < segLeft || playerX - playerHalfW > segRight) continue;
        return i;
    }
    return -1;
}

bool CheckObstacleCollision(const Level& level, const Vector3 playerPos,
                            const float halfW, const float halfH, const float halfD) {
    for (int i = 0; i < level.obstacleCount; ++i) {
        const auto& o = level.obstacles[i];
        // AABB overlap test.
        const float oMinX = o.x - o.sizeX * 0.5f;
        const float oMaxX = o.x + o.sizeX * 0.5f;
        const float oMinY = o.y;
        const float oMaxY = o.y + o.sizeY;
        const float oMinZ = o.z - o.sizeZ * 0.5f;
        const float oMaxZ = o.z + o.sizeZ * 0.5f;

        const float pMinX = playerPos.x - halfW;
        const float pMaxX = playerPos.x + halfW;
        const float pMinY = playerPos.y - halfH;
        const float pMaxY = playerPos.y + halfH;
        const float pMinZ = playerPos.z - halfD;
        const float pMaxZ = playerPos.z + halfD;

        if (pMaxX > oMinX && pMinX < oMaxX &&
            pMaxY > oMinY && pMinY < oMaxY &&
            pMaxZ > oMinZ && pMinZ < oMaxZ) {
            return true;
        }
    }
    return false;
}

bool CheckFinishZoneCrossing(const Level& level, const float playerZ) {
    // If no finish zone (placeholder levels), fall back to totalLength check
    if (level.finish.style == FinishStyle::None) {
        return playerZ > level.totalLength;
    }
    // Player completes when crossing finish.endZ
    return playerZ > level.finish.endZ;
}

float GetSpawnZ(const Level& level) {
    // If no start zone (placeholder levels), use default spawn position
    if (level.start.style == StartStyle::None) {
        return 2.0f;  // Default spawn Z
    }
    // Return spawn Z from start zone
    return level.start.spawnZ;
}
