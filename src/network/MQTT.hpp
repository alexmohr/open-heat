//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#ifndef OPEN_EQIVA_MQTT_CUH
#define OPEN_EQIVA_MQTT_CUH

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
    sensors::ITemperatureSensor& tempSensor,
    heating::RadiatorValve* valve) :
      filesystem_(filesystem), tempSensor_(tempSensor)
  {
    config_ = &filesystem_.getConfig();
    valve_ = valve;
  }

  public:
  void setup();
  void loop();

  private:
  void connect();
  static void publish(const String& topic, const String& message);
  static void messageReceivedCallback(String& topic, String& payload);

  void mqttLogPrinter(
    open_heat::Logger::Level level,
    const char* module,
    const char* message);

  private:
  Filesystem& filesystem_;
  sensors::ITemperatureSensor& tempSensor_;
  static heating::RadiatorValve* valve_;

  static MQTTClient mqttClient_;

  // Topics
  static String getModeTopic_;
  static String setModeTopic_;

  static String setConfiguredTempTopic_;
  static String getConfiguredTempTopic_;

  static String getMeasuredTempTopic_;

  static String logTopic_;

  WiFiClient wiFiClient_;

  unsigned long nextCheckMillis_ = 0;
  // publish every minute
  unsigned long checkIntervalMillis_ = 1 * 60 * 1000;

  static Config* config_;
};
} // namespace network
} // namespace open_heat

#endif // OPEN_EQIVA_MQTT_CUH
