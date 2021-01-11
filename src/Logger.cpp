//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#include "Logger.hpp"
#include <array>
#include <cstdarg>

namespace open_heat {
// ToDo this could be moved into progmem if more RAM is needed
std::array<char, 256>  logBuffer_;


const char CONSOLE_LEVEL_TRACE[] MEM_TYPE   = "\033[1;37m[TRACE]";
const char CONSOLE_LEVEL_DEBUG[] MEM_TYPE   = "\033[1;37m[DEBUG]";
const char CONSOLE_LEVEL_INFO[] MEM_TYPE    = "\033[1;32m[INFO] ";
const char CONSOLE_LEVEL_WARNING[] MEM_TYPE = "\033[1;33m[WARN] ";
const char CONSOLE_LEVEL_ERROR[] MEM_TYPE   = "\033[1;31m[ERROR]";
const char CONSOLE_LEVEL_FATAL[] MEM_TYPE   = "\033[1;31m[FATAL]";
const char CONSOLE_LEVEL_OFF[] MEM_TYPE     = "\033[1;31m[OFF]  ";

const char* const CONSOLE_LOG_LEVEL_STRINGS[] MEM_TYPE = {
  CONSOLE_LEVEL_TRACE,
  CONSOLE_LEVEL_DEBUG,
  CONSOLE_LEVEL_INFO,
  CONSOLE_LEVEL_WARNING,
  CONSOLE_LEVEL_ERROR,
  CONSOLE_LEVEL_FATAL,
  CONSOLE_LEVEL_OFF,
};


Logger::Logger() = default;

void Logger::setup()
{
  getInstance().loggerOutputFunctions_.push_back(LoggerOutputFunction(defaultLog));
}

void Logger::log(Level level, const char* format, ...)
{
  va_list args;
  va_start(args, format);
  vsnprintf(logBuffer_.data(), logBuffer_.size(), format, args);
  va_end(args);

  if (level < getLogLevel()) {
    return;
  }

  for (const auto& outFun: getInstance().loggerOutputFunctions_) {
   outFun(level, "", logBuffer_.data());
  }
}

Logger& Logger::getInstance()
{
  static Logger logger;
  return logger;
}

void Logger::setLogLevel(Logger::Level level)
{
  getInstance().level_ = level;
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

void Logger::defaultLog(Logger::Level level, const char* module, const char* message)
{
  Serial.print(asString(level));
  Serial.print(F(" "));

  if (strlen(module) > 0) {
    Serial.print(F(": "));
    Serial.print(module);
    Serial.print(F(" "));
  }

  Serial.print(message);
  Serial.println("\033[1;97m");
}

const char* Logger::asString(Logger::Level level)
{
  return CONSOLE_LOG_LEVEL_STRINGS[level];
}
void Logger::addPrinter(Logger::LoggerOutputFunction outFun)
{
  getInstance().loggerOutputFunctions_.push_back(outFun);
}

} // namespace open_heat
