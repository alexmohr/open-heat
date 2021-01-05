//
// Copyright (c) 2020 Alexander Mohr
// Open-Heat - Radiator control for ESP8266
// Licensed under the terms of the MIT license
//

#include <Arduino.h>
#include <Logger.hpp>

//#include "Filesystem.hpp"
#include "Filesystem.hpp"
#include "network/WifiManager.hpp"


AsyncWebServer webServer_(80);
DNSServer dnsServer_;
WIFI_MULTI wifiMulti_ = WIFI_MULTI();

DoubleResetDetector drd_(DRD_TIMEOUT, DRD_ADDRESS);

static constexpr std::chrono::milliseconds wifiCheckInterval(5000);

open_heat::Filesystem filesystem_;
open_heat::network::WifiManager wifiManager_(
  wifiCheckInterval,
  &filesystem_,
  &webServer_,
  &dnsServer_,
  &wifiMulti_,
  &drd_);


void waitForSerialPort()
{
  while (!Serial) {
    delay(200);
  }
  Serial.println("");
  open_heat::Logger::log(open_heat::Logger::DEBUG, "Serial port ready");
}

void setup()
{
  // set led pin as output
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LED_OFF);

  open_heat::Logger::setup();
  open_heat::Logger::setLogLevel(open_heat::Logger::TRACE);

  Serial.begin(MONITOR_SPEED);
  waitForSerialPort();

  open_heat::Logger::log(open_heat::Logger::DEBUG, "Board: %s", ARDUINO_BOARD);
  open_heat::Logger::log(open_heat::Logger::DEBUG, "Build date: %s, %s",
                         __DATE__, __TIME__);
  open_heat::Logger::log(
    open_heat::Logger::DEBUG, "WifiManager Version: %s", ESP_ASYNC_WIFIMANAGER_VERSION);

  filesystem_.setup();
  auto &config = filesystem_.getConfig();
  open_heat::Logger::log(open_heat::Logger::DEBUG,
                         "MQTT connection: %s:%i",
                         config.MqttServer,
                         config.MqttPort);
  wifiManager_.setup();
}

void rotateMotor()
{
  //  digitalWrite(open_heat::pins::DialEncoderA, HIGH);
  //  digitalWrite(open_heat::pins::DialEncoderB, LOW);
  //  delay(2000);
  //  digitalWrite(open_heat::pins::DialEncoderB, HIGH);
  //  digitalWrite(open_heat::pins::DialEncoderA, LOW);
  //  delay(2000);
}

void loop()
{
  // Call the double reset detector loop method every so often,
  // so that it can recognise when the timeout expires.
  // You can also call drd.stop() when you wish to no longer
  // consider the next reset as a double reset.
  drd_.loop();
}
