#include "CrashHandler.hpp"
#include <backward.hpp>

namespace CrashHandler {

// Dynamic allocation to keep it alive for the duration of the program
static backward::SignalHandling *s_SignalHandler = nullptr;

void Init() {
  if (!s_SignalHandler) {
    s_SignalHandler = new backward::SignalHandling();
  }
}

} // namespace CrashHandler
