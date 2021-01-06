//
// Copyright (c) 2020 Alexander Mohr
// Licensed under the terms of the MIT license
//
#include "Logger.hpp"
#include <array>
#include <cstdarg>

namespace open_heat {
std::array<char, 256> logBuffer_;

#if defined(ARDUINO_ARCH_AVR)
#include <avr/pgmspace.h>
#endif

// There appears to be an incompatibility with ESP8266 2.3.0.
#if defined(ESP8266)
#define MEM_TYPE
#else
#define MEM_TYPE PROGMEM
#endif

const char LEVEL_TRACE[] MEM_TYPE   = "\033[1;37m[TRACE]";
const char LEVEL_DEBUG[] MEM_TYPE   = "\033[1;37m[DEBUG]";
const char LEVEL_INFO[] MEM_TYPE    = "\033[1;32m[INFO] ";
const char LEVEL_WARNING[] MEM_TYPE = "\033[1;33m[WARN] ";
const char LEVEL_ERROR[] MEM_TYPE   = "\033[1;31m[ERROR]";
const char LEVEL_FATAL[] MEM_TYPE   = "\033[1;31m[FATAL]";
const char LEVEL_OFF[] MEM_TYPE     = "\033[1;31m[OFF]  ";

const char* const LOG_LEVEL_STRINGS[] MEM_TYPE = {
  LEVEL_TRACE,
  LEVEL_DEBUG,
  LEVEL_INFO,
  LEVEL_WARNING,
  LEVEL_ERROR,
  LEVEL_FATAL,
  LEVEL_OFF,
};

void Logger::setup()
{
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

  if (getInstance().loggerOutputFunction_) {
    getInstance().loggerOutputFunction_(level, "", logBuffer_.data());
  } else {
    Logger::defaultLog(level, "", logBuffer_.data());
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
 /* Serial.print(millis());
  Serial.print(F("\t"));*/
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
  return LOG_LEVEL_STRINGS[level];
}

} // namespace open_heat
