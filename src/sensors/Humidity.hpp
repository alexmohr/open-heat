//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#ifndef HUMIDITY_HPP_
#define HUMIDITY_HPP_

namespace open_heat::sensors {

class Humidity {
  public:
  virtual float humidity() = 0;
};

} // namespace open_heat::sensors

#endif // HUMIDITY_HPP_
