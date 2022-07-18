//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#ifndef OPEN_HEAT_SENSOR_HPP
#define OPEN_HEAT_SENSOR_HPP
namespace open_heat::sensors {

class Sensor {
  public:
  virtual bool setup() = 0;
};

} // namespace open_heat::sensors
#endif // OPEN_HEAT_SENSOR_HPP
