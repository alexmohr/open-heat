//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#include "BME280.hpp"

float open_heat::sensors::BME280::getTemperature()
{
  wake();
  const auto temp = bme_.readTemperature();
  sleep();
  return temp;
}
void open_heat::sensors::BME280::setup()
{
  bme_.begin(BME280_ADDRESS_ALTERNATE);
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