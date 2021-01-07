//
// Copyright (c) 2020 Alexander Mohr 
// Licensed under the terms of the MIT license
//
#ifndef FILESYSTEM_HPP_
#define FILESYSTEM_HPP_

#include "hardware/HAL.hpp"
#include <Config.hpp>

namespace open_heat {
class Filesystem {
 public:
     void setup();
      Config& getConfig();
      void clearConfig();

      void persistConfig();

 private:
  void listFiles();
  void initConfig();
  bool isConfigValid();

  static constexpr const char* configFile_ = "/config.dat";

  Config config_{};
  FS* filesystem = &FileFS;
};
}



#endif //FILESYSTEM_HPP_
