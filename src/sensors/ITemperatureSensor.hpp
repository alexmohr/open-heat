//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//


#ifndef ITEMPERATURESENSOR_HPP_
#define ITEMPERATURESENSOR_HPP_


enum TEMP_SENSORS {
  BME280 = 0,
  TP100
};


namespace open_heat {
namespace sensors {

class ITemperatureSensor {
 public:
  virtual float getTemperature() = 0;
  virtual void setup() = 0;
  virtual void loop() = 0;
};

}
}

#endif //ITEMPERATURESENSOR_HPP_
