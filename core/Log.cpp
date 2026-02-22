#include "Log.hpp"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <vector>

namespace Log {

static std::shared_ptr<spdlog::logger> s_Logger;

void Init() {
  std::vector<spdlog::sink_ptr> sinks;

  // Console sink with color
  auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  consoleSink->set_pattern("%^[%T] %n: %v%$");
  sinks.push_back(consoleSink);

  // File sink
  auto fileSink =
      std::make_shared<spdlog::sinks::basic_file_sink_mt>("skyroads.log", true);
  fileSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %n: %v");
  sinks.push_back(fileSink);

  s_Logger =
      std::make_shared<spdlog::logger>("SKYROADS", sinks.begin(), sinks.end());
  spdlog::register_logger(s_Logger);

  s_Logger->set_level(spdlog::level::trace);
  s_Logger->flush_on(spdlog::level::trace);

  LOG_INFO("Logging initialized");
}

void Shutdown() { spdlog::shutdown(); }

std::shared_ptr<spdlog::logger> &GetLogger() { return s_Logger; }

} // namespace Log
