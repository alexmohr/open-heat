//
// Copyright (c) 2020 Alexander Mohr
// Open-Heat - Radiator control for ESP8266
// Licensed under the terms of the MIT license
//


#ifndef ITEMPERATURESENSOR_HPP_
#define ITEMPERATURESENSOR_HPP_

namespace open_heat {
namespace sensors {
class ITemperatureSensor {
 public:
  virtual int getTemperature() = 0;
};

}
}

#endif //ITEMPERATURESENSOR_HPP_
