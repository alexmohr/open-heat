//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#ifndef OPEN_EQIVA_MQTT_CUH
#define OPEN_EQIVA_MQTT_CUH

#include <MQTT.h>
#include <esp-gui/Configuration.hpp>
#include <esp-gui/WifiManager.hpp>
#include <heating/RadiatorValve.hpp>
#include <sensors/Battery.hpp>
#include <sensors/Humidity.hpp>
#include <sensors/Temperature.hpp>
#include <yal/appender/ArduinoMQTT.hpp>
#include <yal/yal.hpp>
#include <chrono>
#include <queue>

namespace open_heat::network {
class MQTT {
  public:
  MQTT(
    esp_gui::WifiManager& wifi,
    esp_gui::Configuration& config,
    sensors::Temperature*& tempSensor,
    sensors::Humidity*& humiditySensor,
    heating::RadiatorValve& valve,
    sensors::Battery& battery) :
      m_wifi(wifi),
      m_config(config),
      m_tempSensor(tempSensor),
      m_humiditySensor(humiditySensor),
      m_battery(battery),
      m_valve(valve),
      m_logger(yal::Logger("MQTT")),
      m_mqttAppender(nullptr)
  {
  }

  ~MQTT() = default;
  MQTT(const MQTT&) = delete;
  MQTT(const MQTT&&) = delete;
  MQTT& operator=(MQTT&) = delete;
  MQTT& operator=(MQTT&&) = delete;

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
  static void setTopic(const String& baseTopic, const String& subTopic, String& out);
  void sendMessageQueue();
  void handleSetModemSleep(const String& payload);

  esp_gui::WifiManager& m_wifi;
  esp_gui::Configuration& m_config;

  sensors::Temperature*& m_tempSensor;
  sensors::Humidity*& m_humiditySensor;
  sensors::Battery& m_battery;
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

  WiFiClient m_wifiClient;

  bool m_configValid{true};

  yal::Logger m_logger;
  yal::appender::ArduinoMQTT<MQTTClient>* m_mqttAppender = nullptr;
};
} // namespace open_heat::network

#endif // OPEN_EQIVA_MQTT_CUH
