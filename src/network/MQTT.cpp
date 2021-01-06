//
// Copyright (c) 2020 Alexander Mohr
// Licensed under the terms of the MIT license
//
#include "MQTT.hpp"
#include <Logger.hpp>

open_heat::network::MQTT* open_heat::network::MQTT::instance_;

void open_heat::network::MQTT::setup()
{
  auto& config = filesystem_.getConfig();
  open_heat::Logger::log(
    open_heat::Logger::DEBUG,
    "MQTT connection: %s:%i",
    config.MqttServer,
    config.MqttPort);


  mqttClient_.begin(config.MqttServer, config.MqttPort, wiFiClient_);
  const char* username = nullptr;
  const char* password = nullptr;

  if (strlen(config.MqttUsername) > 0) {
    username = config.MqttUsername;
  }

  if (strlen(config.MqttPassword) > 0) {
    password = config.MqttPassword;
  }

  static constexpr std::chrono::milliseconds mqttConnectTimeout(3000);
  unsigned long startMillis = millis();
  unsigned long stopMillis = startMillis + mqttConnectTimeout.count();
  while (!mqttClient_.connect("open_heat", username, password) && stopMillis < millis()) {
    delay(200);
  }

  if (!mqttClient_.connected()) {
    Logger::log(
      Logger::DEBUG,
      "Failed to connect to mqtt in setup, check your config: server %s:%i",
      config.MqttServer,
      config.MqttPort);
    return;
  }

  mqttClient_.subscribe(config.MqttTopic);
  //mqttClient_.onMessage(&MQTT::messageReceivedCallback);

  Logger::log(Logger::DEBUG, "MQTT connected, subscribed and callback ready");
}

void open_heat::network::MQTT::loop()
{
}

void open_heat::network::MQTT::messageReceivedCallback(String& topic, String& payload)
{
  Logger::log(
    Logger::INFO,
    "Received message in topic: %s, msg: %s",
    topic.c_str(),
    payload.c_str());
}
