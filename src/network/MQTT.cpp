//
// Copyright (c) 2020 Alexander Mohr
// Licensed under the terms of the MIT license
//
#include "MQTT.hpp"
#include <Logger.hpp>

 Config* open_heat::network::MQTT::config_;
open_heat::heating::RadiatorValve* open_heat::network::MQTT::valve_;
MQTTClient open_heat::network::MQTT::mqttClient_;

String open_heat::network::MQTT::getModeTopic_;
String open_heat::network::MQTT::setModeTopic_;

String open_heat::network::MQTT::setConfiguredTempTopic_;
String open_heat::network::MQTT::getConfiguredTempTopic_;
String open_heat::network::MQTT::getMeasuredTempTopic_;

void open_heat::network::MQTT::setup()
{
  auto& config = filesystem_.getConfig();
  if (strlen(config.MQTT.Server) == 0 || config.MQTT.Port == 0) {
    Logger::log(Logger::DEBUG, "MQTT Server or port not set up");
  }

  mqttClient_.begin(config.MQTT.Server, config.MQTT.Port, wiFiClient_);
  const char* username = nullptr;
  const char* password = nullptr;

  if (strlen(config.MQTT.Username) > 0) {
    username = config.MQTT.Username;
  }

  if (strlen(config.MQTT.Password) > 0) {
    password = config.MQTT.Password;
  }

  if (!mqttClient_.connect(config.Hostname, username, password)) {
    Logger::log(
      Logger::DEBUG,
      "Failed to connect to mqtt in setup, check your config: server %s:%i",
      config.MQTT.Server,
      config.MQTT.Port);
    return;
  }

  setConfiguredTempTopic_ = config_->MQTT.Topic;
  setConfiguredTempTopic_ += "temperature/target/set";
  mqttClient_.subscribe(setConfiguredTempTopic_);
  Logger::log(
    Logger::INFO, "MQTT subscribed to topic: %s", setConfiguredTempTopic_.c_str());

  getConfiguredTempTopic_ = config_->MQTT.Topic;
  getConfiguredTempTopic_ += "temperature/target/get";

  getMeasuredTempTopic_ = config_->MQTT.Topic;
  getMeasuredTempTopic_ += "temperature/measured/get";

  getModeTopic_ = config_->MQTT.Topic;
  getModeTopic_ += "mode/get";

  setModeTopic_ = config_->MQTT.Topic;
  setModeTopic_ += "mode/set";
  mqttClient_.subscribe(setModeTopic_);
  Logger::log(
    Logger::INFO, "MQTT subscribed to topic: %s", setModeTopic_.c_str());


  mqttClient_.onMessage(&MQTT::messageReceivedCallback);
}

void open_heat::network::MQTT::loop()
{
  mqttClient_.loop();

  if (!mqttClient_.connected()) {
    setup();
  }

  if (millis() < nextCheckMillis_) {
    return;
  }

  const auto& config = filesystem_.getConfig();

  publish(getMeasuredTempTopic_, String(tempSensor_.getTemperature()));
  publish(getConfiguredTempTopic_, String(valve_->getConfiguredTemp()));
  publish(getModeTopic_, "heating");
  nextCheckMillis_ = millis() + checkIntervalMillis_;
}

void open_heat::network::MQTT::messageReceivedCallback(String& topic, String& payload)
{
  Logger::log(
    Logger::DEBUG,
    "Received message in topic: %s, msg: %s",
    topic.c_str(),
    payload.c_str());


  if (topic == setConfiguredTempTopic_) {
    auto newTemp = std::strtod(payload.c_str(), nullptr);
    valve_->setConfiguredTemp(newTemp);
    publish(getConfiguredTempTopic_, String(newTemp));
  } else if (topic == getConfiguredTempTopic_) {
    auto setTemp = valve_->getConfiguredTemp();
    publish(getConfiguredTempTopic_, String(setTemp));
  }
}

void open_heat::network::MQTT::publish(const String& topic, const String& message)
{
  Logger::log(
    Logger::DEBUG, "MQTT send '%s' in topic '%s'", message.c_str(), topic.c_str());
  if (!mqttClient_.publish(topic, message)) {
    Logger::log(Logger::ERROR, "Mqtt publish failed: %i", mqttClient_.lastError());
  }
}
