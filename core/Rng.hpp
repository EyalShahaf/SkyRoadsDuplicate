#pragma once

#include <cstdint>

namespace core {
uint32_t NextU32(uint32_t& state);
float NextFloat01(uint32_t& state);
}  // namespace core
