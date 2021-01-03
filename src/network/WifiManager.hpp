//
// Copyright (c) 2020 Alexander Mohr
// Licensed under the terms of the MIT license
//
#ifndef WIFIMANAGER_HPP_
#define WIFIMANAGER_HPP_

#include <chrono>

#include <network/WifiManagerConfig.hpp>

namespace open_heat {
namespace network {
class WifiManager {
public:

  WifiManager(std::chrono::milliseconds checkInterval,
              AsyncWebServer *webServer, DNSServer *dnsServer,
              WM_Config *wmConfig_, ESP8266WiFiMulti *wifiMulti,
              DoubleResetDetector *drd)
      : checkInterval_(checkInterval), webServer_(webServer),
        dnsServer_(dnsServer), wifiMulti_(wifiMulti),
        drd_{drd}, wmConfig_(wmConfig_) {};

  void checkStatus();
  void setup();

private:
  void setupConfigAP(ESPAsync_WiFiManager *espWifiManager);
  void updateConfig(ESPAsync_WiFiManager *espWifiManager, uint8_t i) const;

  void loadConfigFromFS();
  void saveConfigToFS();


  void addAccessPointFromConfig(uint8_t i);

  void checkWifi();
  uint8_t connectMultiWiFi();

  void initSTAIPConfigStruct(WiFi_STA_IPConfig &ipConfig);

  const std::chrono::milliseconds checkInterval_;
  ulong lastWifiCheckMillis_ = 0;


  AsyncWebServer *webServer_{};
  DNSServer *dnsServer_{};
  WIFI_MULTI *wifiMulti_{};
  DoubleResetDetector *drd_{};

  WM_Config *wmConfig_{};

// Use USE_DHCP_IP == true for dynamic DHCP IP, false to use static IP which you
// have to change accordingly to your network
#if (defined(USE_STATIC_IP_CONFIG_IN_CP) && !USE_STATIC_IP_CONFIG_IN_CP)
  // Force DHCP to be true
#if defined(USE_DHCP_IP)
#undef USE_DHCP_IP
#endif

#define USE_DHCP_IP true
#else
// You can select DHCP or Static IP here
#define USE_DHCP_IP true
//#define USE_DHCP_IP     false
#endif

#if (USE_DHCP_IP ||                                                            \
     (defined(USE_STATIC_IP_CONFIG_IN_CP) && !USE_STATIC_IP_CONFIG_IN_CP))
// Use DHCP
#warning Using DHCP IP
  IPAddress stationIP = IPAddress(0, 0, 0, 0);
  IPAddress gatewayIP = IPAddress(192, 168, 2, 1);
  IPAddress netMask = IPAddress(255, 255, 255, 0);
#else
  // Use static IP
#warning Using static IP
#ifdef ESP32
  IPAddress stationIP = IPAddress(192, 168, 2, 232);
#else
  IPAddress stationIP = IPAddress(192, 168, 2, 186);
#endif

  IPAddress gatewayIP = IPAddress(192, 168, 2, 1);
  IPAddress netMask = IPAddress(255, 255, 255, 0);
#endif

  IPAddress dns1IP = gatewayIP;
  IPAddress dns2IP = IPAddress(8, 8, 8, 8);

  // New in v1.4.0
  IPAddress APStaticIP = IPAddress(192, 168, 100, 1);
  IPAddress APStaticGW = IPAddress(192, 168, 100, 1);
  IPAddress APStaticSN = IPAddress(255, 255, 255, 0);
};

} // namespace network
} // namespace open_heat

#endif // WIFIMANAGER_HPP_
