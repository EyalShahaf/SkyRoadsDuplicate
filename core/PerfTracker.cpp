#include "core/PerfTracker.hpp"

#include <cstdlib>
#include <new>

// Override global new/delete in debug builds to count heap allocations.
#ifndef NDEBUG

void* operator new(std::size_t size) {
    perf::g_allocCounter.fetch_add(1, std::memory_order_relaxed);
    void* ptr = std::malloc(size);
    if (!ptr) {
        throw std::bad_alloc();
    }
    return ptr;
}

void* operator new[](std::size_t size) {
    perf::g_allocCounter.fetch_add(1, std::memory_order_relaxed);
    void* ptr = std::malloc(size);
    if (!ptr) {
        throw std::bad_alloc();
    }
    return ptr;
}

void operator delete(void* ptr) noexcept {
    std::free(ptr);
}

void operator delete[](void* ptr) noexcept {
    std::free(ptr);
}

void operator delete(void* ptr, std::size_t) noexcept {
    std::free(ptr);
}

void operator delete[](void* ptr, std::size_t) noexcept {
    std::free(ptr);
}

#endif
