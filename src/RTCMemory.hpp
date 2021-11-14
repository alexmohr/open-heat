//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#include "Config.hpp"
#include "Filesystem.hpp"

#include <cstdint>

#ifndef OPEN_HEAT_RTCMEMORY_H
#define OPEN_HEAT_RTCMEMORY_H
namespace open_heat {
namespace rtc {

struct Memory {
  // todo Memory(const Memory&) = delete;

  uint64_t valveNextCheckMillis = 0;
  uint64_t mqttNextCheckMillis = 0;
  uint64_t millisOffset = 0;
  uint64_t lastResetTime = 0;

  float lastMeasuredTemp = 0;
  float lastPredictedTemp = 0;
  float setTemp = 0;
  int currentRotateTime = 0;
  bool turnOff = false;
  bool openFully = false;
  bool debug = false;

  OperationMode mode = UNKNOWN;
  OperationMode lastMode = mode;

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
void setTurnOff(bool val);
void setOpenFully(bool val);
void setMode(OperationMode val);
void setLastMode(OperationMode val);
void setIsWindowOpen(bool val);
void setRestoreMode(bool val);
void setDrdDisabled(bool val);
void setDebug(bool val);
void setLastResetTime(uint64_t val);
void setModemSleepTime(unsigned long val);

Memory read();
void init(Filesystem& filesystem);

uint64_t offsetMillis();
void wifiDeepSleep(uint64_t timeInMs, bool enableRF, Filesystem& filesystem);

} // namespace rtc
} // namespace open_heat

#endif // OPEN_HEAT_RTCMEMORY_H
