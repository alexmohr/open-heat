//
// Copyright (c) 2020 Alexander Mohr
// Open-HEAT - Radiator control for ESP8266
// Licensed under the terms of the MIT license
//

#include <Arduino.h>
#include <Logger.hpp>

#include "Filesystem.hpp"
#include "network/WifiManager.hpp"

#include <network/MQTT.hpp>
#include <network/WebServer.hpp>

#include <sensors/ITemperatureSensor.hpp>


static constexpr std::chrono::milliseconds wifiCheckInterval(5000);


DNSServer dnsServer_;
DoubleResetDetector drd_(DRD_TIMEOUT, DRD_ADDRESS);

#if TEMP_SENSOR == BME280
#include <heating/RadiatorValve.hpp>
#include <sensors/BME280.hpp>
open_heat::sensors::ITemperatureSensor* tempSensor_ = new open_heat::sensors::BME280();
#elif TEMP_SENSOR == TP100
#include <sensors/TP100.hpp>
open_heat::sensors::ITemperatureSensor* tempSensor_ = new open_heat::sensors::TP100();
#else
#error "Not supported temp sensor"
#endif

open_heat::Filesystem filesystem_;


open_heat::heating::RadiatorValve valve_(*tempSensor_, filesystem_);
open_heat::network::MQTT mqtt_(filesystem_, *tempSensor_, &valve_);
open_heat::network::WebServer webServer_(filesystem_, *tempSensor_, valve_);


open_heat::network::WifiManager wifiManager_(
  wifiCheckInterval,
  &filesystem_,
  webServer_.getWebServer(),
  &dnsServer_,
  &drd_);
//
//typedef struct {
//  uint32_t namesz;
//  uint32_t descsz;
//  uint32_t type;
//  uint8_t data[];
//} ElfNoteSection_t;
//
//extern const ElfNoteSection_t g_note_build_id;
//
//void print_build_id(void) {
//  const uint8_t *build_id_data = &g_note_build_id.data[g_note_build_id.namesz];
//
//  printf("Build ID: ");
//  for (int i = 0; i < g_note_build_id.descsz; ++i) {
//    printf("%02x", build_id_data[i]);
//  }
//  printf("\n");
//}


void waitForSerialPort()
{
  while (!Serial) {
    delay(200);
  }
  Serial.println("");
}

void setupPins()
{
  // set led pin as output
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LED_OFF);

  // Todo make these pins configurable
  pinMode(D5, OUTPUT);
  pinMode(D6, OUTPUT);
}

void logVersions()
{
  open_heat::Logger::log(open_heat::Logger::DEBUG, "Board: %s", ARDUINO_BOARD);
  open_heat::Logger::log(
    open_heat::Logger::DEBUG, "Build date: %s, %s", __DATE__, __TIME__);
  open_heat::Logger::log(
    open_heat::Logger::DEBUG, "WifiManager Version: %s", ESP_ASYNC_WIFIMANAGER_VERSION);
}

void setup()
{
  Serial.begin(MONITOR_SPEED);
  waitForSerialPort();

  open_heat::Logger::setup();
  open_heat::Logger::setLogLevel(open_heat::Logger::TRACE);

  setupPins();



  filesystem_.setup();
  tempSensor_->setup();

  wifiManager_.setup();

  valve_.setup();
  mqtt_.setup();
  webServer_.setup();

  logVersions();
  open_heat::Logger::log(open_heat::Logger::DEBUG, "Setup done");
}

void loop()
{
  // Call the double reset detector loop method every so often,
  // so that it can recognise when the timeout expires.
  // You can also call drd.stop() when you wish to no longer
  // consider the next reset as a double reset.
  drd_.loop();
  wifiManager_.loop();
  valve_.loop();
  mqtt_.loop();
}
