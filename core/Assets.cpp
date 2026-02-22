#include "core/Assets.hpp"
#include <filesystem>
#include <string>

namespace assets {

namespace fs = std::filesystem;

const char *Path(const char *relative) {
  // Using std::filesystem for robust path handling.
  // We still return a static pointer for raylib compatibility,
  // but the logic is now based on modern standards.
  static thread_local std::string s_PathBuf;
  s_PathBuf = (fs::path("assets") / relative).string();
  return s_PathBuf.c_str();
}

bool Exists(const char *relative) {
  return fs::exists(fs::path("assets") / relative);
}

} // namespace assets
