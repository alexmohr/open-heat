//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#include "Config.hpp"
#include <cstdint>

#ifndef OPEN_HEAT_RTCMEMORY_H
#define OPEN_HEAT_RTCMEMORY_H
namespace open_heat {

static constexpr  uint64_t CANARY = 0xDEADBEEFCAFEBABE;

struct RTCMemory {
  uint64_t canary;

  uint64_t valveNextCheckMillis;
  uint64_t mqttNextCheckMillis;
  uint64_t millisOffset;

  float lastMeasuredTemp;
  float lastPredictedTemp;
  float setTemp;
  int currentRotateTime;
  bool turnOff;
  bool openFully;

  OperationMode mode;
  OperationMode lastMode;

  bool isWindowOpen;
  bool restoreMode;

  bool drdDisabled;



};

RTCMemory readRTCMemory();
void writeRTCMemory(const RTCMemory& rtcMemory);
uint64_t offsetMillis();

} // namespace open_heat



#endif // OPEN_HEAT_RTCMEMORY_H
