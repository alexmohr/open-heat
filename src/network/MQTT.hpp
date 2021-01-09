//
// Copyright (c) 2020 Alexander Mohr
// Licensed under the terms of the MIT license
//
#ifndef OPEN_EQIVA_MQTT_CUH
#define OPEN_EQIVA_MQTT_CUH

#include <Filesystem.hpp>
#include <MQTT.h>
#include <sensors/ITemperatureSensor.hpp>
#include <chrono>

namespace open_heat {
namespace network {
class MQTT {
  public:
  explicit MQTT(Filesystem& filesystem,
                    sensors::ITemperatureSensor& tempSensor) : filesystem_(filesystem),
          tempSensor_(tempSensor)
  {
  }

  public:
  void setup();
  void loop();

  private:
  void publish(const String& topic, const String& message);
  static void messageReceivedCallback(String& topic, String& payload);

  private:
  Filesystem& filesystem_;
  sensors::ITemperatureSensor& tempSensor_;

  MQTTClient mqttClient_;
  WiFiClient wiFiClient_;

  unsigned long nextCheckMillis_ = 0;
  // publish every minute
  unsigned long checkIntervalMillis_ = 1 * 60 * 1000;

};
} // namespace network
} // namespace open_heat

#endif // OPEN_EQIVA_MQTT_CUH
