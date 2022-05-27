//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#ifndef OPEN_HEAT_BMP280_H
#define OPEN_HEAT_BMP280_H

#include "BMBase.hpp"
#include "Temperature.hpp"
#include <Adafruit_BMP280.h>

namespace open_heat::sensors {

class BMP280 : public BMBase, public Temperature {
  public:
  BMP280() = default;
  BMP280(const BMP280&) = delete;

  bool setup() override;

  float temperature() override;

  protected:
  void sleep() override;
  void wake() override;

  Adafruit_BMP280 m_bmp;
};
} // namespace open_heat::sensors
#endif // OPEN_HEAT_BMP280_H
