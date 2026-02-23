#include "core/Assets.hpp"
#include <filesystem>
#include <string>

namespace assets {

namespace fs = std::filesystem;

const char *Path(const char *relative) {
  // Search for the 'assets' directory in the current directory and its parents.
  static thread_local std::string s_PathBuf;
  fs::path current = fs::current_path();
  fs::path assetsPath = "assets";

  // Look up to 3 levels up for the assets directory
  for (int i = 0; i < 4; ++i) {
    if (fs::exists(current / "assets")) {
      assetsPath = current / "assets";
      break;
    }
    if (current.has_parent_path()) {
      current = current.parent_path();
    } else {
      break;
    }
  }

  s_PathBuf = (assetsPath / relative).string();
  return s_PathBuf.c_str();
}

bool Exists(const char *relative) {
  return fs::exists(fs::path("assets") / relative);
}

} // namespace assets
