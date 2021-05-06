//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//
#ifndef ESP32_H_
#define ESP32_H_

#if !defined(ESP32)
#error This code is intended to run on the ESP8266 platform! Please check your Tools->Board setting.
#endif

#include <FS.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <esp_wifi.h>

// From v1.1.0
#include <WiFiMulti.h>
#define WIFI_MULTI WiFiMulti

#include <ESPmDNS.h>

// The library will be depreciated after being merged to future major Arduino esp32 core
// release 2.x At that time, just remove this library inclusion
#include <LITTLEFS.h> // https://github.com/lorol/LITTLEFS

#define ESP_getChipId() ((uint32_t)ESP.getEfuseMac())

// static constexpr uint8_t LED_BUILTIN = 2;
static constexpr uint8_t LED_ON = HIGH;
static constexpr uint8_t LED_OFF = LOW;

// Filesystem
#include "FS.h"

// The library will be depreciated after being merged to future major Arduino esp32 core
// release 2.x At that time, just remove this library inclusion
#include <LITTLEFS.h> // https://github.com/lorol/LITTLEFS

#define FileFS LITTLEFS
#define FS_Name "LittleFS"

// Double reset detector
#define ESP_DRD_USE_LITTLEFS true
#define ESP_DRD_USE_SPIFFS false
#define ESP_DRD_USE_EEPROM false

// For ESP32, this better be 0 to shorten the connect time
#define WIFI_MULTI_1ST_CONNECT_WAITING_MS 0

#endif // ESP32_H_
