//
// Copyright (c) 2020 Alexander Mohr
// Licensed under the terms of the MIT license
//

#ifndef OPEN_HEAT_TP100_HPP
#define OPEN_HEAT_TP100_HPP

#include "ITemperatureSensor.hpp"

namespace open_heat {
namespace sensors {

class TP100 : public ITemperatureSensor {
  public: // ITemperatureSensor
  float getTemperature() override;
  void setup() override;
  void loop() override;
};

} // namespace sensors
} // namespace open_heat

#endif // OPEN_HEAT_TP100_HPP
