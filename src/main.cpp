//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#include <Arduino.h>
#include <hardware/DoubleResetDetector.hpp>
#include <hardware/esp_err.h>
#include <network/MQTT.hpp>
#include <sensors/Battery.hpp>

#include "RTCMemory.hpp"
#include "hardware/ESP8266.h"
#include "hardware/pins.h"
#include <sensors/BME280.hpp>
#include <sensors/BMP280.hpp>
#include <sensors/WindowSensor.hpp>

#include <yal/appender/ArduinoSerial.hpp>
#include <yal/yal.hpp>

// external voltage
ADC_MODE(ADC_TOUT)

DoubleResetDetector g_drd(DRD_TIMEOUT, DRD_ADDRESS);

open_heat::sensors::Temperature* g_tempSensor = nullptr;
open_heat::sensors::Humidity* g_humidSensor = nullptr;

esp_gui::Configuration g_config;
esp_gui::WebServer g_webServer(80, "open-heat" /*default hostname*/, g_config);
esp_gui::WifiManager g_wifiManager(g_config, g_webServer);

open_heat::heating::RadiatorValve g_valve(g_tempSensor, g_webServer, g_config);
open_heat::sensors::Battery g_battery;

open_heat::network::MQTT g_mqtt(
  g_wifiManager,
  g_config,
  g_tempSensor,
  g_humidSensor,
  g_valve,
  g_battery);

yal::Logger g_logger("main");
yal::appender::ArduinoSerial<HardwareSerial> g_serialAppender(&g_logger, &Serial, true);

void setupPins()
{
  g_logger.log(yal::Level::DEBUG, "Running setupPins");

  // set led pin as output
  pinMode(LED_BUILTIN_PIN, OUTPUT);
  // level is inverted
  digitalWrite(LED_BUILTIN_PIN, LED_ON);

  // set led pin as output
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LED_OFF);

  const auto motorVin = g_config.value<int>(open_heat::config::MOTOR_VIN);
  if (motorVin > 1) {
    pinMode(static_cast<uint8_t>(motorVin), OUTPUT);
    digitalWrite(static_cast<uint8_t>(motorVin), LOW);
  } else {
    g_logger.log(yal::Level::ERROR, "Motor pin vin invalid: %i", motorVin);
  }
  const auto motorGround = g_config.value<int>(open_heat::config::MOTOR_GROUND);
  if (motorGround > 1) {
    pinMode(static_cast<uint8_t>(motorGround), OUTPUT);
    digitalWrite(static_cast<uint8_t>(motorGround), LOW);

  } else {
    g_logger.log(yal::Level::ERROR, "Motor pin ground invalid: %i", motorGround);
  }

  // enable temp sensor
  const auto tempVin = g_config.value<int>(open_heat::config::TEMP_VIN);
  if (tempVin > 0) {
    pinMode(static_cast<uint8_t>(tempVin), OUTPUT);
    digitalWrite(static_cast<uint8_t>(tempVin), HIGH);
  } else {
    g_logger.log(yal::Level::ERROR, "Temp pin vin invalid: %i", tempVin);
  }
}

void setupTemperatureSensor()
{
  g_logger.log(yal::Level::DEBUG, "Running setupTemperatureSensor");

  open_heat::sensors::Sensor* sensor;
  const auto configuredSensor = static_cast<open_heat::config::TemperatureSensor>(
    g_config.value<int>(open_heat::config::TEMP_SENSOR_TYPE));
  if (configuredSensor == open_heat::config::TemperatureSensor::BME) {
    auto* bme = new open_heat::sensors::BME280();
    g_humidSensor = bme;
    g_tempSensor = bme;
    sensor = reinterpret_cast<open_heat::sensors::Sensor*>(bme);
  } else {
    auto* bmp = new open_heat::sensors::BMP280();
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
    g_logger.log(yal::Level::ERROR, "Failed to init temp sensor");
  }
}

bool isDoubleReset()
{
  if (EspClass::getResetInfoPtr()->reason != REASON_EXT_SYS_RST) {
    g_logger.log(
      yal::Level::DEBUG,
      "no double reset, resetInfo: %s",
      EspClass::getResetInfo().c_str());
    open_heat::rtc::setDrdDisabled(true);
    return false;
  }

  const auto drdDetected = g_drd.detectDoubleReset();

  g_logger.log(yal::Level::DEBUG, "DRD detected: %i", drdDetected);

  const auto millisSinceReset
    = open_heat::rtc::read().lastResetTime - open_heat::rtc::offsetMillis();

  g_logger.log(
    yal::Level::DEBUG,
    "millis since reset: %l, last reset time %, offset millis: %",
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
  if (g_logger.level() < yal::Level::OFF && !DISABLE_ALL_LOGGING) {
    g_serialAppender.begin(115200);
  }

  g_config.setup();

  if (EspClass::getResetInfoPtr()->reason == REASON_DEEP_SLEEP_AWAKE) {
    g_logger.log(yal::Level::DEBUG, "woke up from deep sleep");

    if (!open_heat::rtc::read().drdDisabled) {
      g_drd.stop();
      open_heat::rtc::setDrdDisabled(true);
    }
  }

  const auto offsetMillis = open_heat::rtc::offsetMillis();
  g_logger.log(yal::Level::DEBUG, "Set last reset time to %", offsetMillis);
  open_heat::rtc::setLastResetTime(offsetMillis);

  setupPins();
  setupTemperatureSensor();

  const auto doubleReset = isDoubleReset();

  g_valve.setup();

  if (g_mqtt.needLoop() || doubleReset) {
    g_wifiManager.setup(doubleReset);
    g_mqtt.setup();
  }
  g_mqtt.enableDebug(true);

  if (open_heat::rtc::read().debug) {
    g_webServer.setup(g_config.value<String>("wifi_hostname"));
  }

  g_drd.stop();
  g_logger.log(yal::Level::INFO, "Device startup and setup done");

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
  g_logger.log(yal::Level::DEBUG, msg.c_str());

  const auto minSleepTime = 10000UL;
  uint32_t idleTime;
  uint32_t nextCheckMillis;
  bool enableWifi = false;
  if (valveSleep < mqttSleep) {
    nextCheckMillis = valveSleep;
  } else {
    nextCheckMillis = mqttSleep;
    enableWifi = true;
  }

  if (nextCheckMillis < (open_heat::rtc::offsetMillis() + minSleepTime)) {
    g_logger.log(
      yal::Level::DEBUG,
      "Minimal sleep or underflow prevented, sleep set to %ul ms",
      minSleepTime);
    idleTime = minSleepTime;
  } else {
    idleTime = nextCheckMillis - open_heat::rtc::offsetMillis();
  }

  // Wait before forcing sleep to send messages.
  delay(50);
  open_heat::rtc::wifiDeepSleep(idleTime, enableWifi, g_config);
}
