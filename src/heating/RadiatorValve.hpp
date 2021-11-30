//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#ifndef OPEN_HEAT_RADIATORVALVE_HPP
#define OPEN_HEAT_RADIATORVALVE_HPP

#include <Filesystem.hpp>
#include <RTCMemory.hpp>
#include <sensors/Temperature.hpp>
#include <chrono>
namespace open_heat {
namespace heating {

class RadiatorValve {
  public:
  RadiatorValve(sensors::Temperature*& tempSensor, Filesystem& filesystem);
  RadiatorValve(const RadiatorValve&) = delete;

  uint64_t loop();
  void setup();
  static float getConfiguredTemp();
  void setConfiguredTemp(float temp);
  void setMode(OperationMode mode);
  OperationMode getMode();
  static const char* modeToCharArray(OperationMode mode);

  void registerSetTempChangedHandler(const std::function<void(float)>& handler);
  void registerModeChangedHandler(const std::function<void(OperationMode)>& handler);
  void registerWindowChangeHandler(const std::function<void(bool)>& handler);

  void setWindowState(bool isOpen);

  private:
  void openValve(unsigned int rotateTime);
  void closeValve(unsigned int rotateTime);
  void disablePins();
  void enablePins();
  void updateConfig();

  void setNextCheckTimeNow();

  static constexpr int VALVE_FULL_ROTATE_TIME = 40'000;
  static constexpr int SLEEP_MILLIS_AFTER_WINDOW_CLOSE = 3 * 60 * 1000;

  /**
    When deciding about valve movements, the regulation algorithm tries to
    predict the future by extrapolating the last temperature change. This
    value says how far to extrapolate. Larger values make regulation more
    aggressive, smaller values make it less aggressive.
    Unit:  1
  */
  static constexpr float PREDICTION_STEEPNESS = 2;

  Filesystem& m_filesystem;

  sensors::Temperature*& m_temperatureSensor;

  static constexpr unsigned long m_checkIntervalMillis
    = static_cast<unsigned long>(5 * 60 * 1000);

  static constexpr int m_spinUpMillis = 50;
  static constexpr int m_finalRotateMillis = 1'000;

  std::vector<std::function<void(OperationMode)>> m_OpModeChangeHandler{};
  std::vector<std::function<void(bool)>> m_windowStateHandler{};
  std::vector<std::function<void(float)>> m_setTempChangeHandler{};
  [[nodiscard]] static unsigned int remainingRotateTime(int rotateTime, bool close);
  void rotateValve(
    unsigned int rotateTime,
    const PinSettings& config,
    int vinState,
    int groundState);
  static uint64_t nextCheckTime();
  void handleTempTooLow(
    const open_heat::rtc::Memory& rtcData,
    const float measuredTemp,
    const float predictTemp,
    const float openHysteresis);
  void handleTempTooHigh(
    const open_heat::rtc::Memory& rtcData,
    const float predictTemp,
    const float closeHysteresis);
};
} // namespace heating
} // namespace open_heat

#endif // OPEN_HEAT_RADIATORVALVE_HPP
