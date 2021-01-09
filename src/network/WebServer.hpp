//
// Copyright (c) 2020 Alexander Mohr
// Licensed under the terms of the MIT license
//
#ifndef WEBSERVER_HPP_
#define WEBSERVER_HPP_

#include "hardware/HAL.hpp"
#include <ESPAsyncWebServer.h>
#include <Filesystem.hpp>
#include <heating/RadiatorValve.hpp>
#include <sensors/ITemperatureSensor.hpp>

namespace open_heat {
namespace network {
class WebServer {
  public:
      WebServer(
        Filesystem& filesystem,
        sensors::ITemperatureSensor& tempSensor,
        open_heat::heating::RadiatorValve& valve) :
      filesystem_(filesystem),
      tempSensor_(tempSensor),
          valve_(valve),
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
  open_heat::heating::RadiatorValve& valve_;

  String htmlBuffer_;

  AsyncWebServer asyncWebServer_;

  static constexpr const char* CONTENT_TYPE_HTML = "text/html";
  enum HtmlReturnCode { HTTP_OK = 200, HTTP_DENIED = 403, HTTP_NOT_FOUND = 404 };

  static void installUpdateHandleUpload(
    const String& filename,
    size_t index,
    uint8_t* data,
    size_t len,
    bool final);
  void updateHTML();
  static bool updateField(
  AsyncWebServerRequest* request,
  const char* paramName,
  char* field,
  int fieldLen);
  void rootHandleGet(AsyncWebServerRequest* request);
  void rootHandlePost(AsyncWebServerRequest* pRequest);
  void updateSetTemp(const AsyncWebServerRequest* request);
  void offHandlePost(AsyncWebServerRequest* pRequest);
  void updateConfig(AsyncWebServerRequest* request);
};

} // namespace network
} // namespace open_heat

#endif // WEBSERVER_HPP_
