//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#ifndef WIFIMANAGER_HPP_
#define WIFIMANAGER_HPP_

#include <chrono>

#include <Config.hpp>
#include <Filesystem.hpp>

namespace open_heat {
namespace network {
class WifiManager {
  public:
  WifiManager(
    Filesystem* filesystem,
    AsyncWebServer& webServer,
    DNSServer* dnsServer,
    DoubleResetDetector* drd) :
      webServer_(webServer),
      dnsServer_(dnsServer),
      wifiMulti_(WIFI_MULTI()),
      drd_{drd},
      filesystem_(filesystem){};

  void setup(bool doubleReset);
  bool checkWifi();

  private:
  bool showConfigurationPortal(ESPAsync_WiFiManager* espWifiManager);

  void updateConfig(ESPAsync_WiFiManager* espWifiManager);
  void updateWifiCredentials(ESPAsync_WiFiManager* espWifiManager) const;

  bool loadAPsFromConfig();

  uint8_t connectMultiWiFi();

  void initSTAIPConfigStruct(WiFi_STA_IPConfig& ipConfig);
  void initAdditionalParams();

  static void clearSettings(Config& config);
  void updateSettings(Config& config);

  unsigned char reconnectCount_ = 0;

  AsyncWebServer& webServer_;
  DNSServer* dnsServer_{};
  WIFI_MULTI wifiMulti_{};
  DoubleResetDetector* drd_{};
  Filesystem* filesystem_{};

  ESPAsync_WMParameter paramUpdateUsername_{
    "UpdateUsername",
    "Update Username",
    DEFAULT_USER,
    UPDATE_MAX_USERNAME_LEN};

  ESPAsync_WMParameter paramUpdatePassword_{
    "UpdatePassword",
    "Update Password",
    DEFAULT_PW,
    UPDATE_MAX_PW_LEN};

  ESPAsync_WMParameter paramHostname_{
    "Hostname",
    "Hostname",
    DEFAULT_HOST_NAME,
    HOST_NAME_MAX_LEN};

  ESPAsync_WMParameter* additionalParameters_[3] = {
    &paramHostname_,
    &paramUpdateUsername_,
    &paramUpdatePassword_,

    //  &paramMqttServer_,
    // e &paramMqttPortString_,
    // &paramMqttTopic_,
    // &paramMqttUsername_,
    /*&paramMqttPassword_,
    &paramMotorGround_,
    &paramMotorVin_,
    &paramWindowGround_,
    &paramWindowVin_,*/
  };

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

#if (USE_DHCP_IP || (defined(USE_STATIC_IP_CONFIG_IN_CP) && !USE_STATIC_IP_CONFIG_IN_CP))
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
