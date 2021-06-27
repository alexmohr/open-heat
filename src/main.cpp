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
#include "RTCMemory.hpp"
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
 // pinMode(LED_BUILTIN, OUTPUT);
//  digitalWrite(LED_BUILTIN, LED_OFF);

  const auto& config = filesystem_.getConfig();
  if (config.MotorPins.Vin > 1) {
    pinMode(static_cast<uint8_t>(config.MotorPins.Vin), OUTPUT);
  } else {
    open_heat::Logger::log(
      open_heat::Logger::ERROR, "Motor pin vin invalid: %i\n", config.MotorPins.Vin);
  }
  if (config.MotorPins.Ground > 1) {
    pinMode(static_cast<uint8_t>(config.MotorPins.Ground), OUTPUT);
  } else {
    open_heat::Logger::log(
      open_heat::Logger::ERROR,
      "Motor pin ground invalid: %i\n",
      config.MotorPins.Ground);
  }
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
  logVersions();

  auto rtcMem = open_heat::readRTCMemory();
  open_heat::Logger::log(open_heat::Logger::DEBUG, "canary value: %#16x", rtcMem.canary);

  if (rtcMem.canary == open_heat::CANARY) {
    open_heat::Logger::log(open_heat::Logger::DEBUG, "woke up from deep sleep");
    drd_.stop();

  } else {
    std::memset(&rtcMem, 0, sizeof(open_heat::RTCMemory));
    rtcMem.canary = open_heat::CANARY;
    open_heat::writeRTCMemory(rtcMem);

    open_heat::Logger::log(open_heat::Logger::DEBUG, "Initalized rtcmem");
  }

  filesystem_.setup();
  setupPins();
  tempSensor_->setup();

  wifiManager_.setup();

  valve_.setup();
  mqtt_.setup();
  webServer_.setup();
  windowSensor_.setup();

  open_heat::Logger::log(open_heat::Logger::INFO, "Device startup and setup done");

  loop();
}

void wifiSleep(uint64_t timeInMs)
{
  open_heat::Logger::log(open_heat::Logger::DEBUG, "Sleeping for %lu ms", timeInMs);

  wifi_fpm_open();
  wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
  wifi_set_opmode(NULL_MODE);

  const auto rv = wifi_fpm_do_sleep(timeInMs * 1000);
  if (rv != ESP_OK) {
    open_heat::Logger::log(open_heat::Logger::DEBUG, "Failed to sleep: %i");
    return;
  }

  // ESP8266 will not enter sleep mode immediately,
  // it is going to sleep in the system idle task
  delay(timeInMs);

  if (open_heat::offsetMillis() > 10 * 1000) {
    drd_.stop();
  }

  wifiManager_.setup();
}

void wifiDeepSleep(uint64_t timeInMs)
{
  open_heat::Logger::log(open_heat::Logger::DEBUG, "Sleeping for %lu ms", timeInMs);
  auto mem = open_heat::readRTCMemory();
  mem.millisOffset = open_heat::offsetMillis();
  open_heat::writeRTCMemory(mem);

  ESP.deepSleep(timeInMs * 1000, RF_DISABLED);
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

  // do not sleep if debug is enabled.
  if (open_heat::network::MQTT::debug()) {
    delay(100);

    if (open_heat::offsetMillis() - lastLogMillis_ > 60 * 1000) {
      open_heat::Logger::log(open_heat::Logger::DEBUG, "DEBUG MODE: Sleep disabled");
      lastLogMillis_ = open_heat::offsetMillis();
    }

    return;
  }

  open_heat::Logger::log(
    open_heat::Logger::DEBUG,
    "Sleep times: valveSleep %lu, mqttSleep %lu",
    valveSleep - open_heat::offsetMillis(),
    mqttSleep - open_heat::offsetMillis());

  const auto minSleepTime = 5000UL;
  unsigned long idleTime;
  auto nextCheckMillis = std::min(mqttSleep, valveSleep);
  if (nextCheckMillis < (open_heat::offsetMillis() + minSleepTime)) {
    open_heat::Logger::log(
      open_heat::Logger::DEBUG,
      "Minimal sleep or underflow prevented, sleep set to %ul ms",
      minSleepTime);
    idleTime = minSleepTime;
  } else {
    idleTime = nextCheckMillis - open_heat::offsetMillis();
  }

  // Wait 0.5 seconds before forcing sleep to send messages.
  delay(500);
  // wifiSleep(idleTime);
  wifiDeepSleep(idleTime);

  // // WiFi.forceSleepBegin();
  //  **delay(idleTime);
  //  WiFi.forceSleepWake();
}
