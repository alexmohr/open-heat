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

// external voltage
ADC_MODE(ADC_TOUT);

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
open_heat::sensors::Battery battery_;

open_heat::network::WebServer webServer_(filesystem_, *tempSensor_, battery_, valve_);
open_heat::sensors::WindowSensor windowSensor_(&filesystem_, &valve_);

open_heat::Serial m_serial;

open_heat::network::WifiManager wifiManager_(&filesystem_, webServer_);

open_heat::network::MQTT mqtt_(
  &filesystem_,
  wifiManager_,
  *tempSensor_,
  &valve_,
  &battery_);

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

  const auto drdDetected = drd_.detectDoubleReset();

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
    m_serial.setup();
  }

  open_heat::Logger::setup();
  const auto configValid = filesystem_.setup();

  if (ESP.getResetInfoPtr()->reason == REASON_DEEP_SLEEP_AWAKE) {
    open_heat::Logger::log(open_heat::Logger::DEBUG, "woke up from deep sleep");

    if (!open_heat::rtc::read().drdDisabled) {
      drd_.stop();
      open_heat::rtc::setDrdDisabled(true);
    }

  } else {
    // system reset
    open_heat::rtc::init(filesystem_);
  }

  const auto offsetMillis = open_heat::rtc::offsetMillis();
  open_heat::Logger::log(
    open_heat::Logger::DEBUG, "Set last reset time to %lu", offsetMillis);
  open_heat::rtc::setLastResetTime(offsetMillis);

  setupPins();

  const auto doubleReset = isDoubleReset();

  if (!tempSensor_->setup()) {
    // blink led 10 times to indicate temp sensor error
    for (auto i = 0; i < 10; ++i) {
      digitalWrite(LED_PIN, LED_OFF);
      delay(250);
      digitalWrite(LED_PIN, LED_ON);
    }
    open_heat::Logger::log(open_heat::Logger::ERROR, "Failed to init temp sensor");
  }

  valve_.setup();

  if (mqtt_.needLoop() || doubleReset) {
    wifiManager_.setup(doubleReset);
    mqtt_.setup();
  }

  if (!configValid) {
    mqtt_.enableDebug(true);
  }

  if (open_heat::rtc::read().debug) {
    webServer_.setup(nullptr);
  }

  drd_.stop();
  open_heat::Logger::log(open_heat::Logger::INFO, "Device startup and setup done");

  loop();
}

void loop()
{
  // open_heat::sensors::WindowSensor::loop();
  const auto mqttSleep = mqtt_.loop();

  // must be after mqtt to process received commands
  const auto valveSleep = valve_.loop();
  drd_.loop();

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
  open_heat::rtc::wifiDeepSleep(idleTime, enableWifi, filesystem_);
}
