#include "core/Rng.hpp"

namespace core {
uint32_t NextU32(uint32_t& state) {
    // Xorshift32, deterministic from the explicit caller-provided seed/state.
    if (state == 0u) {
        state = 0xA341316Cu;
    }
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

float NextFloat01(uint32_t& state) {
    constexpr float invMaxU32 = 1.0f / 4294967295.0f;
    return static_cast<float>(NextU32(state)) * invMaxU32;
}
}  // namespace core
