//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#include "BME280.hpp"
#include <yal/yal.hpp>
#include <RTCMemory.hpp>

namespace open_heat::sensors {

float BME280::temperature()
{
  wake();
  const auto temp = m_bme.readTemperature();
  sleep();

  open_heat::rtc::setLastMeasuredTemp(temp);
  return temp;
}

float BME280::humidity()
{
  wake();
  const auto humid = m_bme.readHumidity();
  sleep();
  return humid;
}

bool BME280::setup()
{
  m_logger.log(yal::Level::INFO, "Setting up BME280");
  return BMBase::init([this]() { return m_bme.begin(BME280_ADDRESS_ALTERNATE); });
}

void BME280::sleep()
{
  m_bme.setSampling(Adafruit_BME280::MODE_SLEEP);
}

void BME280::wake()
{
  m_bme.setSampling(Adafruit_BME280::MODE_NORMAL);
}
} // namespace open_heat
