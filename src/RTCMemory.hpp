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

static constexpr uint64_t CANARY = 0xDEADBEEFCAFEBABE;

struct Memory {
  uint64_t canary;

  uint64_t valveNextCheckMillis;
  uint64_t mqttNextCheckMillis;
  uint64_t millisOffset;
  uint64_t lastResetTime;

  float lastMeasuredTemp;
  float lastPredictedTemp;
  float setTemp;
  int currentRotateTime;
  bool turnOff;
  bool openFully;
  bool debug;

  OperationMode mode;
  OperationMode lastMode;

  bool isWindowOpen;
  bool restoreMode;

  bool drdDisabled;
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

Memory read();
void init(Filesystem& filesystem);

uint64_t offsetMillis();
void wifiDeepSleep(uint64_t timeInMs, bool enableRF, Filesystem& filesystem);

} // namespace rtc
} // namespace open_heat

#endif // OPEN_HEAT_RTCMEMORY_H
