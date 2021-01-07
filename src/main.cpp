//
// Copyright (c) 2020 Alexander Mohr
// Open-Heat - Radiator control for ESP8266
// Licensed under the terms of the MIT license
//

#include <Arduino.h>
#include <Logger.hpp>

#include "Filesystem.hpp"
#include "network/WifiManager.hpp"

#include <network/MQTT.hpp>
#include <network/WebServer.hpp>
#include <sensors/BME280.hpp>
#include <sensors/ITemperatureSensor.hpp>
#include <sensors/TP100.hpp>

static constexpr std::chrono::milliseconds wifiCheckInterval(5000);


DNSServer dnsServer_;
DoubleResetDetector drd_(DRD_TIMEOUT, DRD_ADDRESS);

#if TEMP_SENSOR == BME280
open_heat::sensors::ITemperatureSensor* tempSensor_ = new open_heat::sensors::BME280();
#elif TEMP_SENSOR == TP100
open_heat::sensors::ITemperatureSensor* tempSensor_ = new open_heat::sensors::TP100();
#else
#error "Not supported temp sensor"
#endif

open_heat::Filesystem filesystem_;
open_heat::network::MQTT mqtt_(filesystem_);
open_heat::network::WebServer webServer_(filesystem_, *tempSensor_);
open_heat::network::WifiManager wifiManager_(
  wifiCheckInterval,
  &filesystem_,
  webServer_.getWebServer(),
  &dnsServer_,
  &drd_);

// open_heat::network::MQTT mqtt_(filesystem_);



void waitForSerialPort()
{
  while (!Serial) {
    delay(200);
  }
  Serial.println("");
  open_heat::Logger::log(open_heat::Logger::DEBUG, "Serial port ready");
}

void rotateMotor()
{
  open_heat::Logger::log(open_heat::Logger::INFO, "Testing motor");
  digitalWrite(D5, HIGH);
  digitalWrite(D6, LOW);
  delay(500);
  digitalWrite(D6, HIGH);
  digitalWrite(D5, LOW);
  delay(500);
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

  logVersions();

  filesystem_.setup();

  wifiManager_.setup();
  webServer_.setup();
  mqtt_.setup();

  tempSensor_->setup();

  open_heat::Logger::log(
    open_heat::Logger::INFO, "TEMP %f", tempSensor_->getTemperature());
}

bool x = false;
void loop()
{

  // Call the double reset detector loop method every so often,
  // so that it can recognise when the timeout expires.
  // You can also call drd.stop() when you wish to no longer
  // consider the next reset as a double reset.
  drd_.loop();
  wifiManager_.loop();
}
