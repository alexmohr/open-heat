//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#ifndef OPEN_EQIVA_MQTT_CUH
#define OPEN_EQIVA_MQTT_CUH

#include "WifiManager.hpp"
#include <Filesystem.hpp>
#include <Logger.hpp>
#include <MQTT.h>
#include <heating/RadiatorValve.hpp>
#include <sensors/Battery.hpp>
#include <sensors/Humidity.hpp>
#include <sensors/Temperature.hpp>
#include <chrono>
#include <queue>

namespace open_heat::network {
class MQTT {
  public:
  MQTT(
    Filesystem& filesystem,
    WifiManager& wifi,
    sensors::Temperature* tempSensor,
    sensors::Humidity* humiditySensor,
    heating::RadiatorValve& valve,
    sensors::Battery& battery) :
      m_wifi(wifi),
      m_tempSensor(tempSensor),
      m_humiditySensor(humiditySensor),
      m_battery(battery),
      m_filesystem(filesystem),
      m_valve(valve)
  {
  }

  ~MQTT() = default;
  MQTT(const MQTT &) = delete;
  MQTT(const MQTT &&) = delete;
  MQTT &operator=(MQTT &) = delete;
  MQTT &operator=(MQTT &&) = delete;

  void setup();
  bool needLoop();
  uint64_t loop();

  void enableDebug(bool value);

  private:
  void connect();
  void publish(const String& topic, const String& message);
  void messageReceivedCallback(String& topic, String& payload);

  void handleSetConfigTemp(const String& payload);
  void handleSetMode(const String& payload);
  void handleDebug(const String& payload);
  void subscribe(const String& topic);
  void handleLogLevel(const String& payload);
  void setTopic(const String& baseTopic, const String& subTopic, String& out);
  void sendMessageQueue();
  void handleSetModemSleep(const String& payload);

  WifiManager& m_wifi;

  sensors::Temperature* m_tempSensor;
  sensors::Humidity* m_humiditySensor;
  sensors::Battery& m_battery;
  Filesystem& m_filesystem;
  heating::RadiatorValve& m_valve;

  MQTTClient m_mqttClient;

  // Topics
  String m_getModeTopic;
  String m_setModeTopic;

  String m_setConfiguredTempTopic;
  String m_getConfiguredTempTopic;

  String m_getMeasuredTempTopic;
  String m_getMeasuredHumidTopic;
  String m_getBatteryTopic;

  String m_setModemSleepTopic;
  String m_getModemSleepTopic;

  String m_debugEnableTopic;
  String m_debugLogLevelTopic;

  String m_windowStateTopic;

  String m_logTopic;

  struct message {
    const String* const topic;
    const String message;
  };
  std::queue<message> m_messageQueue;

  WiFiClient m_wifiClient;

  bool m_configValid{true};
  bool m_loggerAdded{false};
};
} // namespace open_heat::network

#endif // OPEN_EQIVA_MQTT_CUH
