#pragma once

#include <raylib.h>

struct LevelPalette {
    Color skyTop{};
    Color skyBottom{};
    Color voidTint{};
    Color platformSide{};
    Color platformTop{};
    Color platformWire{};
    Color laneOuter{};
    Color laneCenter{};
    Color laneGlow{};
    Color streak{};
    Color playerBody{};
    Color playerWire{};
    Color particle{};
    Color uiPanel{};
    Color uiText{};
    Color uiAccent{};
    // Scene dressing colors.
    Color starDim{};
    Color starBright{};
    Color gridLine{};
    Color neonEdge{};
    Color neonEdgeGlow{};
    Color decoCube1{};
    Color decoCube2{};
    Color decoCube3{};
    Color mountainSilhouette{};
    Color ambientParticle{};
    Color playerGlow{};
};

const LevelPalette& GetPalette(int index);
