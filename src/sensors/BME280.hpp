//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#ifndef BME280_HPP_
#define BME280_HPP_
#include "BMBase.hpp"
#include "Humidity.hpp"
#include "Temperature.hpp"
#include <Adafruit_BME280.h>

namespace open_heat::sensors {
class BME280 : public BMBase, public Temperature, public Humidity {
  public:
  BME280() = default;
  BME280(const BME280&) = delete;

  bool setup() override;

  float temperature() override;
  float humidity() override;

  protected:
  void sleep() override;
  void wake() override;

  private:
  Adafruit_BME280 m_bme;
};
} // namespace open_heat
#endif // BME280_HPP_
