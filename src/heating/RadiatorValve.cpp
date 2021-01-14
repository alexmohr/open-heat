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

  // closeValve(VALVE_COMPLETE_CLOSE_MILLIS);
}


void open_heat::heating::RadiatorValve::loop()
{
  if (turnOff_) {
    closeValve(VALVE_COMPLETE_CLOSE_MILLIS);
    turnOff_ = false;
  }

  if (millis() < nextCheckMillis_) {
    return;
  }

  const auto temp = tempSensor_.getTemperature();
  float predictTemp = temp + PREDICTION_STEEPNESS * ((int16_t)temp - (int16_t)lastTemp_);
  Logger::log(
    Logger::DEBUG, "Predicted temp %.2f in %lu ms", predictTemp, checkIntervalMillis_);

  if (mode_ != HEAT) {
    lastTemp_ = temp;
    nextCheckMillis_ = millis() + checkIntervalMillis_;
    return;
  }



  // 0.5 works; 0.3 both works
  const auto openHysteresis = 0.3f;
  const auto closeHysteresis = 0.3f;
  short openTime = 400;
  const short closeTime = openTime / 2;

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
    return;
  }

  const float temperatureChange = temp - lastTemp_;
  const float minTemperatureChange = 0.06;

  // Act according to the prediction.
  if (predictTemp < (setTemp_ - openHysteresis)) {
    if (temperatureChange < minTemperatureChange && currentRotateNoChange_ < maxRotateNoChange_) {

      const float diff = 1;
      if (setTemp_ - predictTemp > diff && setTemp_ - temp > diff ) {
        openTime *= 3;
        Logger::log(
          Logger::DEBUG, "3x open time due to large temp difference");
      }

      openValve(openTime);
      currentRotateNoChange_++;
      // wait longer after opening the valve,
      // this should prevent over heating but also increases time to heat.
      additionalWaitTime = checkIntervalMillis_;
    } else {
      Logger::log(
        Logger::INFO,
        "RISE, NO ADJUST: Temp old %.2f, temp now %.2f, temp change %.2f",
        lastTemp_,
        temp,
        temperatureChange);
      currentRotateNoChange_  = 0;
    }

  } else if (predictTemp > (setTemp_ + closeHysteresis)) {
    currentRotateNoChange_  = 0;
    if (temperatureChange >= -minTemperatureChange) {
      closeValve(closeTime);
    } else {
      Logger::log(
        Logger::INFO,
        "SINK, NO ADJUST: Temp old %.2f, temp now %.2f, temp change %.2f",
        lastTemp_,
        temp,
        temperatureChange);
    }

  } else {
    if ((temp - lastTemp_) > 0.1 && std::abs(temp - setTemp_) < 0.33) {
      // temp is rising and near limit. close valve a bit to prevent spikes
      closeValve(closeTime);
      Logger::log(Logger::WARNING, "Spike prevention!");
    } else {
      Logger::log(Logger::DEBUG, "Valve not changed");
    }
  }

  lastPredictTemp_ = predictTemp;
  lastTemp_ = temp;

  nextCheckMillis_ = millis() + checkIntervalMillis_ + additionalWaitTime;
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

void open_heat::heating::RadiatorValve::setMode(OperationMode mode)
{
  mode_ = mode;
  if (mode_ != HEAT) {
    turnOff_ = true;
  }
  auto& config = filesystem_.getConfig();
  config.Mode = mode_;
  filesystem_.persistConfig();
}
OperationMode open_heat::heating::RadiatorValve::getMode()
{
  return mode_;
}

const char* open_heat::heating::RadiatorValve::modeToCharArray(
  const OperationMode mode)
{
  if (mode == HEAT) {
    return "heat";
  } else if (mode == OFF) {
    return "off";
  } else {
    return "unknown";
  }
}