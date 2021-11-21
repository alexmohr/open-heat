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
  WifiManager(Filesystem& filesystem, WebServer& webServer) :
      m_webServer(webServer), m_fileSystem(filesystem){};

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

  unsigned char m_reconnectCount = 0;

  WebServer& m_webServer;
  Filesystem& m_fileSystem;
};

} // namespace network
} // namespace open_heat

#endif // WIFIMANAGER_HPP_
