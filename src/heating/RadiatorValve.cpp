//
// Copyright (c) 2020 Alexander Mohr
// Licensed under the terms of the MIT license
//

#include "RadiatorValve.hpp"
#include <Arduino.h>
#include <Logger.hpp>

/** \def PREDICTION_STEEPNESS
  When deciding about valve movements, the regulation algorithm tries to
  predict the future by extrapolating the last temperature change. This
  value says how far to extrapolate. Larger values make regulation more
  aggressive, smaller values make it less aggressive.
  Unit:  1
  Range: 1, 2, 4, 8 or 16 (must be exponent 2 to keep the binary small)
*/
#define PREDICTION_STEEPNESS 4

open_heat::heating::RadiatorValve::RadiatorValve(
  open_heat::sensors::ITemperatureSensor& tempSensor,
  open_heat::Filesystem& filesystem) :
    filesystem_(filesystem), tempSensor_(tempSensor), setTemp_(20)
{
  setPinsLow();
}

void open_heat::heating::RadiatorValve::loop()
{
  if (mode_ != HEAT) {
    if (turnOff_) {
      closeValve(VALVE_COMPLETE_CLOSE_MILLIS);
      turnOff_ = false;
    }
    return;
  }

  if (millis() < nextCheckMillis_) {
    return;
  }

  const auto temp = tempSensor_.getTemperature();
  const auto hysteresis = 0.05;
  const short openTime = 400;
  const short closeTIme = openTime / 2;

  // https://github.com/Traumflug/ISTAtrol/blob/master/firmware/main.c
  float predictionError = temp - lastPredictTemp_;
  Logger::log(
    Logger::DEBUG,
    "Predicted temperature %.2f, actual %.2f, error %.2f",
    lastPredictTemp_,
    temp,
    predictionError);

  float predictTemp = temp + PREDICTION_STEEPNESS * ((int16_t)temp - (int16_t)lastTemp_);
  Logger::log(
    Logger::DEBUG, "Predicted temp %.2f in %lu ms", predictTemp, checkIntervalMillis_);

  // Act according to the prediction.
  if (predictTemp < (setTemp_ - hysteresis)) {
    openValve(openTime);
  } else if (predictTemp > (setTemp_ + hysteresis)) {
    closeValve(closeTIme);
  } else {
    Logger::log(Logger::DEBUG, "Valve not changed");
  }

  lastPredictTemp_ = predictTemp;
  lastTemp_ = temp;

  nextCheckMillis_ = millis() + checkIntervalMillis_;
}

void open_heat::heating::RadiatorValve::setup()
{
  const auto& config = filesystem_.getConfig();
  setTemp_ = config.SetTemperature;
  lastTemp_ = tempSensor_.getTemperature();

  closeValve(VALVE_COMPLETE_CLOSE_MILLIS);
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
}

void open_heat::heating::RadiatorValve::updateConfig()
{
  auto& config = this->filesystem_.getConfig();
  config.SetTemperature = this->setTemp_;
  this->filesystem_.persistConfig();
}

void open_heat::heating::RadiatorValve::closeValve(unsigned short rotateTime)
{
  open_heat::Logger::log(open_heat::Logger::DEBUG, "Closing valve for %ims", rotateTime);
  digitalWrite(MOTOR_VIN, LOW);
  digitalWrite(MOTOR_GROUND, HIGH);

  delay(rotateTime);

  setPinsLow();
}

void open_heat::heating::RadiatorValve::openValve(unsigned short rotateTime)
{
  open_heat::Logger::log(open_heat::Logger::DEBUG, "Opening valve for %ims", rotateTime);
  digitalWrite(MOTOR_GROUND, LOW);
  digitalWrite(MOTOR_VIN, HIGH);

  delay(rotateTime);

  setPinsLow();
}

void open_heat::heating::RadiatorValve::setPinsLow()
{
  digitalWrite(MOTOR_GROUND, LOW);
  digitalWrite(MOTOR_VIN, LOW);
}

void open_heat::heating::RadiatorValve::setMode(
  open_heat::heating::RadiatorValve::Mode mode)
{
  mode_ = mode;
  if (mode_ != HEAT) {
    turnOff_ = true;
  }
}
open_heat::heating::RadiatorValve::Mode open_heat::heating::RadiatorValve::getMode()
{
  return mode_;
}

const char* open_heat::heating::RadiatorValve::modeToCharArray(
  const open_heat::heating::RadiatorValve::Mode mode)
{
  if (mode == heating::RadiatorValve::HEAT) {
    return  "heat";
  } else if (mode == heating::RadiatorValve::OFF) {
    return "off";
  } else {
    return "unknown";
  }
}