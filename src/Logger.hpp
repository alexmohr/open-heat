//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#ifndef LOGGER_HPP_
#define LOGGER_HPP_

#include "Arduino.h"
#include "Print.h"
#include <string>

#if defined(ARDUINO_ARCH_AVR)
#include <avr/pgmspace.h>
#endif

#define MEM_TYPE PROGMEM
namespace open_heat {

class Logger {
  public:
  enum Level { TRACE = 0, DEBUG, INFO, WARNING, ERROR, FATAL, OFF };

  typedef std::function<void(const Level level, const std::string&)> LoggerOutputFunction;

  static void setup();

  static void setLogLevel(Level level);
  static Level getLogLevel();

  static void log(Level level, const char* format, ...);

  static const char* levelToText(Level level, bool color);

  static String formatBytes(size_t bytes);

  static void addPrinter(const LoggerOutputFunction&& outFun);

  private:
  Logger();
  Logger(const Logger&);
  void operator=(const Logger&);

  static Logger& getInstance();
  static Logger s_logger;
  static void defaultLog(const std::string& message);

  std::vector<LoggerOutputFunction> loggerOutputFunctions_;

  Level level_{LOG_LEVEL};
};
} // namespace open_heat

#endif // LOGGER_HPP_
