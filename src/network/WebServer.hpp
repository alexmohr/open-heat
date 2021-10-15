//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#ifndef WEBSERVER_HPP_
#define WEBSERVER_HPP_

#include "hardware/HAL.hpp"
#include <ESPAsyncWebServer.h>
#include <Filesystem.hpp>
#include <Logger.hpp>
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
      asyncWebServer_(AsyncWebServer(80)),
      logEvents_(AsyncEventSource("/logEvents"))
  {
  }

  public:
  void setup();
  void loop();

  static void installUpdateHandlePost(AsyncWebServerRequest* request, Config& config);
  void fullOpenHandlePost(AsyncWebServerRequest* request);

  AsyncWebServer& getWebServer();

  private:
  Filesystem& filesystem_;
  sensors::ITemperatureSensor& tempSensor_;
  open_heat::heating::RadiatorValve& valve_;

  AsyncWebServer asyncWebServer_;
  AsyncEventSource logEvents_;

  static constexpr const char* CONTENT_TYPE_HTML = "text/html";
  enum HtmlReturnCode { HTTP_OK = 200, HTTP_DENIED = 403, HTTP_NOT_FOUND = 404 };

  static void installUpdateHandleUpload(
    const String& filename,
    size_t index,
    uint8_t* data,
    size_t len,
    bool final);

  static bool updateField(
    AsyncWebServerRequest* request,
    const char* paramName,
    char* field,
    size_t fieldLen);
  void rootHandleGet(AsyncWebServerRequest* request);
  void rootHandlePost(AsyncWebServerRequest* pRequest);
  void updateSetTemp(const AsyncWebServerRequest* request);
  void togglePost(AsyncWebServerRequest* pRequest);
  bool updateConfig(AsyncWebServerRequest* request);

  String indexHTMLProcessor(const String& var);
  void setupEvents();
  void eventsLogPrinter(
    const open_heat::Logger::Level& level,
    const char* module,
    const char* message);
  static void reset(AsyncWebServerRequest* request, AsyncResponseStream* response);

  bool isSetup_ = false;
};

} // namespace network
} // namespace open_heat

#endif // WEBSERVER_HPP_
