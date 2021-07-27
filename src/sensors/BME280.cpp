//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#include "BME280.hpp"
#include <Logger.hpp>

float open_heat::sensors::BME280::getTemperature()
{
  wake();
  const auto temp = bme_.readTemperature();
  sleep();
  return temp;
}
void open_heat::sensors::BME280::setup()
{
  const auto maxRetries = 5;
  auto retries = 0;
  auto initResult = false;
  while (!initResult && retries < maxRetries) {
    initResult = bme_.begin(BME280_ADDRESS_ALTERNATE);
    Logger::log(Logger::INFO, "BME280 init result: %d, try: %d", initResult, retries);
  }
  sleep();
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