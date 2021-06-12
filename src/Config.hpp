//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#ifndef WIFIMANAGERCONFIG_HPP_
#define WIFIMANAGERCONFIG_HPP_

#include "hardware/HAL.hpp"
#include "network/WifiCredentials.hpp"
#include <ESPAsync_WiFiManager.h>

static constexpr uint8_t MIN_AP_PASSWORD_SIZE = 8;
static constexpr uint8_t SSID_MAX_LEN = 32;
static constexpr uint8_t PASS_MAX_LEN = 64;

static constexpr uint16_t MQTT_DEFAULT_PORT = 1883;
static constexpr uint8_t MQTT_SERVER_NAME_MAX_SIZE = 32;
static constexpr uint8_t MQTT_PORT_STR_MAX_SIZE = 16;
static constexpr uint16_t MQTT_TOPIC_MAX_SIZE = 64;
static constexpr uint8_t MQTT_USERNAME_MAX_SIZE = 32;
static constexpr uint8_t MQTT_PASSWORD_MAX_SIZE = 32;

static constexpr uint8_t UPDATE_MIN_USERNAME_LEN = 1;
static constexpr uint8_t UPDATE_MAX_USERNAME_LEN = 32;
static constexpr uint8_t UPDATE_MIN_PW_LEN = 1;
static constexpr uint8_t UPDATE_MAX_PW_LEN = 64;

static constexpr uint8_t HOST_NAME_MAX_LEN = 32;

static constexpr uint8_t DEFAULT_MOTOR_GROUND = D6;
static constexpr uint8_t DEFAULT_MOTOR_VIN = D5;

// On devboard defaults are D8 and D7
// PIN_D7 13
// PIN_D8 15
static constexpr int8_t DEFAULT_WINDOW_GROUND = -1;
static constexpr int8_t DEFAULT_WINDOW_VIN = -1;

static constexpr const char* DEFAULT_HOST_NAME = "OpenHeat";

typedef struct {
  char wifi_ssid[SSID_MAX_LEN];
  char wifi_pw[PASS_MAX_LEN];
} WiFi_Credentials;

typedef struct {
  String wifi_ssid;
  String wifi_pw;
} WiFi_Credentials_String;

typedef struct MQTTSettings {
  char Server[MQTT_SERVER_NAME_MAX_SIZE]{};
  unsigned short Port = MQTT_DEFAULT_PORT;
  char Topic[MQTT_TOPIC_MAX_SIZE]{};
  char Username[MQTT_USERNAME_MAX_SIZE]{};
  char Password[MQTT_PASSWORD_MAX_SIZE]{};
} MQTTSettings;

typedef struct UpdateSettings {
  char Username[UPDATE_MAX_USERNAME_LEN]{};
  char Password[UPDATE_MAX_PW_LEN]{};
} UpdateSettings;

// pins are signed to indicate unused with < 0
typedef struct PinSettings {
  int8 Ground{};
  int8 Vin{};
} PinSettings;

enum OperationMode { HEAT, OFF, FULL_OPEN, UNKNOWN };

typedef struct Config {
  WiFi_Credentials WifiCredentials[NUM_WIFI_CREDENTIALS]{{"", ""}, {"", ""}};
  WiFi_STA_IPConfig StaticIp;
  MQTTSettings MQTT{};
  UpdateSettings Update{};
  char Hostname[HOST_NAME_MAX_LEN]{};
  float SetTemperature{18};
  OperationMode Mode{OFF};
  PinSettings MotorPins{};
  PinSettings WindowPins{};
} Config;

#endif // WIFIMANAGERCONFIG_HPP_
