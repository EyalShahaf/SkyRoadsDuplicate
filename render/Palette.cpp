#include "render/Palette.hpp"

#include "core/Config.hpp"

namespace {
constexpr LevelPalette kPalettes[cfg::kPaletteCount] = {
    // Neon dusk — deep purple sky, cyan/magenta accents.
    LevelPalette{
        /* skyTop          */ Color{18, 10, 42, 255},
        /* skyBottom       */ Color{3, 4, 12, 255},
        /* voidTint        */ Color{6, 8, 20, 255},
        /* platformSide    */ Color{22, 32, 72, 255},
        /* platformTop     */ Color{30, 42, 100, 255},
        /* platformWire    */ Color{60, 130, 255, 255},
        /* laneOuter       */ Color{40, 110, 220, 255},
        /* laneCenter      */ Color{80, 200, 255, 255},
        /* laneGlow        */ Color{100, 220, 255, 255},
        /* streak          */ Color{150, 230, 255, 255},
        /* playerBody      */ Color{255, 56, 56, 255},
        /* playerWire      */ Color{255, 180, 180, 255},
        /* particle        */ Color{146, 234, 255, 255},
        /* uiPanel         */ Color{10, 12, 24, 170},
        /* uiText          */ Color{240, 248, 255, 255},
        /* uiAccent        */ Color{252, 96, 255, 255},
        /* starDim         */ Color{80, 80, 120, 255},
        /* starBright      */ Color{220, 230, 255, 255},
        /* gridLine        */ Color{50, 120, 255, 255},
        /* neonEdge        */ Color{60, 160, 255, 255},
        /* neonEdgeGlow    */ Color{100, 200, 255, 255},
        /* decoCube1       */ Color{255, 140, 40, 255},
        /* decoCube2       */ Color{40, 230, 255, 255},
        /* decoCube3       */ Color{255, 80, 220, 255},
        /* mountainSil     */ Color{12, 16, 34, 255},
        /* ambientParticle */ Color{120, 200, 255, 255},
        /* playerGlow      */ Color{255, 100, 100, 180},
    },
    // Cyan sunrise — navy/teal sky, warm accents.
    LevelPalette{
        /* skyTop          */ Color{16, 40, 82, 255},
        /* skyBottom       */ Color{4, 8, 18, 255},
        /* voidTint        */ Color{5, 10, 24, 255},
        /* platformSide    */ Color{20, 42, 80, 255},
        /* platformTop     */ Color{28, 56, 110, 255},
        /* platformWire    */ Color{50, 180, 255, 255},
        /* laneOuter       */ Color{36, 150, 230, 255},
        /* laneCenter      */ Color{80, 220, 255, 255},
        /* laneGlow        */ Color{86, 230, 255, 255},
        /* streak          */ Color{166, 250, 255, 255},
        /* playerBody      */ Color{255, 160, 40, 255},
        /* playerWire      */ Color{255, 220, 140, 255},
        /* particle        */ Color{150, 246, 255, 255},
        /* uiPanel         */ Color{8, 16, 30, 168},
        /* uiText          */ Color{235, 245, 255, 255},
        /* uiAccent        */ Color{255, 118, 205, 255},
        /* starDim         */ Color{60, 90, 140, 255},
        /* starBright      */ Color{200, 240, 255, 255},
        /* gridLine        */ Color{40, 150, 255, 255},
        /* neonEdge        */ Color{50, 190, 255, 255},
        /* neonEdgeGlow    */ Color{90, 220, 255, 255},
        /* decoCube1       */ Color{255, 100, 100, 255},
        /* decoCube2       */ Color{100, 255, 200, 255},
        /* decoCube3       */ Color{200, 100, 255, 255},
        /* mountainSil     */ Color{8, 14, 28, 255},
        /* ambientParticle */ Color{140, 230, 255, 255},
        /* playerGlow      */ Color{255, 180, 80, 180},
    },
    // Magenta storm — purple/violet sky, pink/green accents.
    LevelPalette{
        /* skyTop          */ Color{42, 12, 66, 255},
        /* skyBottom       */ Color{4, 3, 12, 255},
        /* voidTint        */ Color{8, 6, 18, 255},
        /* platformSide    */ Color{32, 18, 62, 255},
        /* platformTop     */ Color{44, 28, 88, 255},
        /* platformWire    */ Color{180, 80, 255, 255},
        /* laneOuter       */ Color{160, 60, 230, 255},
        /* laneCenter      */ Color{255, 120, 240, 255},
        /* laneGlow        */ Color{255, 110, 230, 255},
        /* streak          */ Color{255, 176, 72, 255},
        /* playerBody      */ Color{80, 255, 160, 255},
        /* playerWire      */ Color{180, 255, 220, 255},
        /* particle        */ Color{248, 166, 255, 255},
        /* uiPanel         */ Color{16, 9, 28, 174},
        /* uiText          */ Color{247, 238, 255, 255},
        /* uiAccent        */ Color{112, 230, 255, 255},
        /* starDim         */ Color{100, 60, 120, 255},
        /* starBright      */ Color{240, 200, 255, 255},
        /* gridLine        */ Color{160, 70, 255, 255},
        /* neonEdge        */ Color{200, 80, 255, 255},
        /* neonEdgeGlow    */ Color{230, 120, 255, 255},
        /* decoCube1       */ Color{255, 200, 50, 255},
        /* decoCube2       */ Color{50, 255, 200, 255},
        /* decoCube3       */ Color{255, 60, 120, 255},
        /* mountainSil     */ Color{14, 8, 22, 255},
        /* ambientParticle */ Color{220, 140, 255, 255},
        /* playerGlow      */ Color{100, 255, 180, 180},
    },
};
}  // namespace

const LevelPalette& GetPalette(int index) {
    int clamped = index;
    if (clamped < 0) {
        clamped = 0;
    } else if (clamped >= cfg::kPaletteCount) {
        clamped = cfg::kPaletteCount - 1;
    }
    return kPalettes[clamped];
}
