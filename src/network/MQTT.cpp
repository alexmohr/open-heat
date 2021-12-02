//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#include "MQTT.hpp"
#include <RTCMemory.hpp>
#include <cstring>

open_heat::heating::RadiatorValve* open_heat::network::MQTT::m_valve;
open_heat::sensors::Battery* open_heat::network::MQTT::m_battery;
open_heat::Filesystem* open_heat::network::MQTT::m_filesystem;
MQTTClient open_heat::network::MQTT::m_mqttClient;

String open_heat::network::MQTT::m_getModeTopic;
String open_heat::network::MQTT::m_setModeTopic;

String open_heat::network::MQTT::m_setConfiguredTempTopic;
String open_heat::network::MQTT::m_getConfiguredTempTopic;
String open_heat::network::MQTT::m_getMeasuredTempTopic;
String open_heat::network::MQTT::m_getMeasuredHumidTopic;
String open_heat::network::MQTT::m_getBatteryTopic;

String open_heat::network::MQTT::m_setModemSleepTopic;
String open_heat::network::MQTT::m_getModemSleepTopic;

String open_heat::network::MQTT::m_debugEnableTopic;
String open_heat::network::MQTT::m_debugLogLevelTopic;

String open_heat::network::MQTT::m_windowStateTopic;

String open_heat::network::MQTT::m_logTopic;
std::queue<open_heat::network::MQTT::message> open_heat::network::MQTT::m_messageQueue;

void open_heat::network::MQTT::setup()
{
  Logger::log(Logger::INFO, "Running MQTT setup");
  m_mqttClient.onMessage(&MQTT::messageReceivedCallback);

  m_valve->registerModeChangedHandler([this](OperationMode mode) {
    m_messageQueue.push({&m_getModeTopic, heating::RadiatorValve::modeToCharArray(mode)});
  });

  m_valve->registerSetTempChangedHandler([this](float temp) {
    m_messageQueue.push({&m_getConfiguredTempTopic, String(temp)});
  });

  m_valve->registerWindowChangeHandler([this](bool state) {
    m_messageQueue.push({&m_windowStateTopic, String(state)});
  });

  if (!m_loggerAdded) {
    Logger::addPrinter([this](const Logger::Level level, const std::string& message) {
      if (!m_configValid) {
        return;
      }
      String buffer = Logger::levelToText(level, false);
      buffer += F(" ");
      buffer += message.c_str();
      m_messageQueue.push({&m_logTopic, buffer});
    });
    m_loggerAdded = true;
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
  if (!m_configValid) {
    Logger::log(Logger::ERROR, "Config is not valid, no mqtt loop!");
    enableDebug(true);
    return 0UL;
  }

  if (!needLoop()) {
    return rtc::read().mqttNextCheckMillis;
  }

  m_wifi.checkWifi();
  if (!m_mqttClient.connected()) {
    connect();
  }

  m_mqttClient.loop();

  // drain message queue for old messages
  sendMessageQueue();

  if (m_humiditySensor != nullptr) {
    publish(m_getMeasuredHumidTopic, String(m_humiditySensor->humidity()));
  }
  if (m_tempSensor != nullptr) {
    publish(m_getMeasuredTempTopic, String(m_tempSensor->temperature()));
  }

  publish(m_getModemSleepTopic, String(rtc::read().modemSleepTime));

  m_battery->loop();
  publish(m_getBatteryTopic + "percent", String(m_battery->percentage()));
  publish(m_getBatteryTopic + "voltage", String(m_battery->voltage()));

  publish(m_getConfiguredTempTopic, String(rtc::read().setTemp));
  publish(m_getModeTopic, String(rtc::read().mode));


  // drain message queue for new messages
  sendMessageQueue();

  rtc::setMqttNextCheckMillis(rtc::offsetMillis() + rtc::read().modemSleepTime);

  m_mqttClient.loop();

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

  if (topic == m_setConfiguredTempTopic) {
    handleSetConfigTemp(payload);
  } else if (topic == m_setModeTopic) {
    handleSetMode(payload);
  } else if (topic == m_setModemSleepTopic) {
    handleSetModemSleep(payload);
  } else if (topic == m_debugEnableTopic) {
    handleDebug(payload);
  } else if (topic == m_debugLogLevelTopic) {
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

  m_valve->setMode(mode);
}

void open_heat::network::MQTT::handleSetConfigTemp(const String& payload)
{
  const auto newTemp = static_cast<float>(std::strtod(payload.c_str(), nullptr));
  if (newTemp <= 0.0f) {
    return;
  }

  m_valve->setConfiguredTemp(newTemp);
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
  if (!m_mqttClient.publish(topic, message)) {
    Logger::log(Logger::ERROR, "Mqtt publish failed: %i", m_mqttClient.lastError());
  }
}

void open_heat::network::MQTT::connect()
{
  const auto& config = m_filesystem->getConfig();
  if ((std::strlen(config.MQTT.Server) == 0 || config.MQTT.Port == 0)) {
    if (m_configValid) {
      Logger::log(
        Logger::ERROR,
        "MQTT Server (%s) or port (%i) not set up",
        config.MQTT.Server,
        config.MQTT.Port);
      enableDebug(true);
    }

    m_configValid = false;
    return;
  }

  m_mqttClient.setTimeout(
    static_cast<int>(std::chrono::milliseconds(std::chrono::seconds(1)).count()));

  m_mqttClient.begin(config.MQTT.Server, config.MQTT.Port, m_wifiClient);
  const char* username = nullptr;
  const char* password = nullptr;

  if (std::strlen(config.MQTT.Username) > 0) {
    username = config.MQTT.Username;
  }

  if (std::strlen(config.MQTT.Password) > 0) {
    password = config.MQTT.Password;
  }

  if (!m_mqttClient.connect(config.Hostname, username, password)) {
    Logger::log(
      Logger::ERROR,
      "Failed to connect to mqtt server host %s, user: %s, pw: %s",
      config.MQTT.Server,
      config.MQTT.Username,
      config.MQTT.Password);
    return;
  }

  setTopic(config.MQTT.Topic, "log", m_logTopic);

  Logger::log(
    Logger::INFO,
    "MQTT topic: %s, topic len: %i",
    config.MQTT.Topic,
    std::strlen(config.MQTT.Topic));

  setTopic(config.MQTT.Topic, "temperature/target/get", m_getConfiguredTempTopic);
  setTopic(config.MQTT.Topic, "temperature/target/set", m_setConfiguredTempTopic);
  setTopic(config.MQTT.Topic, "temperature/measured/get", m_getMeasuredTempTopic);
  setTopic(config.MQTT.Topic, "humidity/measured/get", m_getMeasuredHumidTopic);
  setTopic(config.MQTT.Topic, "modemsleep/set", m_setModemSleepTopic);
  setTopic(config.MQTT.Topic, "modemsleep/get", m_getModemSleepTopic);
  setTopic(config.MQTT.Topic, "battery/", m_getBatteryTopic);
  setTopic(config.MQTT.Topic, "mode/get", m_getModeTopic);
  setTopic(config.MQTT.Topic, "mode/set", m_setModeTopic);
  setTopic(config.MQTT.Topic, "debug/enable", m_debugEnableTopic);

  subscribe(m_debugEnableTopic);
  subscribe(m_setModeTopic);
  subscribe(m_setModemSleepTopic);
  subscribe(m_setConfiguredTempTopic);

  if (!DISABLE_ALL_LOGGING) {
    setTopic(config.MQTT.Topic, "debug/loglevel", m_debugLogLevelTopic);
    subscribe(m_debugLogLevelTopic);
  }

  if (config.WindowPins.Ground > 0 && config.WindowPins.Vin > 0) {
    setTopic(config.MQTT.Topic, "window/get", m_windowStateTopic);
    subscribe(m_windowStateTopic);
  }
}

void open_heat::network::MQTT::subscribe(const String& topic)
{
  if (m_mqttClient.subscribe(topic)) {
    Logger::log(Logger::INFO, "MQTT subscribed to topic: %s", topic.c_str());
  } else {
    Logger::log(Logger::ERROR, "MQTT failed to subscribe to topic: %s", topic.c_str());
    Logger::log(
      Logger::ERROR, "MQTT last error: %i", static_cast<int>(m_mqttClient.lastError()));
  }
}

void open_heat::network::MQTT::enableDebug(bool value)
{
  if (rtc::read().debug == value) {
    return;
  }

  rtc::setDebug(value);
  open_heat::rtc::wifiDeepSleep(1, value, *m_filesystem);
}

void open_heat::network::MQTT::sendMessageQueue()
{
  while (!m_messageQueue.empty()) {
    const auto msg = m_messageQueue.front();
    if (*msg.topic == m_logTopic) {
      // do not log again
      m_mqttClient.publish(*msg.topic, msg.message);
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
