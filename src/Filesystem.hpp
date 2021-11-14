//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#ifndef FILESYSTEM_HPP_
#define FILESYSTEM_HPP_

#include <Config.hpp>
#include <hardware/ESP8266.h>

namespace open_heat {
class Filesystem {
  public:
  bool setup();
  Config& getConfig();
  void clearConfig();

  void persistConfig();

  void format();

  private:
  void listFiles();
  void initConfig();
  static bool isConfigValid();

  static constexpr const char* configFile_ = "/config.dat";

  Config config_{};
  bool setup_ = false;
  FS* filesystem = &FileFS;
};
} // namespace open_heat

#endif // FILESYSTEM_HPP_
