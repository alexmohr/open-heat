//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#include "RadiatorValve.hpp"
#include <RTCMemory.hpp>

namespace open_heat::heating {

RadiatorValve::RadiatorValve(
  sensors::Temperature*& tempSensor,
  esp_gui::WebServer& webServer,
  esp_gui::Configuration& config) :
    m_webServer(webServer),
    m_config(config),
    m_temperatureSensor(tempSensor),
    m_logger("VALVE")
{
}

void RadiatorValve::setup()
{
  disablePins();
}

uint64_t RadiatorValve::loop()
{
  // no check necessary yet
  if (rtc::offsetMillis() < rtc::read().valveNextCheckMillis) {
    return rtc::read().valveNextCheckMillis;
  }

  // heating disabled
  if (
    rtc::read().mode == config::OperationMode::OFF
    || rtc::read().mode == config::OperationMode::FULL_OPEN) {
    const auto nextCheck = std::numeric_limits<uint64_t>::max();
    rtc::setValveNextCheckMillis(nextCheck);
    rtc::read().mode == config::OperationMode::OFF
      ? closeValve(VALVE_FULL_ROTATE_TIME * 2)
      : openValve(VALVE_FULL_ROTATE_TIME * 2);

    m_logger.log(yal::Level::DEBUG, "Heating is turned off, disabled heating");
    return nextCheck;
  }

  if (rtc::read().mode == config::OperationMode::UNKNOWN) {
    m_logger.log(yal::Level::ERROR, "Unknown heating mode!");
    return nextCheckTime();
  } // else mode is heat

  // store data once to work with consistent values
  const auto rtcData = rtc::read();
  m_logger.log(yal::Level::DEBUG, "trying to reach temperature: %", rtcData.setTemp);

  // also updates last measured temp
  const auto measuredTemp = m_temperatureSensor->temperature();
  const float predictPart
    = PREDICTION_STEEPNESS * (measuredTemp - rtcData.lastMeasuredTemp);
  const float predictTemp = measuredTemp + predictPart;
  const auto predictionError = measuredTemp - rtcData.lastPredictedTemp;
  const auto temperatureChange = measuredTemp - rtcData.lastMeasuredTemp;
  const auto minTemperatureChange = 0.2;

  const auto openHysteresis = 0.3F;
  const auto closeHysteresis = 0.2F;
  const auto absTempDiff
    = std::max(rtcData.setTemp, predictTemp) - std::min(rtcData.setTemp, predictTemp);

  rtc::setLastPredictedTemp(predictTemp);

  m_logger.log(
    yal::Level::INFO,
    "Valve loop\n"
    "\tpredictTemp %.2f in %lu ms\n"
    "\tpredictPart: %.2f, predictionError: %.2f\n"
    "\tmeasuredTemp: %.2f, lastMeasuredTemp %.2f, setTemp %.2f \n"
    "\ttemperatureChange %.2f, absTempDiff: %.2f",
    predictTemp,
    m_checkIntervalMillis,
    predictPart,
    predictionError,
    measuredTemp,
    rtcData.lastMeasuredTemp,
    rtcData.setTemp,
    temperatureChange,
    absTempDiff);

  if (0 == predictTemp) {
    const auto nextCheck = nextCheckTime();
    m_logger.log(yal::Level::DEBUG, "Skipping temperature setting, predictTemp = 0");
    return nextCheck;
  }

  // Act according to the prediction.
  if (predictTemp < (rtcData.setTemp - openHysteresis)) {
    if (temperatureChange < minTemperatureChange) {
      handleTempTooLow(rtcData, measuredTemp, predictTemp, openHysteresis);
    } else {
      m_logger.log(
        yal::Level::INFO,
        "RISE, NO ADJUST: Temp old %.2f, temp now %.2f, temp change %.2f",
        rtcData.lastMeasuredTemp,
        measuredTemp,
        temperatureChange);
    }

  } else if (predictTemp > (rtcData.setTemp + closeHysteresis)) {
    if (temperatureChange >= -minTemperatureChange) {
      handleTempTooHigh(rtcData, predictTemp, closeHysteresis);
    } else {
      m_logger.log(
        yal::Level::INFO,
        "SINK, NO ADJUST: Temp old %.2f, temp now %.2f, temp change %.2f",
        rtc::read().lastMeasuredTemp,
        measuredTemp,
        temperatureChange);
    }
  } else {
    m_logger.log(yal::Level::INFO, "Temperature is in tolerance, not changing");
  }

  return nextCheckTime();
}

void RadiatorValve::handleTempTooHigh(
  const rtc::Memory& rtcData,
  const float predictTemp,
  const float closeHysteresis)
{
  auto closeTime = 200;
  const auto predictDiff = rtcData.setTemp - predictTemp - closeHysteresis;
  m_logger.log(yal::Level::INFO, "Close predict diff: %f", predictDiff);
  if (predictDiff <= 2) {
    closeTime = 5000;
  } else if (predictDiff <= 1.5) {
    closeTime = 4000;
  } else if (predictDiff <= 1) {
    closeTime = 2500;
  } else if (predictDiff <= 0.5) {
    closeTime = 1500;
  }

  closeTime -= m_spinUpMillis;
  if (closeTime <= 0) {
    return;
  }

  closeValve(closeTime);
}

void RadiatorValve::handleTempTooLow(
  const rtc::Memory& rtcData,
  const float measuredTemp,
  const float predictTemp,
  const float openHysteresis)
{
  auto openTime = 350;
  const float largeTempDiff = 3;
  const auto predictDiff = rtcData.setTemp - predictTemp - openHysteresis;
  m_logger.log(yal::Level::INFO, "Open predict diff: %f", predictDiff);

  if (
    rtcData.setTemp - predictTemp > largeTempDiff
    && rtcData.setTemp - measuredTemp > largeTempDiff) {
    openTime *= 10;
  } else if (predictDiff >= 2) {
    openTime = 3000;
  } else if (predictDiff >= 1.5) {
    openTime = 2500;
  } else if (predictDiff >= 1) {
    openTime = 1500;
  } else if (predictDiff >= 0.5) {
    openTime = 1000;
  }

  openTime -= m_spinUpMillis;
  if (openTime <= 0) {
    return;
  }
  openValve(openTime);
}
uint64_t RadiatorValve::nextCheckTime()
{
  const auto nextCheck = rtc::offsetMillis() + m_checkIntervalMillis;
  rtc::setValveNextCheckMillis(nextCheck);
  return nextCheck;
}

float RadiatorValve::getConfiguredTemp()
{
  return rtc::read().setTemp;
}

void RadiatorValve::setConfiguredTemp(float temp)
{
  if (temp == rtc::read().setTemp) {
    return;
  }

  m_logger.log(yal::Level::INFO, "New target temperature %f", temp);
  rtc::setSetTemp(temp);
  setNextCheckTimeNow();

  updateConfig();

  for (const auto& handler : m_setTempChangeHandler) {
    handler(temp);
  }
}

void RadiatorValve::updateConfig()
{
  const auto rtcMem = rtc::read();
  m_config.setValue(config::TEMP_SET, rtcMem.setTemp, true);
  m_config.setValue(config::TEMP_MODE, static_cast<int>(rtcMem.mode), true);
}

void RadiatorValve::closeValve(unsigned int rotateTime)
{
  if (rtc::read().currentRotateTime <= (-VALVE_FULL_ROTATE_TIME)) {
    m_logger.log(
      yal::Level::DEBUG,
      "Valve already fully closed, current rotate time: %i",
      rtc::read().currentRotateTime);
    return;
  }

  if (rtc::read().currentRotateTime < 0) {
    rotateTime = remainingRotateTime(static_cast<int>(rotateTime), true);
  }

  rtc::setCurrentRotateTime(
    rtc::read().currentRotateTime - rotateTime, VALVE_FULL_ROTATE_TIME);
  const auto motorGround = m_config.value<int8_t>(config::MOTOR_GROUND);
  const auto motorVin = m_config.value<int8_t>(config::MOTOR_VIN);

  m_logger.log(
    yal::Level::DEBUG,
    "Closing valve for %ms, currentRotateTime: %, vin: %, ground: %",
    rotateTime,
    rtc::read().currentRotateTime,
    motorVin,
    motorGround);

  rotateValve(rotateTime, {motorGround, motorVin}, HIGH, LOW);
}

void RadiatorValve::openValve(unsigned int rotateTime)
{
  if (rtc::read().currentRotateTime >= VALVE_FULL_ROTATE_TIME) {
    m_logger.log(
      yal::Level::DEBUG,
      "Valve already fully open, current rotate time: %",
      rtc::read().currentRotateTime);
    return;
  }

  if (rtc::read().currentRotateTime > 0) {
    rotateTime = remainingRotateTime(static_cast<int>(rotateTime), false);
  }

  rtc::setCurrentRotateTime(
    rtc::read().currentRotateTime + rotateTime, VALVE_FULL_ROTATE_TIME);
  const auto motorGround = m_config.value<int8_t>(config::MOTOR_GROUND);
  const auto motorVin = m_config.value<int8_t>(config::MOTOR_VIN);

  m_logger.log(
    yal::Level::DEBUG,
    "Opening valve for %ms, currentRotateTime: %ms, vin: %, ground: %",
    rotateTime,
    rtc::read().currentRotateTime,
    motorVin,
    motorGround);

  rotateValve(rotateTime, {motorGround, motorVin}, LOW, HIGH);
}

unsigned int RadiatorValve::remainingRotateTime(int rotateTime, bool close)
{
  int remainingTime;
  // if close and rotate time is positive
  // or open and rotate time is negative
  // we still have the full range left
  if ((close && rotateTime < 0) || (!close && rotateTime > 0)) {
    remainingTime = VALVE_FULL_ROTATE_TIME - std::abs(rtc::read().currentRotateTime);
  } else {
    remainingTime = VALVE_FULL_ROTATE_TIME;
  }

  if (remainingTime <= 0 || rotateTime > remainingTime) {
    // correct for eventual offsets
    rotateTime = m_finalRotateMillis;
  }

  return rotateTime;
}

void RadiatorValve::rotateValve(
  unsigned int rotateTime,
  const config::PinSettings& config,
  int vinState,
  int groundState)
{
  enablePins();
  digitalWrite(static_cast<uint8_t>(config.Vin), vinState);
  digitalWrite(static_cast<uint8_t>(config.Ground), groundState);

  delay(rotateTime + m_spinUpMillis);
  disablePins();
  m_logger.log(yal::Level::DEBUG, "Rotating valve done");
}

void RadiatorValve::disablePins()
{
  const auto motorGround = m_config.value<int8_t>(config::MOTOR_GROUND);
  const auto motorVin = m_config.value<int8_t>(config::MOTOR_VIN);

  digitalWrite(static_cast<uint8_t>(motorVin), LOW);
  digitalWrite(static_cast<uint8_t>(motorGround), LOW);

  pinMode(static_cast<uint8_t>(motorVin), INPUT);
  pinMode(static_cast<uint8_t>(motorGround), INPUT);
}

void RadiatorValve::enablePins()
{
  const auto motorGround = m_config.value<int8_t>(config::MOTOR_GROUND);
  const auto motorVin = m_config.value<int8_t>(config::MOTOR_VIN);

  pinMode(static_cast<uint8_t>(motorVin), OUTPUT);
  pinMode(static_cast<uint8_t>(motorGround), OUTPUT);
}

void RadiatorValve::setMode(const config::OperationMode& mode)
{
  if (rtc::read().isWindowOpen) {
    rtc::setRestoreMode(false);
  }

  if (mode == rtc::read().mode) {
    return;
  }

  rtc::setMode(mode);
  updateConfig();
  setNextCheckTimeNow();

  for (const auto& handler : m_OpModeChangeHandler) {
    handler(rtc::read().mode);
  }
}

config::OperationMode RadiatorValve::getMode()
{
  return rtc::read().mode;
}

const char* RadiatorValve::modeToCharArray(const config::OperationMode& mode)
{
  if (mode == config::OperationMode::HEAT) {
    return "heat";
  }
  if (mode == config::OperationMode::OFF) {
    return "off";
  }

  return "unknown";
}

void RadiatorValve::registerSetTempChangedHandler(
  const std::function<void(float)>& handler)
{
  m_setTempChangeHandler.push_back(handler);
}

void RadiatorValve::registerModeChangedHandler(
  const std::function<void(const config::OperationMode&)>& handler)
{
  m_OpModeChangeHandler.emplace_back(handler);
}

void RadiatorValve::setWindowState(const bool isOpen)
{
  if (isOpen == rtc::read().isWindowOpen) {
    m_logger.log(yal::Level::DEBUG, "Window mode % already set", isOpen);
    return;
  }

  if (isOpen) {
    m_logger.log(yal::Level::DEBUG, "Storing mode, window open");
    rtc::setLastMode(rtc::read().mode);
    setMode(config::OperationMode::OFF);
    rtc::setRestoreMode(true);
  } else {
    if (rtc::read().restoreMode) {
      m_logger.log(yal::Level::DEBUG, "Restoring mode, window closed");
      rtc::setValveNextCheckMillis(rtc::offsetMillis() + SLEEP_MILLIS_AFTER_WINDOW_CLOSE);
      setMode(rtc::read().lastMode);
    } else {
      m_logger.log(
        yal::Level::DEBUG, "Mode changed while window was open, not enabled old mode");
    }
  }

  for (const auto& stateHandler : m_windowStateHandler) {
    stateHandler(isOpen);
  }

  rtc::setIsWindowOpen(isOpen);
}

void RadiatorValve::registerWindowChangeHandler(const std::function<void(bool)>& handler)
{
  m_windowStateHandler.push_back(handler);
}

void RadiatorValve::setNextCheckTimeNow()
{
  rtc::setValveNextCheckMillis(0);
}
} // namespace open_heat::heating
