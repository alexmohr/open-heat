//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#include "BME280.hpp"

open_heat::sensors::BME280::BME280()
{

}

float open_heat::sensors::BME280::getTemperature()
{
return bme_.readTemperature();
}
void open_heat::sensors::BME280::setup()
{
  bme_.begin(BME280_ADDRESS_ALTERNATE);
}
void open_heat::sensors::BME280::loop()
{
}
