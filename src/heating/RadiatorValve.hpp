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

  RadiatorValve(sensors::ITemperatureSensor& tempSensor, Filesystem& filesystem);

  void loop();
  void setup();
  float getConfiguredTemp() const;
  void setConfiguredTemp(float temp);
  void setMode(OperationMode mode);
  OperationMode getMode();
  static const char* modeToCharArray(OperationMode mode);

  void registerSetTempChangedHandler(const std::function<void(float)>& handler);
  void registerModeChangedHandler(const std::function<void(OperationMode)>& handler);

  void openFully();

  private:
  void openValve(unsigned short rotateTime);
  void closeValve(unsigned short rotateTime);
  void setPinsLow();
  void updateConfig();

  Filesystem& filesystem_;
  OperationMode mode_{OFF};

  static constexpr int VALVE_FULL_ROTATE_TIME = 10000;
  sensors::ITemperatureSensor& tempSensor_;
  float setTemp_;
  float lastTemp_{0};
  float lastPredictTemp_{0};

  bool turnOff_{false};

  unsigned long nextCheckMillis_{0};
  static constexpr unsigned long checkIntervalMillis_ = 2 * 60 * 1000;
  static constexpr  uint8_t maxRotateNoChange_{8};
  uint8_t  currentRotateNoChange_{0};

  std::vector<std::function<void(OperationMode)>> opModeChangedHandler_{};
  std::vector<std::function<void(float)>> setTempChangedHandler_{};

  /**
    When deciding about valve movements, the regulation algorithm tries to
    predict the future by extrapolating the last temperature change. This
    value says how far to extrapolate. Larger values make regulation more
    aggressive, smaller values make it less aggressive.
    Unit:  1
  */
  static constexpr float PREDICTION_STEEPNESS = 2;
};
} // namespace heating
} // namespace open_heat

#endif // OPEN_HEAT_RADIATORVALVE_HPP
