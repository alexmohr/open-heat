//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
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

String open_heat::network::MQTT::windowStateTopic_;

String open_heat::network::MQTT::logTopic_;

String logBuffer_;

void open_heat::network::MQTT::setup()
{
  connect();

  mqttClient_.onMessage(&MQTT::messageReceivedCallback);

  valve_->registerModeChangedHandler([this](OperationMode mode){
    mqttClient_.publish(getModeTopic_, heating::RadiatorValve::modeToCharArray(mode));
  });

  valve_->registerSetTempChangedHandler([this](float temp){
    mqttClient_.publish(getConfiguredTempTopic_, String(temp));
  });

  valve_->registerWindowChangeHandler([this](bool state){
    mqttClient_.publish(windowStateTopic_, String(state));
  });

  Logger::addPrinter(
    [this](Logger::Level level, const char* module, const char* message) {
      mqttLogPrinter(level, module, message);
    });
}

unsigned long open_heat::network::MQTT::loop()
{
  wifi_.checkWifi();

  mqttClient_.loop();

  if (!mqttClient_.connected()) {
    setup();
  }

  if (millis() < nextCheckMillis_) {
    return nextCheckMillis_;
  }

  publish(getMeasuredTempTopic_, String(tempSensor_.getTemperature()));
  publish(getConfiguredTempTopic_, String(valve_->getConfiguredTemp()));
  publish(getModeTopic_, heating::RadiatorValve::modeToCharArray(valve_->getMode()));
  nextCheckMillis_ = millis() + checkIntervalMillis_;
  return nextCheckMillis_;
}

void open_heat::network::MQTT::messageReceivedCallback(String& topic, String& payload)
{
  Logger::log(
    Logger::DEBUG,
    "Received message in topic: %s, msg: %s",
    topic.c_str(),
    payload.c_str());

  if (topic == setConfiguredTempTopic_) {
    auto newTemp = static_cast<float>(std::strtod(payload.c_str(), nullptr));
    valve_->setConfiguredTemp(newTemp);
    publish(getConfiguredTempTopic_, String(newTemp));
  } else if (topic == getConfiguredTempTopic_) {
    auto setTemp = valve_->getConfiguredTemp();
    publish(getConfiguredTempTopic_, String(setTemp));
  } else if (topic == setModeTopic_) {
    OperationMode mode;
    if (payload == "heat") {
      mode = HEAT;
    } else if (payload == "off") {
      mode = OFF;
    } else {
      Logger::log(Logger::WARNING, "Mode %s not supported", payload.c_str());
      return;
    }
    valve_->setMode(mode);
    publish(getModeTopic_, payload);
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

void open_heat::network::MQTT::connect()
{
  if ((strlen(config_->MQTT.Server) == 0 || config_->MQTT.Port == 0 )){
    if (configValid_) {
        Logger::log(Logger::DEBUG, "MQTT Server or port not set up");
    }

    configValid_ = false;
    return;
  }

  mqttClient_.setTimeout(std::chrono::milliseconds(std::chrono::minutes(3)).count());
  mqttClient_.begin(config_->MQTT.Server, config_->MQTT.Port, wiFiClient_);
  // Large timeout to allow large sleeps
  const char* username = nullptr;
  const char* password = nullptr;

  if (strlen(config_->MQTT.Username) > 0) {
    username = config_->MQTT.Username;
  }

  if (strlen(config_->MQTT.Password) > 0) {
    password = config_->MQTT.Password;
  }

  if (!mqttClient_.connect(config_->Hostname, username, password)) {
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
  Logger::log(Logger::INFO, "MQTT subscribed to topic: %s", setModeTopic_.c_str());

  windowStateTopic_ = config_->MQTT.Topic;
  windowStateTopic_ += "window/get";
  mqttClient_.subscribe(windowStateTopic_);
  Logger::log(Logger::INFO, "MQTT subscribed to topic: %s", windowStateTopic_.c_str());

  logTopic_ = config_->MQTT.Topic;
  logTopic_ += "log";
}

void open_heat::network::MQTT::mqttLogPrinter(
  open_heat::Logger::Level level,
  const char* module,
  const char* message)
{
  logBuffer_ = LOG_LEVEL_STRINGS[level];
  logBuffer_ += F(" ");

  if (strlen(module) > 0) {
    logBuffer_ += F(": ");
    logBuffer_ += module;
    logBuffer_ += F(": ");
  }

  logBuffer_ += message;
  // do not use publish method, because it also logs.
  mqttClient_.publish(logTopic_, logBuffer_);
}
