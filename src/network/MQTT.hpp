//
// Copyright (c) 2020 Alexander Mohr
// Licensed under the terms of the MIT license
//
#ifndef OPEN_EQIVA_MQTT_CUH
#define OPEN_EQIVA_MQTT_CUH

#include <Filesystem.hpp>
#include <MQTT.h>
#include <chrono>

namespace open_heat {
namespace network {
class MQTT {
  public:
  explicit MQTT(Filesystem& filesystem) : filesystem_(filesystem)
  {
    instance_ = this;
  }

  public:
  void setup();
  void loop();
  void publish(const String& topic, const String& message);

  private:
  static void messageReceivedCallback(String &topic, String &payload);
  static MQTT* instance_;

  private:
  Filesystem& filesystem_;
  MQTTClient mqttClient_;
  WiFiClient wiFiClient_;
};
} // namespace network
} // namespace open_heat

#endif // OPEN_EQIVA_MQTT_CUH
