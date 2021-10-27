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
  open_heat::Logger::log(open_heat::Logger::DEBUG, "Running setupPins");

  // set led pin as output
  pinMode(LED_BUILTIN_PIN, OUTPUT);
  // level is inverted
  digitalWrite(LED_BUILTIN_PIN, LED_ON);

  // set led pin as output
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LED_OFF);

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

  // enable temp sensor
  if (config.TempVin > 0) {
    pinMode(static_cast<uint8_t>(config.TempVin), OUTPUT);
    digitalWrite(static_cast<uint8_t>(config.TempVin), HIGH);
  } else {
    open_heat::Logger::log(
      open_heat::Logger::ERROR, "Temp pin vin invalid: %i\n", config.TempVin);
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

void wifiDeepSleep(uint64_t timeInMs, bool enableRF)
{
  const auto& config = filesystem_.getConfig();
  digitalWrite(static_cast<uint8_t>(config.TempVin), LOW);
  digitalWrite(static_cast<uint8_t>(config.MotorPins.Vin), LOW);
  digitalWrite(static_cast<uint8_t>(config.MotorPins.Ground), LOW);

  pinMode(static_cast<uint8_t>(config.TempVin), INPUT);
  pinMode(static_cast<uint8_t>(config.MotorPins.Vin), INPUT);
  pinMode(static_cast<uint8_t>(config.MotorPins.Ground), INPUT);
  pinMode(static_cast<uint8_t>(LED_BUILTIN), INPUT);

  open_heat::Logger::log(open_heat::Logger::INFO, "Sleeping for %lu ms", timeInMs);
  open_heat::rtc::setMillisOffset(open_heat::rtc::offsetMillis() + timeInMs);

  ESP.deepSleep(timeInMs * 1000, enableRF ? RF_CAL : RF_DISABLED);
  delay(1);
}

void setup()
{
  if (open_heat::Logger::getLogLevel() < open_heat::Logger::OFF && !DISABLE_ALL_LOGGING) {
    setupSerial();
  }

  open_heat::Logger::setup();
  logVersions();
  filesystem_.setup();

  const auto rtcMem = open_heat::rtc::read();
  open_heat::Logger::log(open_heat::Logger::DEBUG, "canary value: %#16x", rtcMem.canary);

  if (rtcMem.canary == open_heat::rtc::CANARY) {
    open_heat::Logger::log(open_heat::Logger::DEBUG, "woke up from deep sleep");
    drd_.stop();
  } else {
    open_heat::rtc::init(filesystem_);
  }

  setupPins();

  if (!tempSensor_->setup()) {
    // blink led 10 times to indicate temp sensor error
    for (auto i = 0; i < 10; ++i) {
      digitalWrite(LED_PIN, LED_OFF);
      delay(250);
      digitalWrite(LED_PIN, LED_ON);
    }
  }

  valve_.setup();

  if (mqtt_.needLoop()) {
    wifiManager_.setup(!rtcMem.drdDisabled && drd_.detectDoubleReset());
    mqtt_.setup();
  }

  //windowSensor_.setup();

  open_heat::Logger::log(open_heat::Logger::INFO, "Device startup and setup done");
  while (open_heat::rtc::offsetMillis() < 10*1000) {
    // Call the double reset detector loop method every so often,
    // so that it can recognise when the timeout expires.
    // You can also call drd.stop() when you wish to no longer
    // consider the next reset as a double reset.
    drd_.loop();
    delay(1000);
  }

  open_heat::rtc::setDrdDisabled(true);
  drd_.stop();


  loop();
}

void loop()
{
 // open_heat::sensors::WindowSensor::loop();
  const auto mqttSleep = mqtt_.loop();
  const auto valveSleep = valve_.loop();

  const auto msg
    = "Sleep times: valveSleep: " + std::to_string(valveSleep - open_heat::rtc::offsetMillis())
    + ", mqttSleep: " + std::to_string(mqttSleep - open_heat::rtc::offsetMillis());
  open_heat::Logger::log(open_heat::Logger::DEBUG, msg.c_str());

  // do not sleep if debug is enabled.
  if (open_heat::network::MQTT::debug()) {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LED_ON);
    webServer_.setup();

    delay(100);
    digitalWrite(LED_PIN, LED_OFF);
    delay(100);

    if (open_heat::rtc::offsetMillis() - lastLogMillis_ > 60 * 1000) {
      open_heat::Logger::log(open_heat::Logger::DEBUG, "DEBUG MODE: Sleep disabled");
      lastLogMillis_ = open_heat::rtc::offsetMillis();
    }

    return;
  }

  const auto minSleepTime = 10000UL;
  unsigned long idleTime;
  unsigned long nextCheckMillis;
  bool enableWifi = false;
  if (valveSleep < mqttSleep) {
    nextCheckMillis = valveSleep;
  } else {
    nextCheckMillis = mqttSleep;
    enableWifi = true;
  }

  if (nextCheckMillis < (open_heat::rtc::offsetMillis() + minSleepTime)) {
    open_heat::Logger::log(
      open_heat::Logger::DEBUG,
      "Minimal sleep or underflow prevented, sleep set to %ul ms",
      minSleepTime);
    idleTime = minSleepTime;
  } else {
    idleTime = nextCheckMillis - open_heat::rtc::offsetMillis();
  }

  open_heat::Logger::log(
    open_heat::Logger::DEBUG, "Starting deep sleep for: %lu", idleTime);

  // Wait before forcing sleep to send messages.
  delay(100);
  wifiDeepSleep(idleTime, enableWifi);
}
