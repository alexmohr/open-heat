//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#include "MQTT.hpp"
#include <RTCMemory.hpp>
#include <cstring>

Config* open_heat::network::MQTT::config_;
open_heat::heating::RadiatorValve* open_heat::network::MQTT::valve_;
MQTTClient open_heat::network::MQTT::mqttClient_;

String open_heat::network::MQTT::getModeTopic_;
String open_heat::network::MQTT::setModeTopic_;

String open_heat::network::MQTT::setConfiguredTempTopic_;
String open_heat::network::MQTT::getConfiguredTempTopic_;
String open_heat::network::MQTT::getMeasuredTempTopic_;
String open_heat::network::MQTT::getMeasuredHumidTopic_;

String open_heat::network::MQTT::debugEnableTopic_;
String open_heat::network::MQTT::debugLogLevel_;

String open_heat::network::MQTT::windowStateTopic_;

String open_heat::network::MQTT::logTopic_;

bool open_heat::network::MQTT::debugEnabled_ = false;

void open_heat::network::MQTT::setup()
{
  Logger::log(Logger::INFO, "Running MQTT setup");
  mqttClient_.onMessage(&MQTT::messageReceivedCallback);

  connect();

  valve_->registerModeChangedHandler([this](OperationMode mode) {
    mqttClient_.publish(getModeTopic_, heating::RadiatorValve::modeToCharArray(mode));
  });

  valve_->registerSetTempChangedHandler(
    [this](float temp) { mqttClient_.publish(getConfiguredTempTopic_, String(temp)); });

  valve_->registerWindowChangeHandler(
    [this](bool state) { mqttClient_.publish(windowStateTopic_, String(state)); });

  if (!loggerAdded_) {
    Logger::addPrinter([](const std::string& message) { mqttLogPrinter(message);
    });
    loggerAdded_ = true;
  }
}

bool open_heat::network::MQTT::needLoop()
{
  auto rtcMem = readRTCMemory();
  if (offsetMillis() < rtcMem.mqttNextCheckMillis) {
    return false;
  }
  return true;
}

unsigned long open_heat::network::MQTT::loop()
{
  auto rtcMem = readRTCMemory();
  if (!needLoop()) {
    return rtcMem.mqttNextCheckMillis;
  }

  wifi_.checkWifi();
  if (!mqttClient_.connected()) {
    connect();
  }

  mqttClient_.loop();

  publish(getMeasuredTempTopic_, String(tempSensor_.getTemperature()));
  // publish(getMeasuredHumidTopic_, String(tempSensor_.getHumidity()));
  publish(getConfiguredTempTopic_, String(valve_->getConfiguredTemp()));
  publish(getModeTopic_, heating::RadiatorValve::modeToCharArray(valve_->getMode()));

  rtcMem.mqttNextCheckMillis = offsetMillis() + checkIntervalMillis_;

  writeRTCMemory(rtcMem);
  return rtcMem.mqttNextCheckMillis;
}

void open_heat::network::MQTT::messageReceivedCallback(String& topic, String& payload)
{
  Logger::log(
    Logger::DEBUG,
    "Received message in topic: %s, msg: '%s'",
    topic.c_str(),
    payload.c_str());

  if (topic == setConfiguredTempTopic_) {
    handleSetConfigTemp(payload);
  } else if (topic == getConfiguredTempTopic_) {
    handleGetConfigTemp();
  } else if (topic == setModeTopic_) {
    handleSetMode(payload);
  } else if (topic == debugEnableTopic_) {
    handleDebug(payload);
  } else if (topic == debugLogLevel_) {
    handleLogLevel(payload);
  }
}
void open_heat::network::MQTT::handleLogLevel(const String& payload)
{
  std::stringstream ss(payload.c_str());
  int level;
  if (!(ss >> level)) {
    Logger::log(Logger::DEBUG, "Log level is invalid: %s", payload.c_str());
    return;
  }

  Logger::setLogLevel(static_cast<Logger::Level>(level));
}
void open_heat::network::MQTT::handleDebug(const String& payload)
{
  if (payload == "true") {
    debugEnabled_ = true;
  } else if (payload == "false") {
    debugEnabled_ = false;
  }
}

void open_heat::network::MQTT::handleSetMode(const String& payload)
{
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

void open_heat::network::MQTT::handleGetConfigTemp()
{
  const auto setTemp = valve_->getConfiguredTemp();
  publish(getConfiguredTempTopic_, String(setTemp));
}
void open_heat::network::MQTT::handleSetConfigTemp(const String& payload)
{
  const auto newTemp = static_cast<float>(strtod(payload.c_str(), nullptr));
  valve_->setConfiguredTemp(newTemp);
  publish(getConfiguredTempTopic_, String(newTemp));
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
  if ((std::strlen(config_->MQTT.Server) == 0 || config_->MQTT.Port == 0)) {
    if (configValid_) {
      Logger::log(Logger::ERROR, "MQTT Server or port not set up");
      enableDebug();
    }

    configValid_ = false;
    return;
  }

  mqttClient_.setTimeout(
    static_cast<int>(std::chrono::milliseconds(std::chrono::minutes(3)).count()));
  mqttClient_.begin(config_->MQTT.Server, config_->MQTT.Port, wiFiClient_);
  // Large timeout to allow large sleeps
  const char* username = nullptr;
  const char* password = nullptr;

  if (std::strlen(config_->MQTT.Username) > 0) {
    username = config_->MQTT.Username;
  }

  if (std::strlen(config_->MQTT.Password) > 0) {
    password = config_->MQTT.Password;
  }

  if (!mqttClient_.connect(config_->Hostname, username, password)) {
    Logger::log(
      Logger::ERROR,
      "Failed to connect to mqtt server host %s, user: %s, pw: %s",
      config_->MQTT.Server,
      config_->MQTT.Username,
      config_->MQTT.Password);
    return;
  }

  Logger::log(
    Logger::INFO,
    "MQTT topic: %s, topic len: %i",
    config_->MQTT.Topic,
    std::strlen(config_->MQTT.Topic));

  logTopic_ = config_->MQTT.Topic;
  logTopic_ += "log";

  setConfiguredTempTopic_ = config_->MQTT.Topic;
  setConfiguredTempTopic_ += "temperature/target/set";
  mqttClient_.subscribe(setConfiguredTempTopic_);
  Logger::log(
    Logger::INFO, "MQTT subscribed to topic: %s", setConfiguredTempTopic_.c_str());

  getConfiguredTempTopic_ = config_->MQTT.Topic;
  getConfiguredTempTopic_ += "temperature/target/get";

  getMeasuredTempTopic_ = config_->MQTT.Topic;
  getMeasuredTempTopic_ += "temperature/measured/get";

  getMeasuredHumidTopic_ = config_->MQTT.Topic;
  getMeasuredHumidTopic_ += "humidity/measured/get";

  getModeTopic_ = config_->MQTT.Topic;
  getModeTopic_ += "mode/get";

  setModeTopic_ = config_->MQTT.Topic;
  setModeTopic_ += "mode/set";
  subscribe(setModeTopic_);

  if (!DISABLE_ALL_LOGGING) {
    debugEnableTopic_ = config_->MQTT.Topic;
    debugEnableTopic_ += "debug/enable";
    subscribe(debugEnableTopic_);

    debugLogLevel_ = config_->MQTT.Topic;
    debugLogLevel_ += "debug/loglevel";
    subscribe(debugLogLevel_);
  }

  if (config_->WindowPins.Ground > 0 && config_->WindowPins.Vin > 0) {
      windowStateTopic_ = config_->MQTT.Topic;
    windowStateTopic_ += "window/get";
    subscribe(windowStateTopic_);
  }
}

void open_heat::network::MQTT::subscribe(const String& topic)
{
  if (mqttClient_.subscribe(topic)) {
    Logger::log(Logger::INFO, "MQTT subscribed to topic: %s", topic.c_str());
  } else {
    Logger::log(Logger::ERROR, "MQTT failed to subscribe to topic: %s", topic.c_str());
  }
}

void open_heat::network::MQTT::mqttLogPrinter(const std::string& message)
{
  mqttClient_.publish(logTopic_, message.c_str());
}

bool open_heat::network::MQTT::debug()
{
  return debugEnabled_;
}

void open_heat::network::MQTT::enableDebug()
{
  debugEnabled_ = true;
}
