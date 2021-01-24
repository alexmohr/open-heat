//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#include "WindowSensor.hpp"
#include <Logger.hpp>
namespace open_heat {
namespace sensors {

Filesystem* WindowSensor::filesystem_ = nullptr;
heating::RadiatorValve* WindowSensor::valve_ = nullptr;

//unsigned int WindowSensor::minMillisBetweenEvents_{500};
unsigned long WindowSensor::lastChangeMillis_;


WindowSensor::WindowSensor(Filesystem* filesystem, heating::RadiatorValve* valve)
{
  filesystem_ = filesystem;
  valve_ = valve;
}

void WindowSensor::setup()
{
  const auto& config = filesystem_->getConfig();
  if (config.WindowPins.Ground <= 0 || config.WindowPins.Vin <= 0) {
    Logger::log(Logger::WARNING, "Window pins not set up.");
    return;
  }

  Logger::log(
    Logger::INFO,
    "WindowSwitch setup: ground %i, vin %i",
    config.WindowPins.Ground,
    config.WindowPins.Vin);

  pinMode(config.WindowPins.Ground, INPUT);
  digitalWrite(config.WindowPins.Ground, LOW);

  pinMode(config.WindowPins.Vin, INPUT_PULLUP);

  isSetUp = true;
  const auto isOpen =  digitalRead(config.WindowPins.Vin) == HIGH;
  valve_->setWindowState(isOpen);

  attachInterrupt(
    digitalPinToInterrupt(config.WindowPins.Vin), sensorChangedInterrupt, CHANGE);
}

void WindowSensor::loop()
{
}

void ICACHE_RAM_ATTR WindowSensor::sensorChangedInterrupt()
{
  const auto& config = filesystem_->getConfig();
  const auto isOpen =  digitalRead(config.WindowPins.Vin) == HIGH;
  Logger::log(Logger::DEBUG, "Window switch changed, new state: %i", isOpen);

  const auto now = millis();
  if (now - lastChangeMillis_  < minMillisBetweenEvents_) {
    Logger::log(Logger::DEBUG, "Ignored window event, due to too little time between events", isOpen);
    return;
  }

  lastChangeMillis_ = now;

  valve_->setWindowState(isOpen);
}

} // namespace sensors
} // namespace open_heat