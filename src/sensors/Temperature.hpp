//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#ifndef TEMPERATUR_HPP_
#define TEMPERATUR_HPP_

namespace open_heat {
namespace sensors {

class Temperature {
  public:
  virtual float temperature() = 0;
};

} // namespace sensors
} // namespace open_heat

#endif // TEMPERATUR_HPP_
