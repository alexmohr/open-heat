//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#ifndef WEBSERVER_HPP_
#define WEBSERVER_HPP_

#include <ESPAsyncWebServer.h>
#include <Filesystem.hpp>
#include <Logger.hpp>
#include <heating/RadiatorValve.hpp>
#include <sensors/Battery.hpp>
#include <sensors/Temperature.hpp>

namespace open_heat {
namespace network {
class WebServer {
  public:
  WebServer(
    Filesystem& filesystem,
    sensors::Temperature*& tempSensor,
    sensors::Battery& battery,
    open_heat::heating::RadiatorValve& valve) :
      filesystem_(filesystem),
      m_tempSensor(tempSensor),
      battery_(battery),
      valve_(valve),
      asyncWebServer_(AsyncWebServer(80))
  {
  }

  WebServer(const WebServer&) = delete;

  public:
  void setup(const char* const hostname);
  void setApList(std::vector<String>&& apList);
  void loop();

  static void installUpdateHandlePost(AsyncWebServerRequest* request, Config& config);
  void fullOpenHandlePost(AsyncWebServerRequest* request);

  private:
  Filesystem& filesystem_;
  sensors::Temperature*& m_tempSensor;
  sensors::Battery& battery_;
  open_heat::heating::RadiatorValve& valve_;

  AsyncWebServer asyncWebServer_;
  const char* m_serveIndex;

  bool m_setupDone = false;
  String m_hostname;
  std::vector<String> m_accessPointList;

  static constexpr const char* CONTENT_TYPE_HTML = "text/html";
  enum HtmlReturnCode {
    HTTP_OK = 200,
    HTTP_FOUND = 302,
    HTTP_DENIED = 403,
    HTTP_NOT_FOUND = 404
  };

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

  bool isCaptivePortal(AsyncWebServerRequest* pRequest);
  void onNotFound(AsyncWebServerRequest* request);

  String indexHTMLProcessor(const String& var);
  static void reset(AsyncWebServerRequest* request, AsyncResponseStream* response);
  static bool isIp(const String& str);
};

} // namespace network
} // namespace open_heat

#endif // WEBSERVER_HPP_
