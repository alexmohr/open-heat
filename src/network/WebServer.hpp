//
// Copyright (c) 2020 Alexander Mohr
// Licensed under the terms of the MIT license
//
#ifndef WEBSERVER_HPP_
#define WEBSERVER_HPP_

#include "hardware/HAL.hpp"
#include <ESPAsyncWebServer.h>
#include <Filesystem.hpp>
#include <sensors/ITemperatureSensor.hpp>

namespace open_heat {
namespace network {
class WebServer {
  public:
  WebServer(Filesystem& filesystem, sensors::ITemperatureSensor& tempSensor) :
      filesystem_(filesystem),
      tempSensor_(tempSensor),
      asyncWebServer_(AsyncWebServer(80))
  {
  }

  public:
  void setup();
  void loop();

  static void installUpdateHandlePost(AsyncWebServerRequest* request, Config& config);

  AsyncWebServer& getWebServer();

  private:
  Filesystem& filesystem_;
  sensors::ITemperatureSensor& tempSensor_;
  String htmlBuffer_;

  AsyncWebServer asyncWebServer_;
  String updateUser_ = "";
  String updatePassword_ = "";

  static constexpr const char* CONTENT_TYPE_HTML = "text/html";
  enum HtmlReturnCode { OK = 200, DENIED = 403, NOT_FOUND = 404 };

  static void installUpdateHandleUpload(
    const String& filename,
    size_t index,
    uint8_t* data,
    size_t len,
    bool final);
};

} // namespace network
} // namespace open_heat

#endif // WEBSERVER_HPP_
