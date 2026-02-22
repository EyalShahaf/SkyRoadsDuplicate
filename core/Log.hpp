#pragma once

#include <memory>
#include <string>

#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>

namespace Log {

void Init();
void Shutdown();

std::shared_ptr<spdlog::logger> &GetLogger();

} // namespace Log

#define LOG_TRACE(...) ::Log::GetLogger()->trace(__VA_ARGS__)
#define LOG_DEBUG(...) ::Log::GetLogger()->debug(__VA_ARGS__)
#define LOG_INFO(...) ::Log::GetLogger()->info(__VA_ARGS__)
#define LOG_WARN(...) ::Log::GetLogger()->warn(__VA_ARGS__)
#define LOG_ERROR(...) ::Log::GetLogger()->error(__VA_ARGS__)
#define LOG_CRITICAL(...) ::Log::GetLogger()->critical(__VA_ARGS__)
