//
// Copyright (c) 2020 Alexander Mohr
// Licensed under the terms of the MIT license
//
// This configuration is directly taken from
// https://github.com/khoih-prog/ESP_WiFiManager
#ifndef WIFIMANAGERCONFIG_HPP_
#define WIFIMANAGERCONFIG_HPP_

#define CONFIG_FILENAME F("/wifi_cred.dat")

// From v1.0.10 to permit disable/enable StaticIP configuration in Config Portal
// from sketch. Valid only if DHCP is used. You'll loose the feature of
// dynamically changing from DHCP to static IP, or vice versa You have to
// explicitly specify false to disable the feature.
//#define USE_STATIC_IP_CONFIG_IN_CP          false

// Use false to disable NTP config. Advisable when using Cellphone, Tablet to
// access Config Portal. See Issue 23: On Android phone ConfigPortal is
// unresponsive (https://github.com/khoih-prog/ESP_WiFiManager/issues/23)
#define USE_ESP_WIFIMANAGER_NTP false

// Use true to enable CloudFlare NTP service. System can hang if you don't have
// Internet access while accessing CloudFlare See Issue #21: CloudFlare link in
// the default portal (https://github.com/khoih-prog/ESP_WiFiManager/issues/21)
#define USE_CLOUDFLARE_NTP false

#define USING_CORS_FEATURE true


#define HEARTBEAT_INTERVAL 10000L


#include "hardware/HAL.hpp"
#include "network/WifiCredentials.hpp"

#include <ESPAsync_WiFiManager.h>

typedef struct {
  WiFi_Credentials WiFi_Creds[NUM_WIFI_CREDENTIALS];
} WM_Config;

#endif // WIFIMANAGERCONFIG_HPP_
