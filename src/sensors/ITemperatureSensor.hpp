//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#ifndef ITEMPERATURESENSOR_HPP_
#define ITEMPERATURESENSOR_HPP_

enum TEMP_SENSORS { BME280 = 0, TP100 };

namespace open_heat {
namespace sensors {

class ITemperatureSensor {
  public:
  virtual float getTemperature() = 0;
  virtual float getHumidity() = 0;
  virtual bool setup() = 0;
  virtual void loop() = 0;
  virtual void sleep() = 0;
  virtual void wake() = 0;
};

} // namespace sensors
} // namespace open_heat

#endif // ITEMPERATURESENSOR_HPP_
