//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#include "Logger.hpp"
#include <cstdarg>
#include <cstring>
#include <sstream>

namespace open_heat {
// ToDo this could be moved into progmem if more RAM is needed
std::array<char, 300> logBuffer_;

const char CONSOLE_LEVEL_TRACE[] MEM_TYPE = "\033[1;37m[TRACE] ";
const char CONSOLE_LEVEL_DEBUG[] MEM_TYPE = "\033[1;37m[DEBUG] ";
const char CONSOLE_LEVEL_INFO[] MEM_TYPE = "\033[1;32m[INFO]  ";
const char CONSOLE_LEVEL_WARNING[] MEM_TYPE = "\033[1;33m[WARN]  ";
const char CONSOLE_LEVEL_ERROR[] MEM_TYPE = "\033[1;31m[ERROR] ";
const char CONSOLE_LEVEL_FATAL[] MEM_TYPE = "\033[1;31m[FATAL] ";
const char CONSOLE_LEVEL_OFF[] MEM_TYPE = "\033[1;31m[OFF]   ";

const char* const CONSOLE_LOG_LEVEL_STRINGS[] MEM_TYPE = {
  CONSOLE_LEVEL_TRACE,
  CONSOLE_LEVEL_DEBUG,
  CONSOLE_LEVEL_INFO,
  CONSOLE_LEVEL_WARNING,
  CONSOLE_LEVEL_ERROR,
  CONSOLE_LEVEL_FATAL,
  CONSOLE_LEVEL_OFF,
};

Logger Logger::s_logger;

Logger::Logger() = default;

void Logger::setup()
{
  if (Serial) {
    getInstance().loggerOutputFunctions_.push_back(defaultLog);
  }
}

void Logger::log(Level level, const char* format, ...)
{
  if (level < getLogLevel()) {
    return;
  }

  std::memset(logBuffer_.data(), 0, logBuffer_.size());

  const auto levelText = levelToText(level);
  std::strcpy(logBuffer_.data(), levelText);
  const auto levelTextLen = std::strlen(logBuffer_.data());

  va_list args;
  va_start(args, format);
  vsnprintf(logBuffer_.data() + levelTextLen,
            logBuffer_.size() , format, args);
  va_end(args);

  for (const auto& outFun : getInstance().loggerOutputFunctions_) {
    outFun(logBuffer_.data());
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

const char* Logger::levelToText(Logger::Level level)
{
  return CONSOLE_LOG_LEVEL_STRINGS[level];
}

void Logger::addPrinter(const Logger::LoggerOutputFunction& outFun)
{
  getInstance().loggerOutputFunctions_.push_back(outFun);
}

} // namespace open_heat
