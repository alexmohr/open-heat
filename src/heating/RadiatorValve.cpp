//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#include "RadiatorValve.hpp"
#include <Arduino.h>
#include <Logger.hpp>

open_heat::heating::RadiatorValve::RadiatorValve(
  open_heat::sensors::ITemperatureSensor& tempSensor,
  open_heat::Filesystem& filesystem) :
    filesystem_(filesystem), tempSensor_(tempSensor), setTemp_(20)
{
  setPinsLow();
}

void open_heat::heating::RadiatorValve::setup()
{
  const auto& config = filesystem_.getConfig();
  setTemp_ = config.SetTemperature;
  lastTemp_ = tempSensor_.getTemperature();
  mode_ = config.Mode;

  pinMode(config.MotorPins.Vin, OUTPUT);
  pinMode(config.MotorPins.Ground, OUTPUT);
}

unsigned long open_heat::heating::RadiatorValve::loop()
{
  if (turnOff_) {
    closeValve(VALVE_FULL_ROTATE_TIME);
    turnOff_ = false;
    nextCheckMillis_ = millis() + checkIntervalMillis_;
    return nextCheckMillis_;
  } else if (openFully_) {
    openValve(VALVE_FULL_ROTATE_TIME);
    openFully_ = false;
    nextCheckMillis_ = millis() + checkIntervalMillis_;
    return nextCheckMillis_;
  }

  if (millis() < nextCheckMillis_) {
    return nextCheckMillis_;
  }

  const auto temp = tempSensor_.getTemperature();
  float predictPart = PREDICTION_STEEPNESS * (temp - lastTemp_);
  float predictTemp = temp + predictPart;
  Logger::log(
    Logger::DEBUG,
    "Predicted temp %.2f in %lu ms, predict part: %.2f",
    predictTemp,
    checkIntervalMillis_,
    predictPart);

  if (mode_ != HEAT) {
    lastTemp_ = temp;
    nextCheckMillis_ = millis() + checkIntervalMillis_ * 10;
    return nextCheckMillis_;
  }

  // 0.5 works; 0.3 both works
  const auto openHysteresis = 0.3f;
  const auto closeHysteresis = 0.2f;
  short openTime = 350;
  short closeTime = 200;

  // If valve was opened waiting time is increased
  unsigned long additionalWaitTime = 0;

  float predictionError = temp - lastPredictTemp_;
  Logger::log(
    Logger::DEBUG,
    "Predicted temperature %.2f, actual %.2f, error %.2f",
    lastPredictTemp_,
    temp,
    predictionError);

  if (0 == predictTemp) {
    nextCheckMillis_ = millis() + checkIntervalMillis_ + additionalWaitTime;
    lastPredictTemp_ = predictTemp;
    lastTemp_ = temp;
    return nextCheckMillis_;
  }

  const float temperatureChange = temp - lastTemp_;
  const float minTemperatureChange = 0.2;

  const float lageTempDiff = 1;

  // Act according to the prediction.
  if (predictTemp < (setTemp_ - openHysteresis)) {
    if (temperatureChange < minTemperatureChange) {

      const auto predictDiff = setTemp_ - predictTemp - openHysteresis;
      Logger::log(Logger::INFO, "Open predict diff: %f", predictDiff);

      if (setTemp_ - predictTemp > lageTempDiff && setTemp_ - temp > lageTempDiff) {
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
        lastTemp_,
        temp,
        temperatureChange);
    }

  } else if (predictTemp > (setTemp_ + closeHysteresis)) {
    if (temperatureChange >= -minTemperatureChange) {

      const auto predictDiff = setTemp_ - predictTemp - closeHysteresis;
      Logger::log(Logger::INFO, "Close predict diff: %f", predictDiff);
      if (predictDiff <= 1) {
        closeTime = 600;
      } else if (predictDiff <= 0.5) {
        closeTime = 300;
      }

      closeValve(closeTime);
    } else {
      Logger::log(
        Logger::INFO,
        "SINK, NO ADJUST: Temp old %.2f, temp now %.2f, temp change %.2f",
        lastTemp_,
        temp,
        temperatureChange);
    }
  }

  lastPredictTemp_ = predictTemp;
  lastTemp_ = temp;

  if (std::abs(currentRotateTime_) >= VALVE_FULL_ROTATE_TIME) {
    additionalWaitTime = 30 * 1000;
  }

  nextCheckMillis_ = millis() + checkIntervalMillis_ + additionalWaitTime;
  return nextCheckMillis_;
}

float open_heat::heating::RadiatorValve::getConfiguredTemp() const
{
  return setTemp_;
}

void open_heat::heating::RadiatorValve::setConfiguredTemp(float temp)
{
  if (temp == setTemp_) {
    return;
  }

  if (0 == temp) {
    turnOff_ = true;
  }

  open_heat::Logger::log(open_heat::Logger::INFO, "New target temperature %f", temp);
  setTemp_ = temp;
  nextCheckMillis_ = 0;

  updateConfig();

  for (const auto& handler : setTempChangedHandler_) {
    handler(setTemp_);
  }
}

void open_heat::heating::RadiatorValve::updateConfig()
{
  auto& config = this->filesystem_.getConfig();
  config.SetTemperature = this->setTemp_;
  this->filesystem_.persistConfig();
}

void open_heat::heating::RadiatorValve::closeValve(unsigned short rotateTime)
{
  if (currentRotateTime_ <= -VALVE_FULL_ROTATE_TIME) {
    open_heat::Logger::log(open_heat::Logger::DEBUG, "Valve already fully closed");
    return;
  }

  currentRotateTime_ -= rotateTime;

  const auto& config = filesystem_.getConfig().MotorPins;

  open_heat::Logger::log(
    open_heat::Logger::DEBUG,
    "Closing valve for %ims, currentRotateTime: %ims",
    rotateTime,
    currentRotateTime_);
  digitalWrite(config.Vin, LOW);
  digitalWrite(config.Ground, HIGH);

  delay(rotateTime);

  setPinsLow();
}

void open_heat::heating::RadiatorValve::openValve(unsigned short rotateTime)
{
  if (currentRotateTime_ >= VALVE_FULL_ROTATE_TIME) {
    open_heat::Logger::log(open_heat::Logger::DEBUG, "Valve already fully open");
    return;
  }

  currentRotateTime_ += rotateTime;

  const auto& config = filesystem_.getConfig().MotorPins;

  open_heat::Logger::log(
    open_heat::Logger::DEBUG,
    "Opening valve for %ims, currentRotateTime: %ims",
    rotateTime,
    currentRotateTime_);

  digitalWrite(config.Ground, LOW);
  digitalWrite(config.Vin, HIGH);

  delay(rotateTime);

  setPinsLow();
}

void open_heat::heating::RadiatorValve::setPinsLow()
{
  const auto& config = filesystem_.getConfig().MotorPins;

  digitalWrite(config.Vin, LOW);
  digitalWrite(config.Ground, LOW);
}

void open_heat::heating::RadiatorValve::setMode(OperationMode mode)
{
  if (mode == mode_) {
    return;
  }

  Logger::log(Logger::INFO, "Valve, new mode: %s", modeToCharArray(mode));
  mode_ = mode;

  switch (mode) {
  case OFF:
    turnOff_ = true;
    break;
  case FULL_OPEN:
    openFully_ = true;
    mode_ = HEAT;
  case HEAT:
    nextCheckMillis_ = millis() + checkIntervalMillis_;
  default:
    break;
  }

  if (isWindowOpen_) {
    restoreMode_ = false;
  }

  auto& config = filesystem_.getConfig();
  config.Mode = mode_;
  filesystem_.persistConfig();

  for (const auto& handler : opModeChangedHandler_) {
    handler(mode_);
  }
}
OperationMode open_heat::heating::RadiatorValve::getMode()
{
  return mode_;
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
  if (isOpen == isWindowOpen_) {
    Logger::log(Logger::DEBUG, "Window mode %i already set", isOpen);
    return;
  }

  if (isOpen) {
    Logger::log(Logger::DEBUG, "Storing mode, window open");
    lastMode_ = mode_;
    setMode(OFF);
    restoreMode_ = true;
  } else {
    if (restoreMode_) {
      Logger::log(Logger::DEBUG, "Restoring mode, window closed");
      nextCheckMillis_ += sleepMillisAfterWindowClose_;
      setMode(lastMode_);
    } else {
      Logger::log(
        Logger::DEBUG, "Mode changed while window was open, not enabled old mode");
    }
  }

  for (const auto& stateHandler : windowStateHandler_) {
    stateHandler(isOpen);
  }

  isWindowOpen_ = isOpen;
}

void open_heat::heating::RadiatorValve::registerWindowChangeHandler(
  const std::function<void(bool)>& handler)
{
  windowStateHandler_.push_back(handler);
}
