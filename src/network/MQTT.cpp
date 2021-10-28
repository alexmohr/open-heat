//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#include "MQTT.hpp"
#include <RTCMemory.hpp>
#include <cstring>

open_heat::heating::RadiatorValve* open_heat::network::MQTT::valve_;
open_heat::Filesystem* open_heat::network::MQTT::filesystem_;
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

void open_heat::network::MQTT::setup()
{
  Logger::log(Logger::INFO, "Running MQTT setup");
  mqttClient_.onMessage(&MQTT::messageReceivedCallback);

  valve_->registerModeChangedHandler([this](OperationMode mode) {
    publish(getModeTopic_, heating::RadiatorValve::modeToCharArray(mode));
  });

  valve_->registerSetTempChangedHandler(
    [this](float temp) { publish(getConfiguredTempTopic_, String(temp)); });

  valve_->registerWindowChangeHandler(
    [this](bool state) { publish(windowStateTopic_, String(state)); });

  if (!loggerAdded_) {
    // Logger::addPrinter([](const std::string& message) { mqttLogPrinter(message); });
    loggerAdded_ = true;
  }
}

bool open_heat::network::MQTT::needLoop()
{
  if (rtc::read().debug) {
    return true;
  }

  if (rtc::offsetMillis() < rtc::read().mqttNextCheckMillis) {
    return false;
  }
  return true;
}

unsigned long open_heat::network::MQTT::loop()
{
  if (!configValid_) {
    Logger::log(Logger::ERROR, "Config is not valid, no mqtt loop!");
    return 0UL;
  }

  if (!needLoop()) {
    return rtc::read().mqttNextCheckMillis;
  }

  wifi_.checkWifi();
  if (!mqttClient_.connected()) {
    connect();
  }

  mqttClient_.loop();

  publish(getMeasuredTempTopic_, String(tempSensor_.getTemperature()));
  publish(getMeasuredHumidTopic_, String(tempSensor_.getHumidity()));
  // publish(getConfiguredTempTopic_, String(valve_->getConfiguredTemp()));
  // publish(getModeTopic_, heating::RadiatorValve::modeToCharArray(valve_->getMode()));

  rtc::setMqttNextCheckMillis(rtc::offsetMillis() + checkIntervalMillis_);

  return rtc::read().mqttNextCheckMillis;
}

void open_heat::network::MQTT::messageReceivedCallback(String& topic, String& payload)
{
  /*Logger::log(
    Logger::DEBUG,
    "Received message in topic: %s, msg: '%s'",
    topic.c_str(),*
    payload.c_str());*/

  if (topic == setConfiguredTempTopic_) {
    handleSetConfigTemp(payload);
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
    // Logger::log(Logger::DEBUG, "Log level is invalid: %s", payload.c_str());
    return;
  }

  Logger::setLogLevel(static_cast<Logger::Level>(level));
}
void open_heat::network::MQTT::handleDebug(const String& payload)
{
  bool state = false;
  if (payload == "true") {
    state = true;
  }

  enableDebug(state);
}

void open_heat::network::MQTT::handleSetMode(const String& payload)
{
  OperationMode mode;
  if (payload == "heat") {
    mode = HEAT;
  } else if (payload == "off") {
    mode = OFF;
  } else {
    // Logger::log(Logger::WARNING, "Mode %s not supported", payload.c_str());
    return;
  }
  valve_->setMode(mode);
}

void open_heat::network::MQTT::handleSetConfigTemp(const String& payload)
{
  const auto newTemp = static_cast<float>(strtod(payload.c_str(), nullptr));
  valve_->setConfiguredTemp(newTemp);
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
  const auto& config = filesystem_->getConfig();
  if ((std::strlen(config.MQTT.Server) == 0 || config.MQTT.Port == 0)) {
    if (configValid_) {
      Logger::log(
        Logger::ERROR,
        "MQTT Server (%s) or port (%i) not set up",
        config.MQTT.Server,
        config.MQTT.Port);
      enableDebug(true);
    }

    configValid_ = false;
    return;
  }

  mqttClient_.setTimeout(
    static_cast<int>(std::chrono::milliseconds(std::chrono::seconds(1)).count()));

  mqttClient_.begin(config.MQTT.Server, config.MQTT.Port, wiFiClient_);
  // Large timeout to allow large sleeps
  const char* username = nullptr;
  const char* password = nullptr;

  if (std::strlen(config.MQTT.Username) > 0) {
    username = config.MQTT.Username;
  }

  if (std::strlen(config.MQTT.Password) > 0) {
    password = config.MQTT.Password;
  }

  if (!mqttClient_.connect(config.Hostname, username, password)) {
    Logger::log(
      Logger::ERROR,
      "Failed to connect to mqtt server host %s, user: %s, pw: %s",
      config.MQTT.Server,
      config.MQTT.Username,
      config.MQTT.Password);
    return;
  }

  Logger::log(
    Logger::INFO,
    "MQTT topic: %s, topic len: %i",
    config.MQTT.Topic,
    std::strlen(config.MQTT.Topic));

  logTopic_ = config.MQTT.Topic;
  logTopic_ += "log";

  setConfiguredTempTopic_ = config.MQTT.Topic;
  setConfiguredTempTopic_ += "temperature/target/set";
  subscribe(setConfiguredTempTopic_);

  getConfiguredTempTopic_ = config.MQTT.Topic;
  getConfiguredTempTopic_ += "temperature/target/get";

  getMeasuredTempTopic_ = config.MQTT.Topic;
  getMeasuredTempTopic_ += "temperature/measured/get";

  getMeasuredHumidTopic_ = config.MQTT.Topic;
  getMeasuredHumidTopic_ += "humidity/measured/get";

  getModeTopic_ = config.MQTT.Topic;
  getModeTopic_ += "mode/get";

  setModeTopic_ = config.MQTT.Topic;
  setModeTopic_ += "mode/set";
  subscribe(setModeTopic_);

  debugEnableTopic_ = config.MQTT.Topic;
  debugEnableTopic_ += "debug/enable";
  subscribe(debugEnableTopic_);

  if (!DISABLE_ALL_LOGGING) {
    debugLogLevel_ = config.MQTT.Topic;
    debugLogLevel_ += "debug/loglevel";
    subscribe(debugLogLevel_);
  }

  if (config.WindowPins.Ground > 0 && config.WindowPins.Vin > 0) {
    windowStateTopic_ = config.MQTT.Topic;
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
    Logger::log(
      Logger::ERROR, "MQTT last error: %i", static_cast<int>(mqttClient_.lastError()));
  }
}

void open_heat::network::MQTT::mqttLogPrinter(const std::string& message)
{
  mqttClient_.publish(logTopic_, message.c_str());
}

void open_heat::network::MQTT::enableDebug(bool value)
{
  if (rtc::read().debug == value) {
    return;
  }

  rtc::setDebug(value);
  open_heat::rtc::wifiDeepSleep(1, value, *filesystem_);
}
