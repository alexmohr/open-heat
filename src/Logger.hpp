//
// Copyright (c) 2020 Alexander Mohr 
// Licensed under the terms of the MIT license
//
#ifndef LOGGER_HPP_
#define LOGGER_HPP_

#include "Arduino.h"

namespace open_heat {
class Logger {

public:
  enum Level
  {
   TRACE = 0,
   DEBUG,
   INFO,
   WARNING,
   ERROR,
   FATAL,
  };

  typedef void (*LoggerOutputFunction)(Level level,
                                       const char* module,
                                       const char* message);

  static void setup();

  static void setLogLevel(Level level);
  static Level getLogLevel();

  static void log(Level level, const char* format, ...);

  static const char* asString(Level level);

  static String formatBytes(size_t bytes);

 private:
   Logger() = default;
   Logger(const Logger &);
  void operator = (const Logger &);

  static Logger & getInstance();
  static void defaultLog(Level level, const char* module, const char* message);

  LoggerOutputFunction loggerOutputFunction_;


  Level level_;

  //static std::array<char, 256> logBuffer_;
};
}


#endif // LOGGER_HPP_
