//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#ifndef OPEN_HEAT_WINDOWSENSOR_HPP
#define OPEN_HEAT_WINDOWSENSOR_HPP

#include <Filesystem.hpp>
#include <heating/RadiatorValve.hpp>
#include <yal/yal.hpp>
namespace open_heat::sensors {

class WindowSensor {
  public:
  WindowSensor(Filesystem* filesystem, heating::RadiatorValve*& valve);

  void setup();
  void loop();

  private:
  static void sensorChangedInterrupt();

  static Filesystem* m_filesystem;
  static heating::RadiatorValve* m_valve;

  static const unsigned int s_minMillisBetweenEvents_{250};
  static unsigned long m_lastChangeMillis;

  bool m_isSetUp{false};
  static bool m_isOpen;
  static bool m_validate;

  const inline static yal::Logger m_logger = yal::Logger("WIDNOW");
};

} // namespace open_heat::sensors
#endif // OPEN_HEAT_WINDOWSENSOR_HPP
