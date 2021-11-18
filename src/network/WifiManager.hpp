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
      webServer_(webServer), filesystem_(filesystem){};

  void setup(bool doubleReset);
  bool checkWifi();

  private:
  struct fastConfig {
    uint8_t* bssid = nullptr;
    uint32_t channel = 0;
  };

  [[noreturn]] bool showConfigurationPortal();
  bool loadAPsFromConfig();
  std::vector<String> getApList() const;
  bool getFastConnectConfig(const String& ssid, fastConfig& config);

  wl_status_t connectMultiWiFi();

  unsigned char reconnectCount_ = 0;

  WebServer& webServer_;
  Filesystem* filesystem_{};
};

} // namespace network
} // namespace open_heat

#endif // WIFIMANAGER_HPP_
