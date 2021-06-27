//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#include "Logger.hpp"
#include <RTCMemory.hpp>
#include <user_interface.h>

namespace open_heat {


//prints all rtcData, including the leading crc32
void printMemory(const RTCMemory& rtcMemory) {
  char buf[3];
  uint8_t *ptr = (uint8_t *)&rtcMemory;
  for (size_t i = 0; i < sizeof(RTCMemory); i++) {
    sprintf(buf, "%02X", ptr[i]);
    Serial.print(buf);
    if ((i + 1) % 32 == 0) {
      Serial.println();
    } else {
      Serial.print(" ");
    }
  }
  Serial.println();
}

RTCMemory readRTCMemory()
{
  RTCMemory rtcMemory{};

  if (!ESP.rtcUserMemoryRead(0, (uint32_t *)&rtcMemory, sizeof(RTCMemory))) {
    Logger::log(Logger::ERROR, "Failed to read RTC user memory");
  }

  return rtcMemory;
}

void writeRTCMemory(const RTCMemory& rtcMemory)
{
  if (!ESP.rtcUserMemoryWrite(0, (uint32_t *)&rtcMemory, sizeof(RTCMemory))) {
    Logger::log(Logger::ERROR, "Failed to write RTC user memory");
  }
}

uint64_t offsetMillis()
{
  const auto mem = readRTCMemory();
  const auto ms = millis() + mem.millisOffset;
  return ms;
}

} // namespace open_heat