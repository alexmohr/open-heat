//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#include "Battery.hpp"
#include <Esp.h>
#include <HardwareSerial.h>

namespace open_heat {
namespace sensors {

void Battery::setup()
{
}

void Battery::loop()
{
  const auto iterations = 100;
  unsigned long sum = 0;
  // more samples for better accuracy
  for (int i = 0; i < iterations; i++) {
    sum += (analogRead(A0));
    delayMicroseconds(250);
  }

  // todo add configuration for these?
  double R1 = 10'000'000.0; // resistance of R1 (10M)
  double R2 = 3'300'000.0; // resistance of R2 (3.3M)

  m_voltage = sum / static_cast<double>(iterations) / 1000.0;
  m_voltage = (m_voltage * (R1 + R2)) / R2;
}

float Battery::percentage()
{
  const float maxBattery = 4.2; // maximum voltage of battery
  const float minBattery = 3.1; // minimum voltage of battery before shutdown

  const auto percent = ((m_voltage - minBattery) / (maxBattery - minBattery)) * 100;
  if (percent < 0.0) {
    return 0.0;
  }

  if (percent < 100.0) {
    return percent;
  }

  return 100.0f;
}

float Battery::voltage()
{
  return m_voltage;
}
} // namespace sensors
} // namespace open_heat
