//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#include "Config.hpp"
#include <esp-gui/Configuration.hpp>

#include <cstdint>

#ifndef OPEN_HEAT_RTCMEMORY_H
#define OPEN_HEAT_RTCMEMORY_H
namespace open_heat {
namespace rtc {

struct Memory {
  uint64_t valveNextCheckMillis = 0;
  uint64_t mqttNextCheckMillis = 0;
  uint64_t millisOffset = 0;
  uint64_t lastResetTime = 0;

  float lastMeasuredTemp = 0;
  float lastPredictedTemp = 0;
  float setTemp = 0;
  int currentRotateTime = 0;
  bool debug = false;

  config::OperationMode mode = config::OperationMode::UNKNOWN;
  config::OperationMode lastMode = mode;

  bool isWindowOpen = false;
  bool restoreMode = false;

  bool drdDisabled = false;
  unsigned long modemSleepTime = 15 * 60 * 1000;
  ;
};

void setValveNextCheckMillis(uint64_t val);
void setMqttNextCheckMillis(uint64_t val);
void setMillisOffset(uint64_t val);
void setLastMeasuredTemp(float val);
void setLastPredictedTemp(float val);
void setSetTemp(float val);
void setCurrentRotateTime(int val, int absoluteLimit);
void setMode(config::OperationMode val);
void setLastMode(config::OperationMode val);
void setIsWindowOpen(bool val);
void setRestoreMode(bool val);
void setDrdDisabled(bool val);
void setDebug(bool val);
void setLastResetTime(uint64_t val);
void setModemSleepTime(unsigned long val);
Memory read();
void init(esp_gui::Configuration config);

uint64_t offsetMillis();
void wifiDeepSleep(uint64_t timeInMs, bool enableRF, esp_gui::Configuration& filesystem);

} // namespace rtc
} // namespace open_heat

#endif // OPEN_HEAT_RTCMEMORY_H
