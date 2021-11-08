//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#include "Serial.hpp"
#include "Logger.hpp"
#include <HardwareSerial.h>
namespace open_heat {
void Serial::setup()
{
  ::Serial.begin(MONITOR_SPEED);
  ::Serial.setTimeout(2000);

  while (!::Serial) {
    delay(200);
  }
  ::Serial.println("");

  Logger::addPrinter([](const Logger::Level level, const std::string& message) {
    ::Serial.print(Logger::levelToText(level, true));
    ::Serial.println(message.c_str());
  });
}
} // namespace open_heat