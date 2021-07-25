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
    filesystem_(filesystem),
    tempSensor_(tempSensor)
{
}

void open_heat::heating::RadiatorValve::setup()
{
  auto rtcMem = readRTCMemory();
  const auto& config = filesystem_.getConfig();
  rtcMem.setTemp = config.SetTemperature;
  rtcMem.lastMeasuredTemp = tempSensor_.getTemperature();
  rtcMem.mode = config.Mode;

  writeRTCMemory(rtcMem);
  setPinsLow();
}

unsigned long open_heat::heating::RadiatorValve::loop()
{
  auto rtcMem = readRTCMemory();
  if (rtcMem.turnOff) {
    closeValve(VALVE_FULL_ROTATE_TIME);
    rtcMem.turnOff = false;
    rtcMem.valveNextCheckMillis = offsetMillis() + checkIntervalMillis_;
    writeRTCMemory(rtcMem);
    return rtcMem.valveNextCheckMillis;
  } else if (rtcMem.openFully) {
    openValve(VALVE_FULL_ROTATE_TIME);
    rtcMem.openFully = false;
    rtcMem.valveNextCheckMillis = offsetMillis() + checkIntervalMillis_;
    writeRTCMemory(rtcMem);
    return rtcMem.valveNextCheckMillis;
  }

  if (offsetMillis() < rtcMem.valveNextCheckMillis) {
    return rtcMem.valveNextCheckMillis;
  }

  const auto temp = tempSensor_.getTemperature();
  const float predictPart = PREDICTION_STEEPNESS * (temp - rtcMem.lastMeasuredTemp);
  const float predictTemp = temp + predictPart;
  Logger::log(
    Logger::DEBUG,
    "Predicted temp %.2f in %lu ms, predict part: %.2f",
    predictTemp,
    checkIntervalMillis_,
    predictPart);

  if (rtcMem.mode != HEAT) {
    rtcMem.lastMeasuredTemp = temp;
    rtcMem.valveNextCheckMillis = std::numeric_limits<unsigned long>::max();
    writeRTCMemory(rtcMem);
    return rtcMem.valveNextCheckMillis;
  }

  const auto openHysteresis = 0.3f;
  const auto closeHysteresis = 0.2f;
  unsigned short openTime = 350U;
  unsigned short closeTime = 200U;

  // If valve was opened waiting time is increased
  unsigned long additionalWaitTime = 0;

  float predictionError = temp - rtcMem.lastPredictedTemp;
  Logger::log(
    Logger::DEBUG,
    "Predicted temperature %.2f, actual %.2f, error %.2f",
    rtcMem.lastPredictedTemp,
    temp,
    predictionError);

  if (0 == predictTemp) {
    rtcMem.valveNextCheckMillis = offsetMillis() + checkIntervalMillis_ + additionalWaitTime;
    rtcMem.lastPredictedTemp = predictTemp;
    rtcMem.lastMeasuredTemp = temp;
    writeRTCMemory(rtcMem);
    return rtcMem.valveNextCheckMillis;
  }

  const float temperatureChange = temp - rtcMem.lastMeasuredTemp;
  const float minTemperatureChange = 0.2;

  const float lageTempDiff = 1;

  // Act according to the prediction.
  if (predictTemp < (rtcMem.setTemp - openHysteresis)) {
    if (temperatureChange < minTemperatureChange) {

      const auto predictDiff = rtcMem.setTemp - predictTemp - openHysteresis;
      Logger::log(Logger::INFO, "Open predict diff: %f", predictDiff);

      if (rtcMem.setTemp - predictTemp > lageTempDiff && rtcMem.setTemp - temp > lageTempDiff) {
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
        rtcMem.lastMeasuredTemp,
        temp,
        temperatureChange);
    }

  } else if (predictTemp > (rtcMem.setTemp + closeHysteresis)) {
    if (temperatureChange >= -minTemperatureChange) {

      const auto predictDiff = rtcMem.setTemp - predictTemp - closeHysteresis;
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
        rtcMem.lastMeasuredTemp,
        temp,
        temperatureChange);
    }
  }

  rtcMem.lastPredictedTemp = predictTemp;
  rtcMem.lastMeasuredTemp = temp;

  if (std::abs(rtcMem.currentRotateTime) >= VALVE_FULL_ROTATE_TIME) {
    additionalWaitTime = 30 * 1000;
  }

  rtcMem.valveNextCheckMillis = offsetMillis() + checkIntervalMillis_ + additionalWaitTime;
  writeRTCMemory(rtcMem);
  return rtcMem.valveNextCheckMillis;
}

float open_heat::heating::RadiatorValve::getConfiguredTemp() const
{
  const auto rtcMem = readRTCMemory();
  return rtcMem.setTemp;
}

void open_heat::heating::RadiatorValve::setConfiguredTemp(float temp)
{
  auto rtcMem = readRTCMemory();
  if (temp == rtcMem.setTemp) {
    return;
  }

  if (0 == temp) {
    rtcMem.turnOff = true;
  }

  open_heat::Logger::log(open_heat::Logger::INFO, "New target temperature %f", temp);
  rtcMem.setTemp = temp;
  rtcMem.valveNextCheckMillis = 0;
  writeRTCMemory(rtcMem);

  updateConfig();

  for (const auto& handler : setTempChangedHandler_) {
    handler(rtcMem.setTemp);
  }
}

void open_heat::heating::RadiatorValve::updateConfig()
{
  const auto rtcMem = readRTCMemory();
  auto& config = filesystem_.getConfig();
  config.SetTemperature = rtcMem.setTemp;
  filesystem_.persistConfig();
}

void open_heat::heating::RadiatorValve::closeValve(const unsigned short rotateTime)
{
  auto rtcMem = readRTCMemory();
  if (rtcMem.currentRotateTime <= -VALVE_FULL_ROTATE_TIME) {
    open_heat::Logger::log(open_heat::Logger::DEBUG, "Valve already fully closed");
    return;
  }

  rtcMem.currentRotateTime -= rotateTime;
  writeRTCMemory(rtcMem);

  const auto& config = filesystem_.getConfig().MotorPins;

  open_heat::Logger::log(
    open_heat::Logger::DEBUG,
    "Closing valve for %ims, currentRotateTime: %ims",
    rotateTime,
    rtcMem.currentRotateTime);
  digitalWrite(static_cast<uint8_t>(config.Vin), LOW);
  digitalWrite(static_cast<uint8_t>(config.Ground), HIGH);

  delay(rotateTime);

  setPinsLow();
}

void open_heat::heating::RadiatorValve::openValve(const unsigned short rotateTime)
{
  auto rtcMem = readRTCMemory();
  if (rtcMem.currentRotateTime >= VALVE_FULL_ROTATE_TIME) {
    open_heat::Logger::log(open_heat::Logger::DEBUG, "Valve already fully open");
    return;
  }

  rtcMem.currentRotateTime += rotateTime;
  writeRTCMemory(rtcMem);

  const auto& config = filesystem_.getConfig().MotorPins;

  open_heat::Logger::log(
    open_heat::Logger::DEBUG,
    "Opening valve for %ims, currentRotateTime: %ims",
    rotateTime,
    rtcMem.currentRotateTime);

  digitalWrite(static_cast<uint8_t>(config.Ground), LOW);
  digitalWrite(static_cast<uint8_t>(config.Vin), HIGH);

  delay(rotateTime);

  setPinsLow();
}

void open_heat::heating::RadiatorValve::setPinsLow()
{
  const auto& config = filesystem_.getConfig().MotorPins;

  digitalWrite(static_cast<uint8_t>(config.Vin), LOW);
  digitalWrite(static_cast<uint8_t>(config.Ground), LOW);
}

void open_heat::heating::RadiatorValve::setMode(OperationMode mode)
{
  Logger::log(Logger::DEBUG, "Request to change mode to: %s", modeToCharArray(mode));


  auto rtcMem = readRTCMemory();
  if (mode == rtcMem.mode) {
    return;
  }

  Logger::log(Logger::INFO, "Valve, new mode: %s", modeToCharArray(mode));
  rtcMem.mode = mode;

  switch (mode) {
  case OFF:
    rtcMem.turnOff = true;
    break;
  case FULL_OPEN:
    rtcMem.openFully = true;
    rtcMem.mode = HEAT;
  case HEAT:
    rtcMem.valveNextCheckMillis = offsetMillis();
  default:
    break;
  }

  if (rtcMem.isWindowOpen) {
    rtcMem.restoreMode = false;
  }

  auto& config = filesystem_.getConfig();
  config.Mode = rtcMem.mode;
  filesystem_.persistConfig();
  writeRTCMemory(rtcMem);

  for (const auto& handler : opModeChangedHandler_) {
    handler(rtcMem.mode);
  }
}
OperationMode open_heat::heating::RadiatorValve::getMode()
{
  const auto rtcMem = readRTCMemory();
  return rtcMem.mode;
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
  auto rtcMem = readRTCMemory();
  if (isOpen == rtcMem.isWindowOpen) {
    Logger::log(Logger::DEBUG, "Window mode %i already set", isOpen);
    return;
  }

  if (isOpen) {
    Logger::log(Logger::DEBUG, "Storing mode, window open");
    rtcMem.lastMode = rtcMem.mode;
    setMode(OFF);
    rtcMem.restoreMode = true;
  } else {
    if (rtcMem.restoreMode) {
      Logger::log(Logger::DEBUG, "Restoring mode, window closed");
      rtcMem.valveNextCheckMillis += sleepMillisAfterWindowClose_;
      setMode(rtcMem.lastMode);
    } else {
      Logger::log(
        Logger::DEBUG, "Mode changed while window was open, not enabled old mode");
    }
  }

  writeRTCMemory(rtcMem);

  for (const auto& stateHandler : windowStateHandler_) {
    stateHandler(isOpen);
  }

  rtcMem.isWindowOpen = isOpen;
}

void open_heat::heating::RadiatorValve::registerWindowChangeHandler(
  const std::function<void(bool)>& handler)
{
  windowStateHandler_.push_back(handler);
}
