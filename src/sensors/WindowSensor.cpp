//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#include "WindowSensor.hpp"
#include <yal/yal.hpp>
namespace open_heat::sensors {

// this is probably broken
Filesystem* WindowSensor::m_filesystem = nullptr;
heating::RadiatorValve* WindowSensor::m_valve = nullptr;
bool WindowSensor::m_validate = false;
bool WindowSensor::m_isOpen = false;

// unsigned int WindowSensor::minMillisBetweenEvents_{500};
unsigned long WindowSensor::m_lastChangeMillis;

WindowSensor::WindowSensor(Filesystem* filesystem, heating::RadiatorValve*& valve)
{
  // these are static fields, so we can access them from an ISR
  // todo add setter method?
  m_filesystem = filesystem;
  m_valve = valve;
}

void WindowSensor::setup()
{
  const auto& config = m_filesystem->getConfig();
  if (config.WindowPins.Ground <= 0 || config.WindowPins.Vin <= 0) {
    m_logger.log(yal::Level::WARNING, "Window pins not set up.");
    return;
  }

  m_logger.log(
    yal::Level::INFO,
    "WindowSwitch setup: ground %, vin %",
    config.WindowPins.Ground,
    config.WindowPins.Vin);

  pinMode(static_cast<uint8_t>(config.WindowPins.Ground), INPUT);
  digitalWrite(static_cast<uint8_t>(config.WindowPins.Ground), LOW);

  pinMode(static_cast<uint8_t>(config.WindowPins.Vin), INPUT_PULLUP);

  m_isSetUp = true;
  m_isOpen = digitalRead(static_cast<uint8_t>(config.WindowPins.Vin)) == HIGH;
  m_valve->setWindowState(m_isOpen);

  if (m_valve->getMode() == HEAT) {
    attachInterrupt(
      static_cast<uint8_t>(digitalPinToInterrupt(config.WindowPins.Vin)),
      sensorChangedInterrupt,
      CHANGE);
  }

  // Deatch interrupt when operation mode is not HEAT
  m_valve->registerModeChangedHandler([&config](const OperationMode mode) {
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
  if (!m_validate) {
    return;
  }

  delay(250);
  const auto& config = m_filesystem->getConfig();
  const auto stateNow = digitalRead(static_cast<uint8_t>(config.WindowPins.Vin)) == HIGH;
  if (stateNow == m_isOpen) {
    m_valve->setWindowState(m_isOpen);
  } else {
    m_logger.log(yal::Level::INFO, "Window switch state changed ignored");
  }

  m_validate = false;
}
void ICACHE_RAM_ATTR WindowSensor::sensorChangedInterrupt()
{
  const auto& config = m_filesystem->getConfig();
  m_isOpen = digitalRead(static_cast<uint8_t>(config.WindowPins.Vin)) == HIGH;

  m_logger.log(yal::Level::DEBUG, "Window switch changed, new state: %", m_isOpen);

  const auto now = millis();
  if (now - m_lastChangeMillis < s_minMillisBetweenEvents_) {
    m_logger.log(
      yal::Level::DEBUG,
      "Ignored window event, due to too little time between events",
      m_isOpen);
    return;
  }

  m_validate = true;
}

} // namespace open_heat::sensors
