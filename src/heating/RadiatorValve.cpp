//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#include "RadiatorValve.hpp"
#include <Logger.hpp>
#include <RTCMemory.hpp>

open_heat::heating::RadiatorValve::RadiatorValve(
  open_heat::sensors::ITemperatureSensor& tempSensor,
  open_heat::Filesystem& filesystem) :
    filesystem_(filesystem), tempSensor_(tempSensor)
{
}

void open_heat::heating::RadiatorValve::setup()
{
  disablePins();
}

unsigned long open_heat::heating::RadiatorValve::loop()
{
  if (rtc::read().turnOff) {
    closeValve(VALVE_FULL_ROTATE_TIME);
    rtc::setTurnOff(false);
    rtc::setValveNextCheckMillis(rtc::offsetMillis() + checkIntervalMillis_);

    return rtc::read().valveNextCheckMillis;
  } else if (rtc::read().openFully) {
    openValve(VALVE_FULL_ROTATE_TIME);

    rtc::setOpenFully(false);
    rtc::setValveNextCheckMillis(rtc::offsetMillis() + checkIntervalMillis_);
    return rtc::read().valveNextCheckMillis;
  }

  if (rtc::offsetMillis() < rtc::read().valveNextCheckMillis) {
    return rtc::read().valveNextCheckMillis;
  }

  Logger::log(Logger::DEBUG, "trying to reach temperature: %f", rtc::read().setTemp);

  const auto temp = tempSensor_.getTemperature();
  const float predictPart = PREDICTION_STEEPNESS * (temp - rtc::read().lastMeasuredTemp);
  const float predictTemp = temp + predictPart;
  Logger::log(
    Logger::DEBUG,
    "Predicted temp %.2f in %lu ms, predict part: %.2f",
    predictTemp,
    checkIntervalMillis_,
    predictPart);

  rtc::setLastMeasuredTemp(temp);
  rtc::setLastPredictedTemp(predictTemp);
  if (rtc::read().mode != HEAT) {
    rtc::setValveNextCheckMillis(std::numeric_limits<unsigned long>::max());

    Logger::log(Logger::DEBUG, "Heating is turned off, disable heating");
    return rtc::read().valveNextCheckMillis;
  }

  const auto openHysteresis = 0.3f;
  const auto closeHysteresis = 0.2f;
  unsigned short openTime = 350U;
  unsigned short closeTime = 200U;

  // If valve was opened waiting time is increased
  unsigned long additionalWaitTime = 0UL;

  float predictionError = temp - rtc::read().lastPredictedTemp;
  Logger::log(
    Logger::DEBUG,
    "Predicted temperature %.2f, actual %.2f, error %.2f",
    rtc::read().lastPredictedTemp,
    temp,
    predictionError);

  if (0 == predictTemp) {
    rtc::setValveNextCheckMillis(
      rtc::offsetMillis() + checkIntervalMillis_ + additionalWaitTime);
    rtc::setLastPredictedTemp(temp);

    Logger::log(Logger::DEBUG, "Skipping temperature setting, predictTemp = 0");
    return rtc::read().valveNextCheckMillis;
  }

  const float temperatureChange = temp - rtc::read().lastMeasuredTemp;
  const float minTemperatureChange = 0.2;

  const float largeTempDiff = 3;

  // Act according to the prediction.
  if (predictTemp < (rtc::read().setTemp - openHysteresis)) {
    if (temperatureChange < minTemperatureChange) {

      const auto predictDiff = rtc::read().setTemp - predictTemp - openHysteresis;
      Logger::log(Logger::INFO, "Open predict diff: %f", predictDiff);

      if (
        rtc::read().setTemp - predictTemp > largeTempDiff
        && rtc::read().setTemp - temp > largeTempDiff) {
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

      openValve(openTime);
    } else {
      Logger::log(
        Logger::INFO,
        "RISE, NO ADJUST: Temp old %.2f, temp now %.2f, temp change %.2f",
        rtc::read().lastMeasuredTemp,
        temp,
        temperatureChange);
    }

  } else if (predictTemp > (rtc::read().setTemp + closeHysteresis)) {
    if (temperatureChange >= -minTemperatureChange) {

      const auto predictDiff = rtc::read().setTemp - predictTemp - closeHysteresis;
      Logger::log(Logger::INFO, "Close predict diff: %f", predictDiff);
      if (predictDiff <= 2) {
        closeTime = 5000;
      } else if (predictDiff <= 1.5) {
        closeTime = 4000;
      } else if (predictDiff <= 1) {
        closeTime = 2500;
      } else if (predictDiff <= 0.5) {
        closeTime = 1500;
      }

      closeValve(closeTime);
    } else {
      Logger::log(
        Logger::INFO,
        "SINK, NO ADJUST: Temp old %.2f, temp now %.2f, temp change %.2f",
        rtc::read().lastMeasuredTemp,
        temp,
        temperatureChange);
    }
  } else {
    Logger::log(Logger::INFO, "Temperature is in tolerance, not changing");
  }
  if (std::abs(rtc::read().currentRotateTime) >= VALVE_FULL_ROTATE_TIME) {
    additionalWaitTime = 30 * 1000;
  }

  rtc::setValveNextCheckMillis(
    rtc::offsetMillis() + checkIntervalMillis_ + additionalWaitTime);
  return rtc::read().valveNextCheckMillis;
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

  if (0 == temp) {
    rtc::setTurnOff(true);
  }

  open_heat::Logger::log(open_heat::Logger::INFO, "New target temperature %f", temp);
  rtc::setSetTemp(temp);
  rtc::setValveNextCheckMillis(0);

  updateConfig();

  for (const auto& handler : setTempChangedHandler_) {
    handler(temp);
  }
}

void open_heat::heating::RadiatorValve::updateConfig()
{
  const auto rtcMem = rtc::read();
  auto& config = filesystem_.getConfig();
  config.SetTemperature = rtcMem.setTemp;
  filesystem_.persistConfig();
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
    auto remainingTime
      = VALVE_FULL_ROTATE_TIME - std::abs(rtc::read().currentRotateTime);
    if (remainingTime < 0) {
      remainingTime = 0;
    }
    rotateTime = std::min(rotateTime, static_cast<unsigned int>(remainingTime));
  }

  rtc::setCurrentRotateTime(
    rtc::read().currentRotateTime - rotateTime, VALVE_FULL_ROTATE_TIME);
  const auto& config = filesystem_.getConfig().MotorPins;

  open_heat::Logger::log(
    open_heat::Logger::DEBUG,
    "Closing valve for %ims, currentRotateTime: %ims, vin: %i, ground: %i",
    rotateTime,
    rtc::read().currentRotateTime,
    config.Vin,
    config.Ground);

  enablePins();
  digitalWrite(static_cast<uint8_t>(config.Vin), HIGH);
  digitalWrite(static_cast<uint8_t>(config.Ground), LOW);
  delay(rotateTime);
  disablePins();
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
    auto remainingTime
      = VALVE_FULL_ROTATE_TIME - std::abs(rtc::read().currentRotateTime);
    if (remainingTime < 0) {
      remainingTime = 0;
    }

    rotateTime = std::min(rotateTime, static_cast<unsigned int>(remainingTime));
  }

  rtc::setCurrentRotateTime(
    rtc::read().currentRotateTime + rotateTime, VALVE_FULL_ROTATE_TIME);
  const auto& config = filesystem_.getConfig().MotorPins;

  open_heat::Logger::log(
    open_heat::Logger::DEBUG,
    "Opening valve for %ims, currentRotateTime: %ims, vin: %i, ground: %i",
    rotateTime,
    rtc::read().currentRotateTime,
    config.Vin,
    config.Ground);

  enablePins();
  digitalWrite(static_cast<uint8_t>(config.Vin), LOW);
  digitalWrite(static_cast<uint8_t>(config.Ground), HIGH);
  delay(rotateTime);
  disablePins();
}

void open_heat::heating::RadiatorValve::disablePins()
{
  const auto& config = filesystem_.getConfig().MotorPins;

  digitalWrite(static_cast<uint8_t>(config.Vin), LOW);
  digitalWrite(static_cast<uint8_t>(config.Ground), LOW);

  pinMode(static_cast<uint8_t>(config.Vin), INPUT);
  pinMode(static_cast<uint8_t>(config.Ground), INPUT);
}

void open_heat::heating::RadiatorValve::enablePins()
{
  const auto& config = filesystem_.getConfig().MotorPins;

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
  auto& config = filesystem_.getConfig();
  config.Mode = rtc::read().mode;
  filesystem_.persistConfig();

  switch (mode) {
  case OFF:
    rtc::setTurnOff(true);
    break;
    case FULL_OPEN:
      rtc::setOpenFully(true);
      rtc::setMode(HEAT);
      case HEAT:
        rtc::setValveNextCheckMillis(rtc::offsetMillis());
        default:
          break;
  }

  for (const auto& handler : opModeChangedHandler_) {
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
  setTempChangedHandler_.push_back(handler);
}

void open_heat::heating::RadiatorValve::registerModeChangedHandler(
  const std::function<void(OperationMode)>& handler)
{
  opModeChangedHandler_.push_back(handler);
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
      rtc::setValveNextCheckMillis(rtc::offsetMillis() + sleepMillisAfterWindowClose_);
      setMode(rtc::read().lastMode);
    } else {
      Logger::log(
        Logger::DEBUG, "Mode changed while window was open, not enabled old mode");
    }
  }

  for (const auto& stateHandler : windowStateHandler_) {
    stateHandler(isOpen);
  }

  rtc::setIsWindowOpen(isOpen);
}

void open_heat::heating::RadiatorValve::registerWindowChangeHandler(
  const std::function<void(bool)>& handler)
{
  windowStateHandler_.push_back(handler);
}
