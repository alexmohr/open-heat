//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#ifndef LOGGER_HPP_
#define LOGGER_HPP_

#include "Arduino.h"
#include "Print.h"

#if defined(ARDUINO_ARCH_AVR)
#include <avr/pgmspace.h>
#endif

#define MEM_TYPE PROGMEM
namespace open_heat {

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

class Logger {
  public:
  enum Level {
    TRACE = 0,
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL,
    OFF
  };

  typedef std::function<void(Level level, const char* module, const char* message)>
    LoggerOutputFunction;

  static void setup();

  static void setLogLevel(Level level);
  static Level getLogLevel();

  static void log(Level level, const char* format, ...);

  static const char* asString(Level level);

  static String formatBytes(size_t bytes);

  static void addPrinter(const LoggerOutputFunction& outFun);

  private:
  Logger();
  Logger(const Logger&);
  void operator=(const Logger&);

  static Logger& getInstance();
  static Logger s_logger;
  static void defaultLog(Level level, const char* module, const char* message);

  std::vector<LoggerOutputFunction> loggerOutputFunctions_;

  Level level_{LOG_LEVEL};
};
} // namespace open_heat

#endif // LOGGER_HPP_
