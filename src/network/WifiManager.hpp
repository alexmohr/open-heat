//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#ifndef WIFIMANAGER_HPP_
#define WIFIMANAGER_HPP_

#include <chrono>

#include "WebServer.hpp"
#include <Config.hpp>
#include <Filesystem.hpp>

namespace open_heat {
namespace network {
class WifiManager {
  public:
  WifiManager(Filesystem* filesystem, WebServer& webServer) :
      webServer_(webServer), wifiMulti_(WIFI_MULTI()), filesystem_(filesystem){};

  void setup(bool doubleReset);
  bool checkWifi();

  private:
  [[noreturn]] bool showConfigurationPortal();
  bool loadAPsFromConfig();
  std::vector<String> getApList() const;

  uint8_t connectMultiWiFi();

  unsigned char reconnectCount_ = 0;

  WebServer& webServer_;
  WIFI_MULTI wifiMulti_{};
  Filesystem* filesystem_{};
};

} // namespace network
} // namespace open_heat

#endif // WIFIMANAGER_HPP_
