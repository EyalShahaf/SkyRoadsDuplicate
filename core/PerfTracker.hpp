#pragma once

#include <atomic>
#include <cstdio>

// Lightweight per-frame heap allocation counter.
// Active only in debug builds (NDEBUG not defined).
// Call ResetAllocCounter() before Update(), then ReadAllocCounter() after.

namespace perf {

#ifndef NDEBUG

inline std::atomic<int> g_allocCounter{0};

inline void ResetAllocCounter() { g_allocCounter.store(0, std::memory_order_relaxed); }
inline int  ReadAllocCounter()  { return g_allocCounter.load(std::memory_order_relaxed); }

#else

inline void ResetAllocCounter() {}
inline int  ReadAllocCounter()  { return 0; }

#endif

}  // namespace perf
