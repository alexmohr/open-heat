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
  const auto msg = " canary " + std::to_string(rtcMemory.canary)
    + "\nvalveNextCheckMillis " + std::to_string(rtcMemory.valveNextCheckMillis)
    + "\nmqttNextCheckMillis " + std::to_string(rtcMemory.mqttNextCheckMillis)
    + "\nmillisOffset " + std::to_string(rtcMemory.millisOffset) + "\nlastMeasuredTemp "
    + std::to_string(rtcMemory.lastMeasuredTemp) + "\nlastPredictedTemp "
    + std::to_string(rtcMemory.lastPredictedTemp) + "\nsetTemp "
    + std::to_string(rtcMemory.setTemp) + "\ncurrentRotateTime "
    + std::to_string(rtcMemory.currentRotateTime) + "\nturnOff "
    + std::to_string(rtcMemory.turnOff) + "\nopenFully "
    + std::to_string(rtcMemory.openFully) + "\nmode " + std::to_string(rtcMemory.mode)
    + "\nlastMode " + std::to_string(rtcMemory.lastMode) + "\nisWindowOpen "
    + std::to_string(rtcMemory.isWindowOpen) + "\nrestoreMode "
    + std::to_string(rtcMemory.restoreMode) + "\ndrdDisabled "
    + std::to_string(rtcMemory.drdDisabled);

  Logger::log(Logger::DEBUG, "Rtc memory data: %s", msg.c_str());
}

void writeRTCMemory(const Memory& rtcMemory)
{
  Logger::log(Logger::DEBUG, "Updating RTCMemory");
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
  rtcMem.canary = CANARY;

  writeRTCMemory(rtcMem);
}
void setLastResetTime(uint64_t val)
{
  lockMem();
  auto mem = readWithoutLock();
  mem.lastResetTime = val;
  writeRTCMemory(mem);
  unlockMem();
}
void setValveNextCheckMillis(uint64_t val)
{
  lockMem();
  auto mem = readWithoutLock();
  mem.valveNextCheckMillis = val;
  writeRTCMemory(mem);
  unlockMem();
}
void setMqttNextCheckMillis(uint64_t val)
{
  lockMem();
  auto mem = readWithoutLock();
  mem.mqttNextCheckMillis = val;
  writeRTCMemory(mem);
  unlockMem();
}
void setMillisOffset(uint64_t val)
{
  lockMem();
  auto mem = readWithoutLock();
  mem.millisOffset = val;
  writeRTCMemory(mem);
  unlockMem();
}
void setLastMeasuredTemp(float val)
{
  lockMem();
  auto mem = readWithoutLock();
  mem.lastMeasuredTemp = val;
  writeRTCMemory(mem);
  unlockMem();
}
void setLastPredictedTemp(float val)
{
  lockMem();
  auto mem = readWithoutLock();
  mem.lastPredictedTemp = val;
  writeRTCMemory(mem);
  unlockMem();
}
void setSetTemp(float val)
{
  lockMem();
  auto mem = readWithoutLock();
  mem.setTemp = val;
  writeRTCMemory(mem);
  unlockMem();
}
void setCurrentRotateTime(int val, const int absoluteLimit)
{
  lockMem();
  auto mem = readWithoutLock();
  if (val < -absoluteLimit) {
    val = -absoluteLimit;
  } else if (val > absoluteLimit) {
    val = absoluteLimit;
  }

  mem.currentRotateTime = val;
  writeRTCMemory(mem);
  unlockMem();
}
void setTurnOff(bool val)
{
  lockMem();
  auto mem = readWithoutLock();
  mem.turnOff = val;
  writeRTCMemory(mem);
  unlockMem();
}
void setOpenFully(bool val)
{
  lockMem();
  auto mem = readWithoutLock();
  mem.openFully = val;
  writeRTCMemory(mem);
  unlockMem();
}
void setMode(OperationMode val)
{
  lockMem();
  auto mem = readWithoutLock();
  mem.mode = val;
  writeRTCMemory(mem);
  unlockMem();
}
void setLastMode(OperationMode val)
{
  lockMem();
  auto mem = readWithoutLock();
  mem.lastMode = val;
  writeRTCMemory(mem);
  unlockMem();
}
void setIsWindowOpen(bool val)
{
  lockMem();
  auto mem = readWithoutLock();
  mem.isWindowOpen = val;
  writeRTCMemory(mem);
  unlockMem();
}
void setRestoreMode(bool val)
{
  lockMem();
  auto mem = readWithoutLock();
  mem.restoreMode = val;
  writeRTCMemory(mem);
  unlockMem();
}
void setDrdDisabled(bool val)
{
  lockMem();
  auto mem = readWithoutLock();
  mem.drdDisabled = val;
  writeRTCMemory(mem);
  unlockMem();
}
void setDebug(bool val)
{
  lockMem();
  auto mem = readWithoutLock();
  mem.debug = val;
  writeRTCMemory(mem);
  unlockMem();
}
void setModemSleepTime(unsigned long time)
{
  lockMem();
  auto mem = readWithoutLock();
  mem.modemSleepTime = time;
  writeRTCMemory(mem);
  unlockMem();
}



uint64_t offsetMillis()
{
  const auto mem = read();
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

  ESP.deepSleep(timeInMs * 1000, enableRF ? RF_CAL : RF_DISABLED);
  delay(1);
}

} // namespace rtc
} // namespace open_heat