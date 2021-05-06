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

  unsigned long loop();
  void setup();
  float getConfiguredTemp() const;
  void setConfiguredTemp(float temp);
  void setMode(OperationMode mode);
  OperationMode getMode();
  static const char* modeToCharArray(OperationMode mode);

  void registerSetTempChangedHandler(const std::function<void(float)>& handler);
  void registerModeChangedHandler(const std::function<void(OperationMode)>& handler);
  void registerWindowChangeHandler(const std::function<void(bool)>& handler);

  void setWindowState(bool isOpen);

  private:
  void openValve(unsigned short rotateTime);
  void closeValve(unsigned short rotateTime);
  void setPinsLow();
  void updateConfig();

  Filesystem& filesystem_;
  OperationMode mode_{OFF};

  static constexpr int VALVE_FULL_ROTATE_TIME = 20000;
  sensors::ITemperatureSensor& tempSensor_;
  float setTemp_;
  float lastTemp_{0};
  float lastPredictTemp_{0};
  int currentRotateTime_{-VALVE_FULL_ROTATE_TIME / 2};

  bool turnOff_{false};
  bool openFully_{false};

  unsigned long nextCheckMillis_{0};
  static constexpr unsigned long checkIntervalMillis_
    = static_cast<unsigned long>(2.5 * 60 * 1000);

  std::vector<std::function<void(OperationMode)>> opModeChangedHandler_{};
  std::vector<std::function<void(bool)>> windowStateHandler_{};
  std::vector<std::function<void(float)>> setTempChangedHandler_{};

  OperationMode lastMode_;
  bool restoreMode_ = false;
  bool isWindowOpen_ = false;
  // Wait 3 minutes before heating again after window opened
  const unsigned long sleepMillisAfterWindowClose_{3 * 60 * 1000};

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
