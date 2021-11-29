//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#include <Arduino.h>
#include <Logger.hpp>

#include <Filesystem.hpp>
#include <Serial.hpp>
#include <hardware/DoubleResetDetector.hpp>
#include <hardware/esp_err.h>
#include <network/MQTT.hpp>
#include <network/WebServer.hpp>
#include <network/WifiManager.hpp>
#include <sensors/Battery.hpp>

#include "RTCMemory.hpp"
#include <sensors/BME280.hpp>
#include <sensors/BMP280.hpp>
#include <sensors/WindowSensor.hpp>

// external voltage
ADC_MODE(ADC_TOUT);

DoubleResetDetector g_drd(DRD_TIMEOUT, DRD_ADDRESS);

open_heat::sensors::Temperature* g_tempSensor = nullptr;
open_heat::sensors::Humidity* g_humidSensor = nullptr;

open_heat::Filesystem g_filesystem;

open_heat::heating::RadiatorValve g_valve(g_tempSensor, g_filesystem);
open_heat::sensors::Battery g_battery;

open_heat::network::WebServer g_webServer(g_filesystem, g_tempSensor, g_battery, g_valve);
// open_heat::sensors::WindowSensor g_windowSensor(g_filesystem, g_valve);

open_heat::Serial g_serial;

open_heat::network::WifiManager g_wifiManager_(g_filesystem, g_webServer);

open_heat::network::MQTT g_mqtt(
  &g_filesystem,
  g_wifiManager_,
  g_tempSensor,
  g_humidSensor,
  &g_valve,
  &g_battery);

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

  const auto& config = g_filesystem.getConfig();
  if (config.MotorPins.Vin > 1) {
    pinMode(static_cast<uint8_t>(config.MotorPins.Vin), OUTPUT);
    digitalWrite(static_cast<uint8_t>(config.MotorPins.Ground), LOW);
  } else {
    open_heat::Logger::log(
      open_heat::Logger::ERROR, "Motor pin vin invalid: %i", config.MotorPins.Vin);
  }
  if (config.MotorPins.Ground > 1) {
    pinMode(static_cast<uint8_t>(config.MotorPins.Ground), OUTPUT);
    digitalWrite(static_cast<uint8_t>(config.MotorPins.Ground), LOW);

  } else {
    open_heat::Logger::log(
      open_heat::Logger::ERROR, "Motor pin ground invalid: %i", config.MotorPins.Ground);
  }

  // enable temp sensor
  if (config.TempVin > 0) {
    pinMode(static_cast<uint8_t>(config.TempVin), OUTPUT);
    digitalWrite(static_cast<uint8_t>(config.TempVin), HIGH);
  } else {
    open_heat::Logger::log(
      open_heat::Logger::ERROR, "Temp pin vin invalid: %i", config.TempVin);
  }
}

void setupTemperatureSensor()
{
  open_heat::Logger::log(open_heat::Logger::DEBUG, "Running setupTemperatureSensor");
  const auto& config = g_filesystem.getConfig();

  open_heat::sensors::Sensor* sensor;
  if (config.TempSensor == BME) {
    auto bme = new open_heat::sensors::BME280();
    g_humidSensor = bme;
    g_tempSensor = bme;
    sensor = reinterpret_cast<open_heat::sensors::Sensor*>(bme);
  } else {
    auto bmp = new open_heat::sensors::BMP280();
    g_humidSensor = nullptr;
    g_tempSensor = bmp;
    sensor = reinterpret_cast<open_heat::sensors::Sensor*>(bmp);
  }

  if (!sensor->setup()) {
    // blink led 10 times to indicate temp sensor error
    for (auto i = 0; i < 10; ++i) {
      digitalWrite(LED_PIN, LED_OFF);
      delay(250);
      digitalWrite(LED_PIN, LED_ON);
    }
    open_heat::Logger::log(open_heat::Logger::ERROR, "Failed to init temp sensor");
  }
}

bool isDoubleReset()
{
  if (ESP.getResetInfoPtr()->reason != REASON_EXT_SYS_RST) {
    open_heat::Logger::log(
      open_heat::Logger::DEBUG,
      "no double reset, resetInfo: %s",
      ESP.getResetInfo().c_str());
    open_heat::rtc::setDrdDisabled(true);
    return false;
  }

  const auto drdDetected = g_drd.detectDoubleReset();

  open_heat::Logger::log(open_heat::Logger::DEBUG, "DRD detected: %i", drdDetected);

  const auto millisSinceReset
    = open_heat::rtc::read().lastResetTime - open_heat::rtc::offsetMillis();

  open_heat::Logger::log(
    open_heat::Logger::DEBUG,
    "millis since reset: %lu, last reset time %lu, offset millis: %lu",
    millisSinceReset,
    open_heat::rtc::read().lastResetTime,
    open_heat::rtc::offsetMillis());

  const auto doubleReset = (drdDetected || millisSinceReset < 10 * 1000)
    && !open_heat::rtc::read().drdDisabled;

  open_heat::rtc::setDrdDisabled(doubleReset);
  return doubleReset;
}

void setup()
{
  if (open_heat::Logger::getLogLevel() < open_heat::Logger::OFF && !DISABLE_ALL_LOGGING) {
    g_serial.setup();
  }

  open_heat::Logger::setup();
  const auto configValid = g_filesystem.setup();

  if (EspClass::getResetInfoPtr()->reason == REASON_DEEP_SLEEP_AWAKE) {
    open_heat::Logger::log(open_heat::Logger::DEBUG, "woke up from deep sleep");

    if (!open_heat::rtc::read().drdDisabled) {
      g_drd.stop();
      open_heat::rtc::setDrdDisabled(true);
    }

  } else {
    // system reset
    open_heat::rtc::init(g_filesystem);
  }

  const auto offsetMillis = open_heat::rtc::offsetMillis();
  open_heat::Logger::log(
    open_heat::Logger::DEBUG, "Set last reset time to %lu", offsetMillis);
  open_heat::rtc::setLastResetTime(offsetMillis);

  setupPins();
  setupTemperatureSensor();

  const auto doubleReset = isDoubleReset();

  g_valve.setup();

  if (g_mqtt.needLoop() || doubleReset) {
    g_wifiManager_.setup(doubleReset);
    g_mqtt.setup();
  }

  if (!configValid) {
    g_mqtt.enableDebug(true);
  }

  if (open_heat::rtc::read().debug) {
    g_webServer.setup(nullptr);
  }

  g_drd.stop();
  open_heat::Logger::log(open_heat::Logger::INFO, "Device startup and setup done");

  loop();
}

void loop()
{
  // open_heat::sensors::WindowSensor::loop();
  const auto mqttSleep = g_mqtt.loop();

  // must be after mqtt to process received commands
  const auto valveSleep = g_valve.loop();
  g_drd.loop();

  // do not sleep if debug is enabled.
  if (open_heat::rtc::read().debug) {
    delay(100);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LED_OFF);
    delay(100);

    return;
  }

  const auto msg = "Sleep times: valveSleep: "
    + std::to_string(valveSleep - open_heat::rtc::offsetMillis())
    + ", mqttSleep: " + std::to_string(mqttSleep - open_heat::rtc::offsetMillis());
  open_heat::Logger::log(open_heat::Logger::DEBUG, msg.c_str());

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

  // Wait before forcing sleep to send messages.
  delay(50);
  open_heat::rtc::wifiDeepSleep(idleTime, enableWifi, g_filesystem);
}
