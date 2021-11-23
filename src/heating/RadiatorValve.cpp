//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#include "RadiatorValve.hpp"
#include <Logger.hpp>
#include <RTCMemory.hpp>

open_heat::heating::RadiatorValve::RadiatorValve(
  open_heat::sensors::Temperature*& tempSensor,
  open_heat::Filesystem& filesystem) :
    m_filesystem(filesystem), m_temperatureSensor(tempSensor)
{
}

void open_heat::heating::RadiatorValve::setup()
{
  disablePins();
}

uint64_t open_heat::heating::RadiatorValve::loop()
{
  // no check necessary yet
  if (rtc::offsetMillis() < rtc::read().valveNextCheckMillis) {
    return rtc::read().valveNextCheckMillis;
  }

  // heating disabled
  if (rtc::read().mode != HEAT) {
    const auto nextCheck = std::numeric_limits<uint64_t>::max();
    rtc::setValveNextCheckMillis(nextCheck);
    closeValve(VALVE_FULL_ROTATE_TIME);

    Logger::log(Logger::DEBUG, "Heating is turned off, disabled heating");
    return nextCheck;
  }

  // store data once to work with consistent values
  const auto rtcData = rtc::read();
  Logger::log(Logger::DEBUG, "trying to reach temperature: %f", rtcData.setTemp);

  // also updates last measured temp
  const auto measuredTemp = m_temperatureSensor->temperature();
  const float predictPart
    = PREDICTION_STEEPNESS * (measuredTemp - rtcData.lastMeasuredTemp);
  const float predictTemp = measuredTemp + predictPart;
  const auto predictionError = measuredTemp - rtcData.lastPredictedTemp;
  const auto temperatureChange = measuredTemp - rtcData.lastMeasuredTemp;
  const auto minTemperatureChange = 0.2;

  const auto hysteresis = 0.2f;
  const auto predictDiff = rtcData.setTemp - predictTemp - hysteresis;
  const auto absTempDiff
    = std::max(rtcData.setTemp, predictTemp) - std::min(rtcData.setTemp, predictTemp);

  rtc::setLastPredictedTemp(predictTemp);

  Logger::log(
    Logger::INFO,
    "Valve loop\n"
    "\tpredictTemp %.2f in %lu ms\n"
    "\tpredictPart: %.2f, predictionError: %.2f, predictDiff: %.2f\n"
    "\tmeasuredTemp: %.2f, lastMeasuredTemp %.2f, setTemp %.2f \n"
    "\ttemperatureChange %.2f, absTempDiff: %.2f",
    predictTemp,
    m_checkIntervalMillis,
    predictPart,
    predictionError,
    predictDiff,
    measuredTemp,
    rtcData.lastMeasuredTemp,
    rtcData.setTemp,
    temperatureChange,
    absTempDiff);

  if (0 == predictTemp) {
    const auto nextCheck = rtc::offsetMillis() + m_checkIntervalMillis;
    rtc::setValveNextCheckMillis(nextCheck);
    Logger::log(Logger::DEBUG, "Skipping temperature setting, predictTemp = 0");
    return nextCheck;
  }

  if (absTempDiff < hysteresis) {
    Logger::log(Logger::INFO, "Predicated temperature is in tolerance, not changing");
    return nextCheckTime();
  }

  const auto rotateFactorOpen = 1200.0f;
  const auto rotateFactorClose = rotateFactorOpen + 300.0f;
  const auto predictedTempTooLow = predictTemp < (rtcData.setTemp - hysteresis);
  const auto rotateFactor = predictedTempTooLow ? rotateFactorOpen : rotateFactorClose;

  // limit valve rotate time to something sane
  const auto maxRotateTime = 10'000;
  auto rotateTime = std::abs(predictDiff) * rotateFactor;
  if (rotateTime > maxRotateTime) {
    rotateTime = maxRotateTime;
  }

  const auto temperatureChangedEnough
    = (predictedTempTooLow && temperatureChange >= (minTemperatureChange))
    || (!predictedTempTooLow && temperatureChange <= (-minTemperatureChange));

  if (temperatureChangedEnough) {
    Logger::log(Logger::INFO, "Temperature changed enough by %.2f", temperatureChange);
    return nextCheckTime();
  }

  if (predictedTempTooLow) {
    openValve(rotateTime);
  } else {
    closeValve(rotateTime);
  }

  return nextCheckTime();
}
uint64_t open_heat::heating::RadiatorValve::nextCheckTime() const
{
  const auto nextCheck = rtc::offsetMillis() + m_checkIntervalMillis;
  rtc::setValveNextCheckMillis(nextCheck);
  return nextCheck;
}

float open_heat::heating::RadiatorValve::getConfiguredTemp() const
{
  return rtc::read().setTemp;
}

void open_heat::heating::RadiatorValve::setConfiguredTemp(float temp)
{
  if (temp == rtc::read().setTemp) {
    return;
  }

  open_heat::Logger::log(open_heat::Logger::INFO, "New target temperature %f", temp);
  rtc::setSetTemp(temp);
  setNextCheckTimeNow();

  updateConfig();

  for (const auto& handler : m_setTempChangeHandler) {
    handler(temp);
  }
}

void open_heat::heating::RadiatorValve::updateConfig()
{
  const auto rtcMem = rtc::read();
  auto& config = m_filesystem.getConfig();
  config.SetTemperature = rtcMem.setTemp;
  config.Mode = rtcMem.mode;
  m_filesystem.persistConfig();
}

void open_heat::heating::RadiatorValve::closeValve(unsigned int rotateTime)
{
  if (rtc::read().currentRotateTime <= (-VALVE_FULL_ROTATE_TIME)) {
    open_heat::Logger::log(
      open_heat::Logger::DEBUG,
      "Valve already fully closed, current rotate time: %i",
      rtc::read().currentRotateTime);
    return;
  }

  if (rtc::read().currentRotateTime < 0) {
    rotateTime = remainingRotateTime(rotateTime);
  }

  rtc::setCurrentRotateTime(
    rtc::read().currentRotateTime - rotateTime, VALVE_FULL_ROTATE_TIME);
  const auto& config = m_filesystem.getConfig().MotorPins;

  open_heat::Logger::log(
    open_heat::Logger::DEBUG,
    "Closing valve for %ims, currentRotateTime: %ims, vin: %i, ground: %i",
    rotateTime,
    rtc::read().currentRotateTime,
    config.Vin,
    config.Ground);

  rotateValve(rotateTime, config, HIGH, LOW);
}

void open_heat::heating::RadiatorValve::openValve(unsigned int rotateTime)
{
  if (rtc::read().currentRotateTime >= VALVE_FULL_ROTATE_TIME) {
    open_heat::Logger::log(
      open_heat::Logger::DEBUG,
      "Valve already fully open, current rotate time: %i",
      rtc::read().currentRotateTime);
    return;
  }

  if (rtc::read().currentRotateTime > 0) {
    rotateTime = remainingRotateTime(rotateTime);
  }

  rtc::setCurrentRotateTime(
    rtc::read().currentRotateTime + rotateTime, VALVE_FULL_ROTATE_TIME);
  const auto& config = m_filesystem.getConfig().MotorPins;

  open_heat::Logger::log(
    open_heat::Logger::DEBUG,
    "Opening valve for %ims, currentRotateTime: %ims, vin: %i, ground: %i",
    rotateTime,
    rtc::read().currentRotateTime,
    config.Vin,
    config.Ground);

  rotateValve(rotateTime, config, LOW, HIGH);
}

unsigned int open_heat::heating::RadiatorValve::remainingRotateTime(
  unsigned int rotateTime) const
{
  auto remainingTime = VALVE_FULL_ROTATE_TIME - abs(rtc::read().currentRotateTime);
  if (remainingTime < 0) {
    remainingTime = 0;
  }

  rotateTime = std::min(rotateTime, static_cast<unsigned int>(remainingTime));
  return rotateTime;
}

void open_heat::heating::RadiatorValve::rotateValve(
  unsigned int rotateTime,
  const PinSettings& config,
  int vinState,
  int groundState)
{
  enablePins();
  digitalWrite(static_cast<uint8_t>(config.Vin), vinState);
  digitalWrite(static_cast<uint8_t>(config.Ground), groundState);
  delay(rotateTime);
  disablePins();
  Logger::log(Logger::DEBUG, "Rotating valve done");
}

void open_heat::heating::RadiatorValve::disablePins()
{
  const auto& config = m_filesystem.getConfig().MotorPins;

  digitalWrite(static_cast<uint8_t>(config.Vin), LOW);
  digitalWrite(static_cast<uint8_t>(config.Ground), LOW);

  pinMode(static_cast<uint8_t>(config.Vin), INPUT);
  pinMode(static_cast<uint8_t>(config.Ground), INPUT);
}

void open_heat::heating::RadiatorValve::enablePins()
{
  const auto& config = m_filesystem.getConfig().MotorPins;

  pinMode(static_cast<uint8_t>(config.Vin), OUTPUT);
  pinMode(static_cast<uint8_t>(config.Ground), OUTPUT);
}

void open_heat::heating::RadiatorValve::setMode(const OperationMode mode)
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

OperationMode open_heat::heating::RadiatorValve::getMode()
{
  return rtc::read().mode;
}

const char* open_heat::heating::RadiatorValve::modeToCharArray(const OperationMode mode)
{
  if (mode == HEAT) {
    return "heat";
  } else if (mode == OFF) {
    return "off";
  } else {
    return "unknown";
  }
}

void open_heat::heating::RadiatorValve::registerSetTempChangedHandler(
  const std::function<void(float)>& handler)
{
  m_setTempChangeHandler.push_back(handler);
}

void open_heat::heating::RadiatorValve::registerModeChangedHandler(
  const std::function<void(OperationMode)>& handler)
{
  m_OpModeChangeHandler.push_back(handler);
}

void open_heat::heating::RadiatorValve::setWindowState(const bool isOpen)
{
  if (isOpen == rtc::read().isWindowOpen) {
    Logger::log(Logger::DEBUG, "Window mode %i already set", isOpen);
    return;
  }

  if (isOpen) {
    Logger::log(Logger::DEBUG, "Storing mode, window open");
    rtc::setLastMode(rtc::read().mode);
    setMode(OFF);
    rtc::setRestoreMode(true);
  } else {
    if (rtc::read().restoreMode) {
      Logger::log(Logger::DEBUG, "Restoring mode, window closed");
      rtc::setValveNextCheckMillis(rtc::offsetMillis() + SLEEP_MILLIS_AFTER_WINDOW_CLOSE);
      setMode(rtc::read().lastMode);
    } else {
      Logger::log(
        Logger::DEBUG, "Mode changed while window was open, not enabled old mode");
    }
  }

  for (const auto& stateHandler : m_windowStateHandler) {
    stateHandler(isOpen);
  }

  rtc::setIsWindowOpen(isOpen);
}

void open_heat::heating::RadiatorValve::registerWindowChangeHandler(
  const std::function<void(bool)>& handler)
{
  m_windowStateHandler.push_back(handler);
}

void open_heat::heating::RadiatorValve::setNextCheckTimeNow()
{
  rtc::setValveNextCheckMillis(0);
}
