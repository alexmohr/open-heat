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

namespace open_heat {
namespace network {
class MQTT {
  public:
  MQTT(
    Filesystem* filesystem,
    WifiManager& wifi,
    sensors::Temperature*& tempSensor,
    sensors::Humidity*& humiditySensor,
    heating::RadiatorValve* valve,
    sensors::Battery* battery) :
      m_wifi(wifi), m_tempSensor(tempSensor), m_humiditySensor(humiditySensor)
  {
    m_valve = valve;
    m_filesystem = filesystem;
    m_battery = battery;
  }

  MQTT(const MQTT&) = delete;

  public:
  void setup();
  static bool needLoop();
  uint64_t loop();

  static void enableDebug(bool value);

  private:
  void connect();
  static void publish(const String& topic, const String& message);
  static void messageReceivedCallback(String& topic, String& payload);

  static void handleSetConfigTemp(const String& payload);
  static void handleSetMode(const String& payload);
  static void handleDebug(const String& payload);
  static void subscribe(const String& topic);
  static void handleLogLevel(const String& payload);
  void setTopic(const String& baseTopic, const String& subTopic, String& out);
  void sendMessageQueue();
  static void handleSetModemSleep(const String& payload);

  private:
  WifiManager& m_wifi;

  sensors::Temperature*& m_tempSensor;
  sensors::Humidity*& m_humiditySensor;
  static sensors::Battery* m_battery;
  static Filesystem* m_filesystem;
  static heating::RadiatorValve* m_valve;

  static MQTTClient m_mqttClient;

  // Topics
  static String m_getModeTopic;
  static String m_setModeTopic;

  static String m_setConfiguredTempTopic;
  static String m_getConfiguredTempTopic;

  static String m_getMeasuredTempTopic;
  static String m_getMeasuredHumidTopic;
  static String m_getBatteryTopic;

  static String m_setModemSleepTopic;
  static String m_getModemSleepTopic;

  static String m_debugEnableTopic;
  static String m_debugLogLevelTopic;

  static String m_windowStateTopic;

  static String m_logTopic;

  struct message {
    const String* const topic;
    const String message;
  };
  static std::queue<message> m_messageQueue;

  WiFiClient m_wifiClient;

  bool m_configValid{true};
  bool m_loggerAdded{false};
};
} // namespace network
} // namespace open_heat

#endif // OPEN_EQIVA_MQTT_CUH
