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
  static const char* modeToCharArray(Mode mode);

  private:
  static void openValve(unsigned short rotateTime);
  static void closeValve(unsigned short rotateTime);
  static void setPinsLow();
  void updateConfig();

  Filesystem& filesystem_;
  Mode mode_{OFF};

  static constexpr int VALVE_COMPLETE_CLOSE_MILLIS = 5000;
  sensors::ITemperatureSensor& tempSensor_;
  float setTemp_;
  float lastTemp_{0};
  float lastPredictTemp_{0};

  bool turnOff_{false};

  unsigned long nextCheckMillis_{0};
  static constexpr unsigned long checkIntervalMillis_ = 2 * 60 * 1000;

  /**
    When deciding about valve movements, the regulation algorithm tries to
    predict the future by extrapolating the last temperature change. This
    value says how far to extrapolate. Larger values make regulation more
    aggressive, smaller values make it less aggressive.
    Unit:  1
    Range: 1, 2, 4, 8 or 16
  */
  static constexpr uint8 PREDICTION_STEEPNESS = 4;
};
} // namespace heating
} // namespace open_heat

#endif // OPEN_HEAT_RADIATORVALVE_HPP
