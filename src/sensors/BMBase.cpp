//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#include "BMBase.hpp"
#include <Logger.hpp>

namespace open_heat {
namespace sensors {

bool BMBase::init(std::function<bool()>&& sensorBegin)
{
  if (m_isSetup) {
    return true;
  }

  const auto maxRetries = 5;
  auto retries = 0;
  auto initResult = false;
  while (retries < maxRetries) {
    initResult = sensorBegin();
    Logger::log(Logger::INFO, "BMSensor init result: %d, try: %d", initResult, ++retries);
    if (initResult) {
      break;
    }
    delay(100);
  }

  sleep();
  m_isSetup = initResult;
  return initResult;
}

} // namespace sensors
} // namespace open_heat
