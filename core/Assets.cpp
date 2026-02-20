#include "core/Assets.hpp"

#include <cstdio>
#include <cstring>

#include <raylib.h>

namespace assets {

const char* Path(const char* relative) {
    // Thread-local buffer avoids heap allocation and is safe for
    // sequential load calls (one path at a time).
    static thread_local char buf[512];
    std::snprintf(buf, sizeof(buf), "assets/%s", relative);
    return buf;
}

bool Exists(const char* relative) {
    return FileExists(Path(relative));
}

}  // namespace assets
