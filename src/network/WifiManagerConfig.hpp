//
// Copyright (c) 2020 Alexander Mohr
// Licensed under the terms of the MIT license
//
// This configuration is directly taken from
// https://github.com/khoih-prog/ESP_WiFiManager
#ifndef WIFIMANAGERCONFIG_HPP_
#define WIFIMANAGERCONFIG_HPP_


#include "hardware/HAL.hpp"
#include "network/WifiCredentials.hpp"

#include <ESPAsync_WiFiManager.h>

typedef struct {
  WiFi_Credentials WiFi_Creds[NUM_WIFI_CREDENTIALS];
} WM_Config;

#endif // WIFIMANAGERCONFIG_HPP_
