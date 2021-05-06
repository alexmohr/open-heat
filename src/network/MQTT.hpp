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
#include <sensors/ITemperatureSensor.hpp>
#include <chrono>

namespace open_heat {
namespace network {
class MQTT {
  public:
  explicit MQTT(
    Filesystem& filesystem,
    WifiManager& wifi,
    sensors::ITemperatureSensor& tempSensor,
    heating::RadiatorValve* valve) :
      filesystem_(filesystem), wifi_(wifi), tempSensor_(tempSensor)
  {
    config_ = &filesystem_.getConfig();
    valve_ = valve;
  }

  public:
  void setup();
  unsigned long loop();

  static bool debug();

  private:
  void connect();
  static void publish(const String& topic, const String& message);
  static void messageReceivedCallback(String& topic, String& payload);

  static void mqttLogPrinter(
    open_heat::Logger::Level level,
    const char* module,
    const char* message);

  private:
  Filesystem& filesystem_;
  WifiManager& wifi_;

  sensors::ITemperatureSensor& tempSensor_;
  static heating::RadiatorValve* valve_;

  static MQTTClient mqttClient_;

  // Topics
  static String getModeTopic_;
  static String setModeTopic_;

  static String setConfiguredTempTopic_;
  static String getConfiguredTempTopic_;

  static String getMeasuredTempTopic_;

  static String debugEnableTopic_;
  static String debugLogLevel_;

  static String windowStateTopic_;

  static String logTopic_;

  WiFiClient wiFiClient_;

  unsigned long nextCheckMillis_ = 0;
  unsigned long checkIntervalMillis_ = 3 * 60 * 1000;

  bool configValid_{true};
  bool loggerAdded_{false};
  static bool debugEnabled_;

  static Config* config_;
  static void handleSetConfigTemp(const String& payload);
  static void handleGetConfigTemp();
  static void handleSetMode(const String& payload);
  static void handleDebug(const String& payload);
  static void subscribe(const String& topic);
  static void handleLogLevel(const String& payload);
};
} // namespace network
} // namespace open_heat

#endif // OPEN_EQIVA_MQTT_CUH
