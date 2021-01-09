//
// Copyright (c) 2020 Alexander Mohr
// Licensed under the terms of the MIT license
//

#include "RadiatorValve.hpp"
#include <Arduino.h>
#include <Logger.hpp>
void open_heat::heating::RadiatorValve::loop()
{
  if (setTemp_ == 0) {
    if (turnOff_){
      closeValve(VALVE_COMPLETE_CLOSE_MILLIS);
      turnOff_ = false;
    }
    
    return;
  }


  if (millis() < nextCheckMillis_) {
    return;
  }

  auto temp = tempSensor_.getTemperature();
  auto minTemp = setTemp_ - tempMaxDiff_;
  auto maxTemp = setTemp_ + tempMaxDiff_;

  open_heat::Logger::log(
    open_heat::Logger::DEBUG,
    "Checking temp is: %f, set %f, min %f°C, max %f°C",
    temp,
    setTemp_,
    minTemp,
    maxTemp);

  if (temp >= minTemp && temp <= maxTemp) {
    open_heat::Logger::log(
      open_heat::Logger::DEBUG, "Measured temp %f°C is in range.", temp);
    nextCheckMillis_ = millis() + checkIntervalMillis_;
    return;
  }

  unsigned short rotateTime = 500;
  const float lastFactor = 0.75f;
  float diff;
  if (temp > maxTemp) {
    diff = maxTemp - temp;

    // temp still rising?
    if (diff > lastDiff_) {
      diff += lastFactor * lastDiff_;
      Logger::log(Logger::DEBUG, "temp still rising, adding correction");
    }

    rotateTime = static_cast<unsigned short>((float)rotateTime * std::abs(diff));
    closeValve(rotateTime);
  } else {
    diff = minTemp - temp;

    // temp still sinking?
    if (diff < lastDiff_) {
      Logger::log(Logger::DEBUG, "temp still sinking, adding correction");
      diff += lastFactor * lastDiff_;
    }


    rotateTime = static_cast<unsigned short>((float)rotateTime * std::abs(diff));
    openValve(rotateTime);
  }

  lastDiff_ = diff;

  open_heat::Logger::log(
    open_heat::Logger::DEBUG,
    "Corrected temperature difference %f, rotated valve for %ims",
    diff,
    rotateTime);

  nextCheckMillis_ = millis() + checkIntervalMillis_;
}

void open_heat::heating::RadiatorValve::setup()
{
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
