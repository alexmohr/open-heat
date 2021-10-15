//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#ifndef BME280_HPP_
#define BME280_HPP_
#include "ITemperatureSensor.hpp"
#include <Adafruit_BME280.h>

namespace open_heat {
namespace sensors {
class BME280 : public ITemperatureSensor {
  public:
  BME280() = default;

  public: // ITemperatureSensor
  float getTemperature() override;
  float getHumidity() override;
  void setup() override;
  void loop() override;
  void sleep() override;
  void wake() override;

  private:
  // use I2C interface
  Adafruit_BME280 bme_;

  /*  Adafruit_Sensor* bme_pressure = bme.getPressureSensor();
    Adafruit_Sensor* bme_humidity = bme.getHumiditySensor();
  */
};
} // namespace sensors
} // namespace open_heat
#endif // BME280_HPP_
