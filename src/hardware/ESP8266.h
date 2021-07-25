//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//
#ifndef ESP8266_H_
#define ESP8266_H_

#if !defined(ESP8266)
#error This code is intended to run on the ESP8266 platform! Please check your Tools->Board setting.
#endif

#include <FS.h>

#include <ESP8266WiFi.h> //https://github.com/esp8266/Arduino
// needed for library
#include <DNSServer.h>
#include <LittleFS.h>

// From v1.1.0
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>

#define WIFI_MULTI ESP8266WiFiMulti

#define ESP_getChipId() (ESP.getChipId())

static constexpr uint8_t LED_ON = HIGH;
static constexpr uint8_t LED_OFF = LOW;

// Filesystem
#include <LittleFS.h>

#define FileFS LittleFS
#define FS_Name "LittleFS"

// Double reset detector
#define ESP_DRD_USE_LITTLEFS true
#define ESP_DRD_USE_SPIFFS false

// For ESP8266, this better be 2200 to enable connect the 1st time
#define WIFI_MULTI_1ST_CONNECT_WAITING_MS 2200L
#endif
