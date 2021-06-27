//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#include <Arduino.h>
#include <Logger.hpp>

#include <Filesystem.hpp>
#include <network/WifiManager.hpp>

#include <network/MQTT.hpp>
#include <network/WebServer.hpp>

#include <hardware/esp_err.h>

DNSServer dnsServer_;
DoubleResetDetector drd_(DRD_TIMEOUT, DRD_ADDRESS);

#if TEMP_SENSOR == BME280
#include <sensors/BME280.hpp>
#include <sensors/WindowSensor.hpp>
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

unsigned long lastLogMillis_ = 0;

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
  if (open_heat::Logger::getLogLevel() < open_heat::Logger::OFF) {
    setupSerial();
  }

  open_heat::Logger::setup();

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
}

void wifiSleep(uint64_t timeInMs)
{
  open_heat::Logger::log(open_heat::Logger::DEBUG, "Sleeping for %lu ms", timeInMs);

  wifi_fpm_open();
  wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
  wifi_set_opmode(NULL_MODE);

  const auto rv = wifi_fpm_do_sleep(timeInMs * 1000);
  if (rv != ESP_OK) {
    open_heat::Logger::log(
      open_heat::Logger::DEBUG, "Failed to sleep: %i");
    return;
  }

  // ESP8266 will not enter sleep mode immediately,
  // it is going to sleep in the system idle task
  delay(timeInMs);

  if (millis() > 10 * 1000) {
    drd_.stop();
  }

  wifiManager_.setup();
}

void loop()
{
  // Call the double reset detector loop method every so often,
  // so that it can recognise when the timeout expires.
  // You can also call drd.stop() when you wish to no longer
  // consider the next reset as a double reset.
  drd_.loop();

  open_heat::sensors::WindowSensor::loop();
  const auto valveSleep = valve_.loop();
  const auto mqttSleep = mqtt_.loop();

  if (millis() > 10 * 1000) {
    drd_.stop();
  }

  // do not sleep if debug is enabled.
  if (open_heat::network::MQTT::debug()) {
    delay(100);

    if (millis() - lastLogMillis_ > 60 * 1000) {
      open_heat::Logger::log(open_heat::Logger::DEBUG, "DEBUG MODE: Sleep disabled");
      lastLogMillis_ = millis();
    }

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

  // Wait 0.5 seconds before forcing sleep to send messages.
  delay(500);
  wifiSleep(idleTime);

  // // WiFi.forceSleepBegin();
  //  **delay(idleTime);
  //  WiFi.forceSleepWake();
}
