//
// Copyright (c) 2020 Alexander Mohr 
// Licensed under the terms of the MIT license
//
#ifndef FILESYSTEM_HPP_
#define FILESYSTEM_HPP_

#include "hardware/HAL.hpp"


namespace open_heat {
class Filesystem {
 public:
  void setup();

 private:
  void listFiles();


  FS* filesystem = &FileFS;
};
}



#endif //FILESYSTEM_HPP_
