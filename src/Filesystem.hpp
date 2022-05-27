//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#ifndef FILESYSTEM_HPP_
#define FILESYSTEM_HPP_

#include <Config.hpp>
#include <hardware/ESP8266.h>
#include <yal/yal.hpp>

namespace open_heat {
class Filesystem {
  public:
  Filesystem() = default;
  Filesystem(const Filesystem&) = delete;

  bool setup();
  Config& getConfig();
  void clearConfig();

  void persistConfig();

  void format();

  private:
  void listFiles();
  void initConfig();
  bool isConfigValid();

  [[nodiscard]] String formatBytes(size_t bytes);

  static constexpr const char* configFile_ = "/config.dat";

  Config m_config{};
  bool m_setup = false;
  FS* m_filesystem = &FileFS;
  yal::Logger m_logger;
};
} // namespace open_heat

#endif // FILESYSTEM_HPP_
