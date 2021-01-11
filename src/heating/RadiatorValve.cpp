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

  unsigned short rotateTime = 1000;
  const float lastFactor = 1.0f;
  const float maxChangePerLoop = 0.25;
  float diff = 0;

  // diffToLast > 0: warmer than in last loop
  // diffToLast < 0: colder than in last loop
  float diffToLast = temp - lastTemp_;
  if (temp > maxTemp) {
    diff = maxTemp - temp;

    // temp still rising?
    if (temp > lastTemp_) {
      diff += lastFactor * lastDiff_;
      Logger::log(Logger::DEBUG, "temp still rising, adding correction");
    }

    if (diffToLast < maxChangePerLoop) {
      rotateTime = static_cast<unsigned short>((float)rotateTime * std::abs(diff));
      closeValve(rotateTime);
    } else {
      Logger::log(Logger::INFO, "Temp changed enough, do not change valve");
    }

  } else {
    diff = minTemp - temp;

    // temp still sinking?
    if (temp < lastTemp_) {
      Logger::log(Logger::DEBUG, "temp still sinking, adding correction");
      diff += lastFactor * lastDiff_;
    }

    if (diffToLast < 0.1f) {
      rotateTime += 150;
    }

    if (diffToLast < maxChangePerLoop) {
      rotateTime = static_cast<unsigned short>((float)rotateTime * std::abs(diff));
      openValve(rotateTime);
    } else {
      Logger::log(Logger::INFO, "Temp changed enough, do not change valve");
    }
  }

  lastDiff_ = diff;
  lastTemp_ = temp;

  open_heat::Logger::log(
    open_heat::Logger::INFO,
    "Corrected temperature difference  %.2f°C, changed for %ims, temp now %.2f°C, "
    "tempLast %.2f°C, diffToLast: %.2f°C",
    diff,
    rotateTime,
    temp,
    lastTemp_,
    diffToLast);

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
    // update in next loop
    nextCheckMillis_ = 0;
  }

  open_heat::Logger::log(open_heat::Logger::INFO, "New target temperature %f", temp);
  setTemp_ = temp;

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
