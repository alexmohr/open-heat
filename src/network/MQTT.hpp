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
#include <sensors/ITemperatureSensor.hpp>
#include <chrono>
#include <queue>

namespace open_heat {
namespace network {
class MQTT {
  public:
  MQTT(
    Filesystem* filesystem,
    WifiManager& wifi,
    sensors::ITemperatureSensor& tempSensor,
    heating::RadiatorValve* valve,
    sensors::Battery* battery) :
      wifi_(wifi), tempSensor_(tempSensor)
  {
    valve_ = valve;
    filesystem_ = filesystem;
    battery_ = battery;
  }

  public:
  void setup();
  static bool needLoop();
  unsigned long loop();

  static void enableDebug(bool value);

  private:
  void connect();
  static void publish(const String& topic, const String& message);
  static void messageReceivedCallback(String& topic, String& payload);

  private:
  WifiManager& wifi_;

  sensors::ITemperatureSensor& tempSensor_;
  static sensors::Battery* battery_;
  static Filesystem* filesystem_;
  static heating::RadiatorValve* valve_;

  static MQTTClient mqttClient_;

  // Topics
  static String getModeTopic_;
  static String setModeTopic_;

  static String setConfiguredTempTopic_;
  static String getConfiguredTempTopic_;

  static String getMeasuredTempTopic_;
  static String getMeasuredHumidTopic_;
  static String getBatteryTopic_;

  static String setModemSleepTopic_;
  static String getModemSleepTopic_;

  static String debugEnableTopic_;
  static String debugLogLevelTopic_;

  static String windowStateTopic_;

  static String logTopic_;

  struct message {
    const String* const topic;
    const String message;
  };
  static std::queue<message> m_messageQueue;

  WiFiClient wiFiClient_;

  bool configValid_{true};
  bool loggerAdded_{false};

  static void handleSetConfigTemp(const String& payload);
  static void handleSetMode(const String& payload);
  static void handleDebug(const String& payload);
  static void subscribe(const String& topic);
  static void handleLogLevel(const String& payload);
  void setTopic(const String& baseTopic, const String& subTopic, String& out);
  void sendMessageQueue();
  static void handleSetModemSleep(const String& payload);
};
} // namespace network
} // namespace open_heat

#endif // OPEN_EQIVA_MQTT_CUH
