//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#include "BMP280.hpp"
#include <Logger.hpp>
#include <RTCMemory.hpp>

namespace open_heat {
namespace sensors {

float BMP280::temperature()
{
  wake();
  const auto temp = m_bmp.readTemperature();
  sleep();

  open_heat::rtc::setLastMeasuredTemp(temp);
  return temp;
}

bool BMP280::setup()
{
  Logger::log(Logger::INFO, "Setting up BMP280");
  return BMBase::init([this]() { return m_bmp.begin(BMP280_ADDRESS_ALT); });
}

void BMP280::sleep()
{
  m_bmp.setSampling(Adafruit_BMP280::MODE_SLEEP);
}

void BMP280::wake()
{
  m_bmp.setSampling(Adafruit_BMP280::MODE_NORMAL);
}
} // namespace sensors
} // namespace open_heat