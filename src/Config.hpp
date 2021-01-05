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

static constexpr int MIN_AP_PASSWORD_SIZE = 8;
static constexpr int SSID_MAX_LEN = 32;
static constexpr int PASS_MAX_LEN = 64;

static constexpr int MQTT_DEFAULT_PORT = 1883;
static constexpr char MQTT_SERVER_NAME_MAX_SIZE = 32;
static constexpr int MQTT_PORT_STR_MAX_SIZE = 6;
static constexpr int MQTT_TOPIC_MAX_SIZE = 256;
static constexpr int MQTT_USERNAME_MAX_SIZE = 64;
static constexpr int MQTT_PASSWORD_MAX_SIZE = 64;

typedef struct {
  char wifi_ssid[SSID_MAX_LEN];
  char wifi_pw[PASS_MAX_LEN];
} WiFi_Credentials;

typedef struct {
  String wifi_ssid;
  String wifi_pw;
} WiFi_Credentials_String;

typedef struct Config {
  WiFi_Credentials WifiCredentials[NUM_WIFI_CREDENTIALS]{{"", ""}, {"", ""}};
  WiFi_STA_IPConfig StaticIp;

  char MqttServer[MQTT_SERVER_NAME_MAX_SIZE]{};
  int MqttPort = MQTT_DEFAULT_PORT;
  char MqttTopic[MQTT_TOPIC_MAX_SIZE]{};
  char MqttUsername[MQTT_USERNAME_MAX_SIZE]{};
  char MqttPassword[MQTT_PASSWORD_MAX_SIZE]{};
} Config;

#endif // WIFIMANAGERCONFIG_HPP_
