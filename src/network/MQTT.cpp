//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#include "MQTT.hpp"
#include <RTCMemory.hpp>
#include <cstring>

void open_heat::network::MQTT::setup()
{
  m_logger.log(yal::Level::INFO, "Running MQTT setup");
  m_mqttClient.onMessage(
    [this](String& topic, String& payload) { messageReceivedCallback(topic, payload); });

  m_valve.registerModeChangedHandler([this](OperationMode mode) {
    m_mqttAppender.queue().push(
      {m_getModeTopic, heating::RadiatorValve::modeToCharArray(mode)});
  });

  m_valve.registerSetTempChangedHandler([this](float temp) {
    m_mqttAppender.queue().push({m_getConfiguredTempTopic, String(temp)});
  });

  m_valve.registerWindowChangeHandler([this](bool state) {
    m_mqttAppender.queue().push({m_windowStateTopic, String(static_cast<int>(state))});
  });
}

bool open_heat::network::MQTT::needLoop()
{
  return rtc::offsetMillis() >= rtc::read().mqttNextCheckMillis;
}

uint64_t open_heat::network::MQTT::loop()
{
  if (!m_configValid) {
    m_logger.log(yal::Level::ERROR, "Config is not valid, no mqtt loop!");
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

  m_battery.loop();
  publish(m_getBatteryTopic + "percent", String(m_battery.percentage()));
  publish(m_getBatteryTopic + "voltage", String(m_battery.voltage()));

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
  int level = 0;
  if (!(ss >> level)) {
    return;
  }

  m_logger.setLevel(static_cast<yal::Level::Value>(level));
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
    m_logger.log(yal::Level::WARNING, "Mode % not supported", payload.c_str());
    return;
  }

  m_valve.setMode(mode);
}

void open_heat::network::MQTT::handleSetConfigTemp(const String& payload)
{
  const auto newTemp = static_cast<float>(std::strtod(payload.c_str(), nullptr));
  if (newTemp <= 0.0F) {
    return;
  }

  m_valve.setConfiguredTemp(newTemp);
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
  m_logger.log(yal::Level::INFO, "Set new modem sleep time %", newTime);
}

void open_heat::network::MQTT::publish(const String& topic, const String& message)
{
  m_logger.log(
    yal::Level::DEBUG, "MQTT send '%' in topic '%'", message.c_str(), topic.c_str());
  if (!m_mqttClient.publish(topic, message)) {
    m_logger.log(yal::Level::ERROR, "MQTT publish failed: %", m_mqttClient.lastError());
  }
}

void open_heat::network::MQTT::connect()
{
  const auto& config = m_filesystem.getConfig();
  if ((std::strlen(config.MQTT.Server) == 0 || config.MQTT.Port == 0)) {
    if (m_configValid) {
      m_logger.log(
        yal::Level::ERROR,
        "MQTT Server (%) or port (%) not set up",
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
    m_logger.log(
      yal::Level::ERROR,
      "Failed to connect to mqtt server host %, user: %, pw: %",
      config.MQTT.Server,
      config.MQTT.Username,
      config.MQTT.Password);
    return;
  }

  setTopic(config.MQTT.Topic, "log", m_logTopic);

  m_logger.log(
    yal::Level::INFO,
    "MQTT topic: %, topic len: %",
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

  if (DISABLE_ALL_LOGGING) {
    m_logger.setLevel(yal::Level::OFF);
  } else {
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
    m_logger.log(yal::Level::INFO, "MQTT subscribed to topic: %", topic.c_str());
  } else {
    m_logger.log(
      yal::Level::ERROR, "MQTT failed to subscribe to topic: %", topic.c_str());
    m_logger.log(
      yal::Level::ERROR,
      "MQTT last error: %",
      static_cast<int>(m_mqttClient.lastError()));
  }
}

void open_heat::network::MQTT::enableDebug(bool value)
{
  if (rtc::read().debug == value) {
    return;
  }

  rtc::setDebug(value);
  open_heat::rtc::wifiDeepSleep(1, value, m_filesystem);
}

void open_heat::network::MQTT::sendMessageQueue()
{
  while (!m_mqttAppender.queue().empty()) {
    const auto msg = m_mqttAppender.queue().front();
    if (msg.topic == m_logTopic) {
      // do not log again
      m_mqttClient.publish(msg.topic, msg.message);
    } else {
      publish(msg.topic, msg.message);
    }

    m_mqttAppender.queue().pop();
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
