//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#include "WindowSensor.hpp"
#include <Logger.hpp>
namespace open_heat {
namespace sensors {

// this is probably broken
Filesystem* WindowSensor::filesystem_ = nullptr;
heating::RadiatorValve* WindowSensor::valve_ = nullptr;
bool WindowSensor::validate_ = false;
bool WindowSensor::isOpen_ = false;

// unsigned int WindowSensor::minMillisBetweenEvents_{500};
unsigned long WindowSensor::lastChangeMillis_;

WindowSensor::WindowSensor(Filesystem* filesystem, heating::RadiatorValve*& valve)
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

  pinMode(static_cast<uint8_t>(config.WindowPins.Ground), INPUT);
  digitalWrite(static_cast<uint8_t>(config.WindowPins.Ground), LOW);

  pinMode(static_cast<uint8_t>(config.WindowPins.Vin), INPUT_PULLUP);

  isSetUp = true;
  isOpen_ = digitalRead(static_cast<uint8_t>(config.WindowPins.Vin)) == HIGH;
  valve_->setWindowState(isOpen_);

  if (valve_->getMode() == HEAT) {
    attachInterrupt(
      static_cast<uint8_t>(digitalPinToInterrupt(config.WindowPins.Vin)),
      sensorChangedInterrupt,
      CHANGE);
  }

  // Deatch interrupt when operation mode is not HEAT
  valve_->registerModeChangedHandler([&config](const OperationMode mode) {
    if (mode == HEAT) {
      attachInterrupt(
        static_cast<uint8_t>(digitalPinToInterrupt(config.WindowPins.Vin)),
        sensorChangedInterrupt,
        CHANGE);
    } else {
      detachInterrupt(static_cast<uint8_t>(digitalPinToInterrupt(config.WindowPins.Vin)));
    }
  });
}

void WindowSensor::loop()
{
  if (!validate_) {
    return;
  }

  delay(250);
  const auto& config = filesystem_->getConfig();
  const auto stateNow = digitalRead(static_cast<uint8_t>(config.WindowPins.Vin)) == HIGH;
  if (stateNow == isOpen_) {
    valve_->setWindowState(isOpen_);
  } else {
    Logger::log(Logger::INFO, "Window switch state changed ignored");
  }

  validate_ = false;
}
void ICACHE_RAM_ATTR WindowSensor::sensorChangedInterrupt()
{
  const auto& config = filesystem_->getConfig();
  isOpen_ = digitalRead(static_cast<uint8_t>(config.WindowPins.Vin)) == HIGH;

  Logger::log(Logger::DEBUG, "Window switch changed, new state: %i", isOpen_);

  const auto now = millis();
  if (now - lastChangeMillis_ < minMillisBetweenEvents_) {
    Logger::log(
      Logger::DEBUG,
      "Ignored window event, due to too little time between events",
      isOpen_);
    return;
  }

  validate_ = true;
}

} // namespace sensors
} // namespace open_heat