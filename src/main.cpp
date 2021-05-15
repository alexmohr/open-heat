//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#include <Arduino.h>
#include <Logger.hpp>

#include "Filesystem.hpp"
#include "network/WifiManager.hpp"

#include <network/MQTT.hpp>
#include <network/WebServer.hpp>

#include <sensors/ITemperatureSensor.hpp>

DNSServer dnsServer_;
DoubleResetDetector drd_(DRD_TIMEOUT, DRD_ADDRESS);

#if TEMP_SENSOR == BME280
#include <heating/RadiatorValve.hpp>
#include <sensors/BME280.hpp>
#include <sensors/WindowSensor.hpp>
#include <cstring>
open_heat::sensors::ITemperatureSensor* tempSensor_ = new open_heat::sensors::BME280();
#elif TEMP_SENSOR == TP100
#include <sensors/TP100.hpp>
open_heat::sensors::ITemperatureSensor* tempSensor_ = new open_heat::sensors::TP100();
#else
#error "Not supported temp sensor"
#endif

open_heat::Filesystem filesystem_;

open_heat::heating::RadiatorValve valve_(*tempSensor_, filesystem_);
open_heat::network::WebServer webServer_(filesystem_, *tempSensor_, valve_);
open_heat::sensors::WindowSensor windowSensor_(&filesystem_, &valve_);

open_heat::network::WifiManager wifiManager_(
  &filesystem_,
  webServer_.getWebServer(),
  &dnsServer_,
  &drd_);

open_heat::network::MQTT mqtt_(filesystem_, wifiManager_, *tempSensor_, &valve_);

void setupSerial()
{
  Serial.begin(MONITOR_SPEED);
  Serial.setTimeout(2000);

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
  //setupSerial();
  open_heat::Logger::setup();
  open_heat::Logger::setLogLevel(open_heat::Logger::FATAL);

  setupPins();
  filesystem_.setup();
  tempSensor_->setup();

  wifiManager_.setup();

  valve_.setup();
  mqtt_.setup();
  webServer_.setup();
  windowSensor_.setup();

  logVersions();
  open_heat::Logger::log(open_heat::Logger::INFO, "Device startup and setup done");

//  mqtt_.enableDebug();
}

void loop()
{
  wifi_set_sleep_type(NONE_SLEEP_T);

  // Call the double reset detector loop method every so often,
  // so that it can recognise when the timeout expires.
  // You can also call drd.stop() when you wish to no longer
  // consider the next reset as a double reset.
  drd_.loop();

  open_heat::sensors::WindowSensor::loop();
  const auto valveSleep = valve_.loop();
  const auto mqttSleep = mqtt_.loop();

  if (millis() < 10 * 1000) {
    delay(100);
    return;
  }

  drd_.stop();

  // do not sleep if debug is enabled.
  if (open_heat::network::MQTT::debug()) {
    delay(100);
    return;
  }

  open_heat::Logger::log(
    open_heat::Logger::DEBUG,
    "Sleep times: valveSleep %lu, mqttSleep %lu",
    valveSleep - millis(),
    mqttSleep - millis());

  const auto minSleepTime = 5000UL;
  unsigned long idleTime;
  auto nextCheckMillis = std::min(mqttSleep, valveSleep);
  if (nextCheckMillis < (millis() + minSleepTime)) {
    open_heat::Logger::log(
      open_heat::Logger::DEBUG,
      "Minimal sleep or underflow prevented, sleep set to %ul ms",
      minSleepTime);
    idleTime = minSleepTime;
  } else {
    idleTime = nextCheckMillis - millis();
  }

  open_heat::Logger::log(open_heat::Logger::DEBUG, "Sleeping for %lu ms", idleTime);

  const auto& config = filesystem_.getConfig();
  wifi_set_sleep_type(LIGHT_SLEEP_T);

  // Wait one second before forcing sleep to send messages.
  delay(500);
  if (idleTime == minSleepTime) {
    return;
  }

  WiFi.forceSleepBegin();
  delay(idleTime);
  WiFi.forceSleepWake();
}
