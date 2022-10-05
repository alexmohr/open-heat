//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#ifndef OPEN_HEAT_BMBASE_H
#define OPEN_HEAT_BMBASE_H

#include "Sensor.hpp"
#include <yal/yal.hpp>
#include <functional>

namespace open_heat::sensors {

class BMBase : public Sensor {

  protected:
  BMBase() = default;
  BMBase(const BMBase&) = delete;

  bool init(std::function<bool()>&& sensorBegin);

  virtual void wake() = 0;
  virtual void sleep() = 0;

  yal::Logger m_logger = yal::Logger("BMx");

  private:
  bool m_isSetup = false;
};
} // namespace open_heat::sensors

#endif // OPEN_HEAT_BMBASE_H
