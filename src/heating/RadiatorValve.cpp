//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#include "RadiatorValve.hpp"
#include <RTCMemory.hpp>

open_heat::heating::RadiatorValve::RadiatorValve(
  open_heat::sensors::Temperature*& tempSensor,
  open_heat::Filesystem& filesystem) :
    m_filesystem(filesystem), m_temperatureSensor(tempSensor), m_logger("VALVE")
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
  if (rtc::read().mode == OFF || rtc::read().mode == FULL_OPEN) {
    const auto nextCheck = std::numeric_limits<uint64_t>::max();
    rtc::setValveNextCheckMillis(nextCheck);
    rtc::read().mode == OFF ? closeValve(VALVE_FULL_ROTATE_TIME * 2)
                            : openValve(VALVE_FULL_ROTATE_TIME * 2);
    
    m_logger.log(yal::Level::DEBUG, "Heating is turned off, disabled heating");
    return nextCheck;
  }

  if (rtc::read().mode == UNKNOWN) {
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

void open_heat::heating::RadiatorValve::handleTempTooHigh(
  const open_heat::rtc::Memory& rtcData,
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

void open_heat::heating::RadiatorValve::handleTempTooLow(
  const open_heat::rtc::Memory& rtcData,
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
uint64_t open_heat::heating::RadiatorValve::nextCheckTime()
{
  const auto nextCheck = rtc::offsetMillis() + m_checkIntervalMillis;
  rtc::setValveNextCheckMillis(nextCheck);
  return nextCheck;
}

float open_heat::heating::RadiatorValve::getConfiguredTemp()
{
  return rtc::read().setTemp;
}

void open_heat::heating::RadiatorValve::setConfiguredTemp(float temp)
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
  const auto& config = m_filesystem.getConfig().MotorPins;

  m_logger.log(
    yal::Level::DEBUG,
    "Closing valve for %ms, currentRotateTime: %, vin: %, ground: %",
    rotateTime,
    rtc::read().currentRotateTime,
    config.Vin,
    config.Ground);

  rotateValve(rotateTime, config, HIGH, LOW);
}

void open_heat::heating::RadiatorValve::openValve(unsigned int rotateTime)
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
  const auto& config = m_filesystem.getConfig().MotorPins;

  m_logger.log(
    yal::Level::DEBUG,
    "Opening valve for %ms, currentRotateTime: %ms, vin: %, ground: %",
    rotateTime,
    rtc::read().currentRotateTime,
    config.Vin,
    config.Ground);

  rotateValve(rotateTime, config, LOW, HIGH);
}

unsigned int open_heat::heating::RadiatorValve::remainingRotateTime(
  int rotateTime,
  bool close)
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

void open_heat::heating::RadiatorValve::rotateValve(
  unsigned int rotateTime,
  const PinSettings& config,
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
    m_logger.log(yal::Level::DEBUG, "Window mode % already set", isOpen);
    return;
  }

  if (isOpen) {
    m_logger.log(yal::Level::DEBUG, "Storing mode, window open");
    rtc::setLastMode(rtc::read().mode);
    setMode(OFF);
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

void open_heat::heating::RadiatorValve::registerWindowChangeHandler(
  const std::function<void(bool)>& handler)
{
  m_windowStateHandler.push_back(handler);
}

void open_heat::heating::RadiatorValve::setNextCheckTimeNow()
{
  rtc::setValveNextCheckMillis(0);
}
