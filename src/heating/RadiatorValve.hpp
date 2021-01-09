//
// Copyright (c) 2020 Alexander Mohr
// Licensed under the terms of the MIT license
//

#ifndef OPEN_HEAT_RADIATORVALVE_HPP
#define OPEN_HEAT_RADIATORVALVE_HPP

#include <sensors/ITemperatureSensor.hpp>
#include <chrono>
namespace open_heat { namespace heating {
class RadiatorValve {
  public:
  explicit RadiatorValve(sensors::ITemperatureSensor& tempSensor) :
      tempSensor_(tempSensor), setTemp_(20)
  {
    setPinsLow();
  }

  void loop();
  void setup();
  float getConfiguredTemp() const;
  void setConfiguredTemp(float temp);

  private:
  static void openValve(unsigned short rotateTime);
  static void closeValve(unsigned  short rotateTime);
  static void setPinsLow();


  static constexpr int VALVE_COMPLETE_CLOSE_MILLIS = 2500;
  sensors::ITemperatureSensor& tempSensor_;
  float setTemp_;
  float lastDiff_ = 0;
  static constexpr float tempMaxDiff_ = 0.25;
  bool turnOff_ = false;

  unsigned long nextCheckMillis_;
  unsigned long checkIntervalMillis_ = 1 * 60 * 1000;

};
} } // namespace open_heat::heating

#endif // OPEN_HEAT_RADIATORVALVE_HPP
