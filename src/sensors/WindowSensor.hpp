//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#ifndef OPEN_HEAT_WINDOWSENSOR_HPP
#define OPEN_HEAT_WINDOWSENSOR_HPP

#include <Filesystem.hpp>
#include <heating/RadiatorValve.hpp>
namespace open_heat {
namespace sensors {

class WindowSensor {
  public:
  WindowSensor(Filesystem* filesystem, heating::RadiatorValve* valve);

  void setup();
  void loop();


  private:
  static void sensorChangedInterrupt();

  private:
  static Filesystem* filesystem_;
  static heating::RadiatorValve* valve_;

  static const unsigned int minMillisBetweenEvents_{250};
  static unsigned long lastChangeMillis_;

  bool isSetUp{false};
};

} // namespace sensors
} // namespace open_heat
#endif // OPEN_HEAT_WINDOWSENSOR_HPP
