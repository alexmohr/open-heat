//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//


#ifndef OPEN_HEAT_WINDOWSENSOR_HPP
#define OPEN_HEAT_WINDOWSENSOR_HPP

class WindowSensor {
  public:
      unsigned long getLastChangeMillis();
      bool isOpen();

  private:
      unsigned long lastChangeMillis_;
};

#endif // OPEN_HEAT_WINDOWSENSOR_HPP
