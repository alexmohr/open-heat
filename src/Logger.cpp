//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#include "Logger.hpp"
#include "RTCMemory.hpp"
#include <cstdarg>
#include <cstring>
#include <sstream>

namespace open_heat {
// ToDo this could be moved into progmem if more RAM is needed
std::array<char, 768> logBuffer_;

static constexpr const char COLORED_LEVEL_TRACE[] MEM_TYPE = "\033[1;37m[TRACE] ";
static constexpr const char COLORED_LEVEL_DEBUG[] MEM_TYPE = "\033[1;37m[DEBUG] ";
static constexpr const char COLORED_LEVEL_INFO[] MEM_TYPE = "\033[1;32m[INFO]  ";
static constexpr const char COLORED_LEVEL_WARNING[] MEM_TYPE = "\033[1;33m[WARN]  ";
static constexpr const char COLORED_LEVEL_ERROR[] MEM_TYPE = "\033[1;31m[ERROR] ";
static constexpr const char COLORED_LEVEL_FATAL[] MEM_TYPE = "\033[1;31m[FATAL] ";
static constexpr const char COLORED_LEVEL_OFF[] MEM_TYPE = "\033[1;31m[OFF]   ";

static constexpr const char LEVEL_TRACE[] MEM_TYPE = "[TRACE]";
static constexpr const char LEVEL_DEBUG[] MEM_TYPE = "[DEBUG]";
static constexpr const char LEVEL_INFO[] MEM_TYPE = "[INFO] ";
static constexpr const char LEVEL_WARNING[] MEM_TYPE = "[WARN]";
static constexpr const char LEVEL_ERROR[] MEM_TYPE = "[ERROR]";
static constexpr const char LEVEL_FATAL[] MEM_TYPE = "[FATAL]";
static constexpr const char LEVEL_OFF[] MEM_TYPE = "[OFF]  ";

static constexpr const char* const LOG_LEVEL_STRINGS[] MEM_TYPE = {
  LEVEL_TRACE,
  LEVEL_DEBUG,
  LEVEL_INFO,
  LEVEL_WARNING,
  LEVEL_ERROR,
  LEVEL_FATAL,
  LEVEL_OFF,
};

const char* const COLORED_LOG_LEVEL_STRINGS[] MEM_TYPE = {
  COLORED_LEVEL_TRACE,
  COLORED_LEVEL_DEBUG,
  COLORED_LEVEL_INFO,
  COLORED_LEVEL_WARNING,
  COLORED_LEVEL_ERROR,
  COLORED_LEVEL_FATAL,
  COLORED_LEVEL_OFF,
};

Logger Logger::s_logger;

Logger::Logger() = default;

void Logger::setup()
{
}

void Logger::log(Level level, const char* format, ...)
{
  if (DISABLE_ALL_LOGGING) {
    return;
  }

  if (level < getLogLevel()) {
    return;
  }

  std::memset(logBuffer_.data(), 0, logBuffer_.size());

  va_list args;
  va_start(args, format);
  const auto time = rtc::offsetMillis();
  const auto offset = std::sprintf(logBuffer_.data(), "[%010llu] ", time);
  vsnprintf(logBuffer_.data() + offset, logBuffer_.size(), format, args);
  va_end(args);

  for (const auto& outFun : getInstance().loggerOutputFunctions_) {
    outFun(level, logBuffer_.data());
  }
}

Logger& Logger::getInstance()
{
  return s_logger;
}

void Logger::setLogLevel(Logger::Level level)
{
  if (level >= LOG_LEVEL) {
    getInstance().level_ = level;
  }
}

Logger::Level Logger::getLogLevel()
{
  return getInstance().level_;
}

String Logger::formatBytes(size_t bytes)
{
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  } else {
    return String(bytes / 1024.0 / 1024.0 / 1024.0) + "GB";
  }
}

void Logger::defaultLog(const std::string& message)
{
  Serial.println(message.c_str());
}

const char* Logger::levelToText(const Logger::Level level, const bool color)
{
  if (color) {
    return COLORED_LOG_LEVEL_STRINGS[level];
  }
  return LOG_LEVEL_STRINGS[level];
}

void Logger::addPrinter(const LoggerOutputFunction&& outFun)
{
  getInstance().loggerOutputFunctions_.push_back(std::move(outFun));
}

} // namespace open_heat
