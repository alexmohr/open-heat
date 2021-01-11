//
// Copyright (c) 2020 Alexander Mohr
// Licensed under the terms of the MIT license
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

void open_heat::heating::RadiatorValve::loop()
{
  if (setTemp_ == 0) {
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
  const auto minTemp = setTemp_ - tempMinDiff_;
  const auto maxTemp = setTemp_ + tempMaxDiff_;

  open_heat::Logger::log(
    open_heat::Logger::DEBUG,
    "Checking temp is: %.2f°C, set %.2f°C, min %.2f°C, max %.2f°C",
    temp,
    setTemp_,
    minTemp,
    maxTemp);

  if (temp >= minTemp && temp <= maxTemp) {
    open_heat::Logger::log(
      open_heat::Logger::DEBUG, "Measured temp %.2f°C is in range.", temp);
    nextCheckMillis_ = millis() + checkIntervalMillis_;
    return;
  }

  const short rotateTimePerDegree = 2200;

  const float temperatureChange = temp - lastTemp_;
  const float minTemperatureChange = 0.1;
  const float requiredChange = setTemp_ - temp;

  const bool tempRising = (temp > lastTemp_) && std::abs(temperatureChange) > minTemperatureChange;
  const bool tempSinking = (temp < lastTemp_) && std::abs(temperatureChange) > minTemperatureChange;
  const bool tempTooHigh = temp > maxTemp;
  const bool tempTooLow = temp < minTemp;

  double rotateTime = 0;
  if (tempSinking) {
    if (tempTooLow) {
      rotateTime = requiredChange * rotateTimePerDegree;
    } else {
      // temp is sinking as it should
    }
  } else if (tempRising) {
    if (tempTooHigh) {
      rotateTime = -(requiredChange * rotateTimePerDegree);
    } else {
      // temp is rising as it should
    }
  } else {
    // no change.
    if (tempTooLow) {
      rotateTime = 100;
    } if (tempTooHigh){
      rotateTime = -100;
    }
  }

  Logger::log(Logger::INFO, "Rotate Time: %f, change %f", rotateTime,
              temperatureChange);
  rotateValve(static_cast<short>(rotateTime));

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

void open_heat::heating::RadiatorValve::rotateValve(short time)
{
  // negative time is close
  if (time < 0) {
    closeValve(std::abs(time));
  } else {
    openValve(time);
  }
}
