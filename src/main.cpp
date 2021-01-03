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

WM_Config wmConfig_{};
AsyncWebServer webServer_(80);
DNSServer dnsServer_;
WIFI_MULTI wifiMulti_ = WIFI_MULTI();

DoubleResetDetector drd_(DRD_TIMEOUT, DRD_ADDRESS);


static constexpr std::chrono::milliseconds wifiCheckInterval(5000);

open_heat::network::WifiManager wifiManager_(wifiCheckInterval, &webServer_,
                                             &dnsServer_, &wmConfig_,
                                             &wifiMulti_, &drd_);
open_heat::Filesystem filesystem_;

void waitForSerialPort() {
  while (!Serial) {
    delay(200);
  }
  Serial.println("");
  open_heat::Logger::log(open_heat::Logger::DEBUG, "Serial port ready");
}
bool initialConfig = true;

void setup() {
  open_heat::Logger::setup();
  open_heat::Logger::setLogLevel(open_heat::Logger::TRACE);

  Serial.begin(MONITOR_SPEED);
  waitForSerialPort();

  open_heat::Logger::log(open_heat::Logger::DEBUG, "Board: %s", ARDUINO_BOARD);
  open_heat::Logger::log(open_heat::Logger::DEBUG, "WifiManager Version: %s",
                               ESP_ASYNC_WIFIMANAGER_VERSION);

  // set led pin as output
  pinMode(LED_BUILTIN, OUTPUT);

  filesystem_.setup();
  wifiManager_.setup();
}

void rotateMotor() {
  //  digitalWrite(open_heat::pins::DialEncoderA, HIGH);
  //  digitalWrite(open_heat::pins::DialEncoderB, LOW);
  //  delay(2000);
  //  digitalWrite(open_heat::pins::DialEncoderB, HIGH);
  //  digitalWrite(open_heat::pins::DialEncoderA, LOW);
  //  delay(2000);
}

void loop() {}
