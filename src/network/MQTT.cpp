//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#include "MQTT.hpp"
#include <RTCMemory.hpp>
#include <cstring>

open_heat::heating::RadiatorValve* open_heat::network::MQTT::valve_;
open_heat::sensors::Battery* open_heat::network::MQTT::battery_;
open_heat::Filesystem* open_heat::network::MQTT::filesystem_;
MQTTClient open_heat::network::MQTT::mqttClient_;

String open_heat::network::MQTT::getModeTopic_;
String open_heat::network::MQTT::setModeTopic_;

String open_heat::network::MQTT::setConfiguredTempTopic_;
String open_heat::network::MQTT::getConfiguredTempTopic_;
String open_heat::network::MQTT::getMeasuredTempTopic_;
String open_heat::network::MQTT::getMeasuredHumidTopic_;
String open_heat::network::MQTT::getBatteryTopic_;

String open_heat::network::MQTT::setModemSleepTopic_;
String open_heat::network::MQTT::getModemSleepTopic_;

String open_heat::network::MQTT::debugEnableTopic_;
String open_heat::network::MQTT::debugLogLevelTopic_;

String open_heat::network::MQTT::windowStateTopic_;

String open_heat::network::MQTT::logTopic_;
std::queue<open_heat::network::MQTT::message> open_heat::network::MQTT::m_messageQueue;

void open_heat::network::MQTT::setup()
{
  Logger::log(Logger::INFO, "Running MQTT setup");
  mqttClient_.onMessage(&MQTT::messageReceivedCallback);

  valve_->registerModeChangedHandler([this](OperationMode mode) {
    m_messageQueue.push({&getModeTopic_, heating::RadiatorValve::modeToCharArray(mode)});
  });

  valve_->registerSetTempChangedHandler([this](float temp) {
    m_messageQueue.push({&getConfiguredTempTopic_, String(temp)});
  });

  valve_->registerWindowChangeHandler([this](bool state) {
    m_messageQueue.push({&windowStateTopic_, String(state)});
  });

  if (!loggerAdded_) {
    Logger::addPrinter([this](const Logger::Level level, const std::string& message) {
      if (!configValid_) {
        return;
      }
      String buffer = Logger::levelToText(level, false);
      buffer += F(" ");
      buffer += message.c_str();
      m_messageQueue.push({&logTopic_, buffer});
    });
    loggerAdded_ = true;
  }
}

bool open_heat::network::MQTT::needLoop()
{
  if (rtc::offsetMillis() < rtc::read().mqttNextCheckMillis) {
    return false;
  }
  return true;
}

uint64_t open_heat::network::MQTT::loop()
{
  if (!configValid_) {
    Logger::log(Logger::ERROR, "Config is not valid, no mqtt loop!");
    enableDebug(true);
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

  // drain message queue for old messages
  sendMessageQueue();

  publish(getMeasuredTempTopic_, String(tempSensor_.getTemperature()));
  publish(getMeasuredHumidTopic_, String(tempSensor_.getHumidity()));
  publish(getModemSleepTopic_, String(rtc::read().modemSleepTime));

  battery_->loop();
  publish(getBatteryTopic_ + "percent", String(battery_->percentage()));
  publish(getBatteryTopic_ + "voltage", String(battery_->voltage()));

  // drain message queue for new messages
  sendMessageQueue();

  rtc::setMqttNextCheckMillis(rtc::offsetMillis() + rtc::read().modemSleepTime);

  mqttClient_.loop();

  return rtc::read().mqttNextCheckMillis;
}

void open_heat::network::MQTT::messageReceivedCallback(String& topic, String& payload)
{
  // todo this log statement breaks mqtt logger with floating point numbers
  /*
  Logger::log(
    Logger::INFO,
    "Received message in topic '%s', payload '%s'",
    topic.c_str(),
    payload.c_str());
    */

  if (payload.isEmpty()) {
    return;
  }

  if (topic == setConfiguredTempTopic_) {
    handleSetConfigTemp(payload);
  } else if (topic == setModeTopic_) {
    handleSetMode(payload);
  } else if (topic == setModemSleepTopic_) {
    handleSetModemSleep(payload);
  } else if (topic == debugEnableTopic_) {
    handleDebug(payload);
  } else if (topic == debugLogLevelTopic_) {
    handleLogLevel(payload);
  }
}
void open_heat::network::MQTT::handleLogLevel(const String& payload)
{
  std::stringstream ss(payload.c_str());
  int level;
  if (!(ss >> level)) {
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
    Logger::log(Logger::WARNING, "Mode %s not supported", payload.c_str());
    return;
  }

  valve_->setMode(mode);
}

void open_heat::network::MQTT::handleSetConfigTemp(const String& payload)
{
  const auto newTemp = static_cast<float>(std::strtod(payload.c_str(), nullptr));
  if (newTemp <= 0.0f) {
    return;
  }

  valve_->setConfiguredTemp(newTemp);
}

void open_heat::network::MQTT::handleSetModemSleep(const String& payload)
{
  const auto newTime = std::strtoul(payload.c_str(), nullptr, 10);
  if (newTime == 0) {
    return;
  }

  if (newTime == rtc::read().modemSleepTime) {
    return;
  }

  rtc::setModemSleepTime(newTime);
  Logger::log(Logger::INFO, "Set new modem sleep time %lu", newTime);
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

  setTopic(config.MQTT.Topic, "log", logTopic_);

  Logger::log(
    Logger::INFO,
    "MQTT topic: %s, topic len: %i",
    config.MQTT.Topic,
    std::strlen(config.MQTT.Topic));

  setTopic(config.MQTT.Topic, "temperature/target/get", getConfiguredTempTopic_);
  setTopic(config.MQTT.Topic, "temperature/target/set", setConfiguredTempTopic_);
  setTopic(config.MQTT.Topic, "temperature/measured/get", getMeasuredTempTopic_);
  setTopic(config.MQTT.Topic, "humidity/measured/get", getMeasuredHumidTopic_);
  setTopic(config.MQTT.Topic, "modemsleep/set", setModemSleepTopic_);
  setTopic(config.MQTT.Topic, "modemsleep/get", getModemSleepTopic_);
  setTopic(config.MQTT.Topic, "battery/", getBatteryTopic_);
  setTopic(config.MQTT.Topic, "mode/get", getModeTopic_);
  setTopic(config.MQTT.Topic, "mode/set", setModeTopic_);
  setTopic(config.MQTT.Topic, "debug/enable", debugEnableTopic_);

  subscribe(debugEnableTopic_);
  subscribe(setModeTopic_);
  subscribe(setModemSleepTopic_);
  subscribe(setConfiguredTempTopic_);

  if (!DISABLE_ALL_LOGGING) {
    setTopic(config.MQTT.Topic, "debug/loglevel", debugLogLevelTopic_);
    subscribe(debugLogLevelTopic_);
  }

  if (config.WindowPins.Ground > 0 && config.WindowPins.Vin > 0) {
    setTopic(config.MQTT.Topic, "window/get", windowStateTopic_);
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

void open_heat::network::MQTT::enableDebug(bool value)
{
  if (rtc::read().debug == value) {
    return;
  }

  rtc::setDebug(value);
  open_heat::rtc::wifiDeepSleep(1, value, *filesystem_);
}

void open_heat::network::MQTT::sendMessageQueue()
{
  while (!m_messageQueue.empty()) {
    const auto msg = m_messageQueue.front();
    if (*msg.topic == logTopic_) {
      // do not log again
      mqttClient_.publish(*msg.topic, msg.message);
    } else {
      publish(*msg.topic, msg.message);
    }

    m_messageQueue.pop();
  }
}

void open_heat::network::MQTT::setTopic(
  const String& baseTopic,
  const String& subTopic,
  String& out)
{
  out = baseTopic;
  out += subTopic;
}
