//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#include "Logger.hpp"
#include <RTCMemory.hpp>
#include <user_interface.h>

namespace open_heat {
namespace rtc {

bool _lock = false;

void printRTCMemory(const Memory& rtcMemory)
{
  const auto msg = +"valveNextCheckMillis "
    + std::to_string(rtcMemory.valveNextCheckMillis) + "\nmqttNextCheckMillis "
    + std::to_string(rtcMemory.mqttNextCheckMillis) + "\nmillisOffset "
    + std::to_string(rtcMemory.millisOffset) + "\nlastMeasuredTemp "
    + std::to_string(rtcMemory.lastMeasuredTemp) + "\nlastPredictedTemp "
    + std::to_string(rtcMemory.lastPredictedTemp) + "\nsetTemp "
    + std::to_string(rtcMemory.setTemp) + "\ncurrentRotateTime "
    + std::to_string(rtcMemory.currentRotateTime) + "\nturnOff " + "\nlastMode "
    + std::to_string(rtcMemory.lastMode) + "\nisWindowOpen "
    + std::to_string(rtcMemory.isWindowOpen) + "\nrestoreMode "
    + std::to_string(rtcMemory.restoreMode) + "\ndrdDisabled "
    + std::to_string(rtcMemory.drdDisabled);

  Logger::log(Logger::DEBUG, "Rtc memory data: %s", msg.c_str());
}

void writeRTCMemory(const Memory& rtcMemory)
{
  // printRTCMemory(rtcMemory);
  if (!ESP.rtcUserMemoryWrite(0, (uint32_t*)&rtcMemory, sizeof(Memory))) {
    Logger::log(Logger::ERROR, "Failed to write RTC user memory");
  }
}

void lockMem()
{
  while (_lock) {
    delay(10);
  }
  _lock = true;
}

void unlockMem()
{
  _lock = false;
}

Memory readWithoutLock()
{
  Memory rtcMemory{};

  if (!ESP.rtcUserMemoryRead(0, (uint32_t*)&rtcMemory, sizeof(Memory))) {
    Logger::log(Logger::ERROR, "Failed to read RTC user memory");
  }

  return rtcMemory;
}

Memory read()
{
  lockMem();
  const auto mem = readWithoutLock();
  unlockMem();

  return mem;
}

void init(Filesystem& filesystem)
{
  Memory rtcMem{};
  const auto& config = filesystem.getConfig();
  rtcMem.setTemp = config.SetTemperature;
  rtcMem.mode = config.Mode;

  writeRTCMemory(rtcMem);
}

void updateMemory(std::function<void(Memory& mem)> setField)
{
  lockMem();
  auto mem = readWithoutLock();
  setField(mem);
  writeRTCMemory(mem);
  unlockMem();
}

void setLastResetTime(uint64_t val)
{
  updateMemory([&val](Memory& mem) { mem.lastResetTime = val; });
}
void setValveNextCheckMillis(uint64_t val)
{
  updateMemory([&val](Memory& mem) { mem.valveNextCheckMillis = val; });
}
void setMqttNextCheckMillis(uint64_t val)
{
  updateMemory([&val](Memory& mem) { mem.mqttNextCheckMillis = val; });
}
void setMillisOffset(uint64_t val)
{
  updateMemory([&val](Memory& mem) { mem.millisOffset = val; });
}
void setLastMeasuredTemp(float val)
{
  updateMemory([&val](Memory& mem) { mem.lastMeasuredTemp = val; });
}
void setLastPredictedTemp(float val)
{
  updateMemory([&val](Memory& mem) { mem.lastPredictedTemp = val; });
}
void setSetTemp(float val)
{
  updateMemory([&val](Memory& mem) { mem.setTemp = val; });
}
void setCurrentRotateTime(int val, const int absoluteLimit)
{
  updateMemory([&val, absoluteLimit](Memory& mem) {
    if (val < -absoluteLimit) {
      val = -absoluteLimit;
    } else if (val > absoluteLimit) {
      val = absoluteLimit;
    }

    mem.currentRotateTime = val;
  });
}
void setMode(OperationMode val)
{
  updateMemory([&val](Memory& mem) { mem.mode = val; });
}
void setLastMode(OperationMode val)
{
  updateMemory([&val](Memory& mem) { mem.lastMode = val; });
}
void setIsWindowOpen(bool val)
{
  updateMemory([&val](Memory& mem) { mem.isWindowOpen = val; });
}
void setRestoreMode(bool val)
{
  updateMemory([&val](Memory& mem) { mem.restoreMode = val; });
}
void setDrdDisabled(bool val)
{
  updateMemory([&val](Memory& mem) { mem.drdDisabled = val; });
}
void setDebug(bool val)
{
  updateMemory([&val](Memory& mem) { mem.debug = val; });
}
void setModemSleepTime(unsigned long val)
{
  updateMemory([&val](Memory& mem) { mem.modemSleepTime = val; });
}

uint64_t offsetMillis()
{
  const auto mem = readWithoutLock();
  const auto ms = millis() + mem.millisOffset;
  return ms;
}

void wifiDeepSleep(uint64_t timeInMs, bool enableRF, Filesystem& filesystem)
{
  const auto& config = filesystem.getConfig();
  digitalWrite(static_cast<uint8_t>(config.TempVin), LOW);
  digitalWrite(static_cast<uint8_t>(config.MotorPins.Vin), LOW);
  digitalWrite(static_cast<uint8_t>(config.MotorPins.Ground), LOW);

  pinMode(static_cast<uint8_t>(config.TempVin), INPUT);
  pinMode(static_cast<uint8_t>(config.MotorPins.Vin), INPUT);
  pinMode(static_cast<uint8_t>(config.MotorPins.Ground), INPUT);
  pinMode(static_cast<uint8_t>(LED_BUILTIN), INPUT);

  open_heat::Logger::log(open_heat::Logger::INFO, "Sleeping for %lu ms", timeInMs);
  setMillisOffset(offsetMillis() + timeInMs);

  ESP.deepSleepInstant(timeInMs * 1000, enableRF ? RF_CAL : RF_DISABLED);
  delay(1);
}

} // namespace rtc
} // namespace open_heat