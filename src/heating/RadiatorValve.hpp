//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#ifndef OPEN_HEAT_RADIATORVALVE_HPP
#define OPEN_HEAT_RADIATORVALVE_HPP

#include <Filesystem.hpp>
#include <sensors/ITemperatureSensor.hpp>
#include <chrono>
namespace open_heat {
namespace heating {

class RadiatorValve {
  public:
  enum Mode { HEAT, OFF, UNKNOWN };


  RadiatorValve(sensors::ITemperatureSensor& tempSensor, Filesystem& filesystem);

  void loop();
  void setup();
  float getConfiguredTemp() const;
  void setConfiguredTemp(float temp);
  void setMode(Mode mode);
  Mode getMode();
  static const char* modeToCharArray(const Mode mode);

  private:
  static void openValve(unsigned short rotateTime);
  static void closeValve(unsigned short rotateTime);
  static void setPinsLow();
  void updateConfig();

  Filesystem& filesystem_;
  Mode mode_;

  static constexpr int VALVE_COMPLETE_CLOSE_MILLIS = 5000;
  sensors::ITemperatureSensor& tempSensor_;
  float setTemp_;
  float lastTemp_ = 0;
  float lastPredictTemp_;

  static constexpr float tempMaxDiff_ = 0.1;
  static constexpr float tempMinDiff_ = 0.4;
  bool turnOff_ = false;

  unsigned long nextCheckMillis_{0};
  unsigned long checkIntervalMillis_ = 2 * 60 * 1000;
};
} // namespace heating
} // namespace open_heat

#endif // OPEN_HEAT_RADIATORVALVE_HPP
