//
// Copyright (c) 2020 Alexander Mohr
// Open-Heat - Radiator control for ESP8266
// Licensed under the terms of the MIT license
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
