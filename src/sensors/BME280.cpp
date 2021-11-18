//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#include "BME280.hpp"
#include <Logger.hpp>
#include <RTCMemory.hpp>

float open_heat::sensors::BME280::getTemperature()
{
  wake();
  const auto temp = bme_.readTemperature();
  sleep();

  open_heat::rtc::setLastMeasuredTemp(temp);
  return temp;
}

float open_heat::sensors::BME280::getHumidity()
{
  wake();
  const auto humid = bme_.readHumidity();
  sleep();
  return humid;
}

bool open_heat::sensors::BME280::setup()
{
  const auto maxRetries = 5;
  auto retries = 0;
  auto initResult = false;
  while (retries < maxRetries) {
    initResult = bme_.begin(BME280_ADDRESS_ALTERNATE);
    Logger::log(Logger::INFO, "BME280 init result: %d, try: %d", initResult, ++retries);
    if (initResult) {
      break;
    }
    delay(100);
  }

  sleep();
  return initResult;
}

void open_heat::sensors::BME280::loop()
{
}

void open_heat::sensors::BME280::sleep()
{
  bme_.setSampling(Adafruit_BME280::MODE_SLEEP);
}

void open_heat::sensors::BME280::wake()
{
  bme_.setSampling(Adafruit_BME280::MODE_NORMAL);
}