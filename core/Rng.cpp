#include "core/Rng.hpp"
#include <random>

namespace core {

uint32_t NextU32(uint32_t &state) {
  // mt19937 is a widely used industrial standard for high-quality RNG.
  // Use state as seed if it's the first call or reset.
  if (state == 0)
    state = 0xA341316Cu;

  std::mt19937 gen(state);
  state = static_cast<uint32_t>(gen());
  return state;
}

float NextFloat01(uint32_t &state) {
  std::mt19937 gen(state);
  std::uniform_real_distribution<float> dis(0.0f, 1.0f);
  float val = dis(gen);
  state = static_cast<uint32_t>(gen()); // Advance state
  return val;
}

} // namespace core
