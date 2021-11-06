//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#ifndef OPEN_HEAT_BATTERY_H
#define OPEN_HEAT_BATTERY_H
namespace open_heat::sensors {
class Battery {
  public:
  void setup();
  void loop();
  float percentage();
  float voltage();
  private:
      double m_voltage;
};
} // namespace open_heat::sensors

#endif // OPEN_HEAT_BATTERY_H
