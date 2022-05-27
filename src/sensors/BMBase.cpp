//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#include "BMBase.hpp"
#include <Arduino.h>

namespace open_heat::sensors {

bool BMBase::init(std::function<bool()>&& sensorBegin)
{
  if (m_isSetup) {
    return true;
  }

  static constexpr const auto maxRetries = 5U;
  auto retries = 0U;
  auto initResult = false;
  while (retries < maxRetries) {
    initResult = sensorBegin();
    m_logger.log(yal::Level::INFO, "BMSensor init result: %, try: %", initResult, ++retries);
    if (initResult) {
      break;
    }
    static constexpr const auto bmeInitRetryDelay = 100U;
    delay(bmeInitRetryDelay);
  }

  sleep();
  m_isSetup = initResult;
  return initResult;
}

} // namespace open_heat
