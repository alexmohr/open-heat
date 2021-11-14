//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#include "WebServer.hpp"
#include "generated/html/config.hpp"
#include "generated/html/index.hpp"
#include "generated/html/redirect_15.hpp"
#include "generated/html/redirect_now.hpp"
#include <cstring>
#include <functional>

namespace open_heat {
namespace network {

void open_heat::network::WebServer::setup(const char* const hostname)
{
  if (m_setupDone) {
    Logger::log(Logger::DEBUG, "Webserver setup already done");
    return;
  }

  if (hostname != nullptr) {
    m_serveIndex = HTML_CONFIG;
    Logger::log(Logger::INFO, "Serving configuration portal");
    m_hostname = String(hostname);
  } else {
    Logger::log(Logger::INFO, "Serving webinterface");
    m_serveIndex = HTML_INDEX;
    m_hostname = "";
  }

  m_setupDone = true;

  auto& config = filesystem_.getConfig();
  MDNS.begin(config.Hostname);
  MDNS.addService("http", "tcp", 80);

  const char* rootPath = "/";
  const char* installUpdatePath = "/installUpdate";
  const char* togglePath = "/toggle";
  const char* fullOpen = "/fullOpen";

  asyncWebServer_.on(
    fullOpen,
    HTTP_POST,
    std::bind(&WebServer::fullOpenHandlePost, this, std::placeholders::_1));

  asyncWebServer_.on(
    togglePath,
    HTTP_POST,
    std::bind(&WebServer::togglePost, this, std::placeholders::_1));

  asyncWebServer_.on(
    rootPath,
    HTTP_POST,
    std::bind(&WebServer::rootHandlePost, this, std::placeholders::_1));

  asyncWebServer_.onNotFound(
    std::bind(&WebServer::onNotFound, this, std::placeholders::_1));

  asyncWebServer_.on(rootPath, HTTP_GET, [this](AsyncWebServerRequest* request) {
    if (!isCaptivePortal(request)) {
      rootHandleGet(request);
    }
  });

  asyncWebServer_.on(togglePath, HTTP_GET, [this](AsyncWebServerRequest* request) {
    request->send(HTTP_OK, CONTENT_TYPE_HTML, HTML_REDIRECT_NOW);
  });

  asyncWebServer_.on(installUpdatePath, HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(HTTP_DENIED, CONTENT_TYPE_HTML, "Access denied");
  });

  asyncWebServer_.on(
    installUpdatePath,
    HTTP_POST,
    [&config](AsyncWebServerRequest* request) {
      installUpdateHandlePost(request, config);
    },
    [](
      AsyncWebServerRequest* request,
      const String& filename,
      size_t index,
      uint8_t* data,
      size_t len,
      bool final) { installUpdateHandleUpload(filename, index, data, len, final); });

  asyncWebServer_.begin();
  Logger::log(Logger::DEBUG, "Web server ready");
}

void open_heat::network::WebServer::loop()
{
}

void WebServer::installUpdateHandlePost(
  AsyncWebServerRequest* const request,
  Config& config)
{
  if (
    std::strlen(config.Update.Username) > 0 && std::strlen(config.Update.Password) > 0) {
    if (!request->authenticate(config.Update.Username, config.Update.Password)) {
      return request->requestAuthentication();
    }
  }

  const bool updateSuccess = !Update.hasError();
  AsyncResponseStream* const response = request->beginResponseStream(CONTENT_TYPE_HTML);
  response->printf(HTML_REDIRECT_15, updateSuccess ? "succeeded" : "failed");

  if (updateSuccess) {
    reset(request, response);
  }

  request->send(response);
}

void WebServer::reset(
  AsyncWebServerRequest* const request,
  AsyncResponseStream* const response)
{
  response->addHeader("Connection", "close");
  request->onDisconnect([]() {
    Logger::log(Logger::WARNING, "Restarting");
    ESP.reset();
  });
}

void WebServer::fullOpenHandlePost(AsyncWebServerRequest* const request)
{
  valve_.setMode(FULL_OPEN);

  AsyncResponseStream* const response = request->beginResponseStream(CONTENT_TYPE_HTML);
  response->printf(HTML_REDIRECT_15, "succeeded");
  request->send(response);
}

void WebServer::installUpdateHandleUpload(
  const String& filename,
  size_t index,
  uint8_t* data,
  size_t len,
  bool final)
{

  if (!index) {
    Logger::log(Logger::INFO, "Starting update with file: %s", filename.c_str());

    Update.runAsync(true);
    if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)) {
      Update.printError(Serial);
    }
  }

  if (!Update.hasError()) {
    if (Update.write(data, len) != len) {
      Update.printError(Serial);
    }
  }

  if (final) {
    if (Update.end(true)) {
      Logger::log(Logger::INFO, "Update success, filesize: %uB\n", index + len);
    } else {
      Update.printError(Serial);
    }
  }
}

void WebServer::rootHandleGet(AsyncWebServerRequest* const request)
{
  Logger::log(Logger::DEBUG, "Received request for /");
  request->send_P(HTTP_OK, CONTENT_TYPE_HTML, m_serveIndex, [this](const String& data) {
    return indexHTMLProcessor(data);
  });
}

void WebServer::rootHandlePost(AsyncWebServerRequest* const request)
{
  updateSetTemp(request);
  const auto needReset = updateConfig(request);
  if (needReset) {
    AsyncResponseStream* response = request->beginResponseStream(CONTENT_TYPE_HTML);
    response->printf(HTML_REDIRECT_15, "succeeded");
    reset(request, response);
    request->send(response);
  } else {
    request->send_P(HTTP_OK, CONTENT_TYPE_HTML, m_serveIndex, [this](const String& data) {
      return indexHTMLProcessor(data);
    });
  }
}

bool WebServer::updateConfig(AsyncWebServerRequest* const request)
{
  auto& config = filesystem_.getConfig();
  bool updateConfig = false;
  char portBuf[MQTT_PORT_STR_MAX_SIZE];
  char motorVinBuf[4]{};
  char motorGroundBuf[4]{};

  char tempVinBuf[4]{};

  char windowVinBuf[4]{};
  char windowGroundBuf[4]{};

  std::vector<std::tuple<const char*, char*>> params = {
    std::tuple<const char*, char*>{"ssid", config.WifiCredentials.wifi_ssid},
    std::tuple<const char*, char*>{"wifiPassword", config.WifiCredentials.wifi_pw},
    std::tuple<const char*, char*>{"updatePassword", config.Update.Password},
    std::tuple<const char*, char*>{"updateUsername", config.Update.Username},
    std::tuple<const char*, char*>{"mqttHost", config.MQTT.Server},
    std::tuple<const char*, char*>{"mqttTopic", config.MQTT.Topic},
    std::tuple<const char*, char*>{"mqttUsername", config.MQTT.Username},
    std::tuple<const char*, char*>{"mqttPassword", config.MQTT.Password},
    std::tuple<const char*, char*>{"mqttPort", portBuf},
    std::tuple<const char*, char*>{"netHost", config.Hostname},
    std::tuple<const char*, char*>{"motorGround", motorGroundBuf},
    std::tuple<const char*, char*>{"motorVIN", motorVinBuf},
    std::tuple<const char*, char*>{"tempVIN", tempVinBuf},
    std::tuple<const char*, char*>{"windowGround", windowGroundBuf},
    std::tuple<const char*, char*>{"windowVIN", windowVinBuf}};

  for (const auto& param : params) {
    updateConfig |= updateField(
      request, std::get<0>(param), std::get<1>(param), sizeof(std::get<1>(param)));
  }

  if (updateConfig) {
    String mqttBaseTopic = config.MQTT.Topic;
    if (!mqttBaseTopic.endsWith("/")) {
      mqttBaseTopic += "/";
      std::strcpy(config.MQTT.Topic, mqttBaseTopic.c_str());
    }

    if (std::strlen(portBuf) > 0) {
      config.MQTT.Port = static_cast<unsigned short>(std::strtol(portBuf, nullptr, 10));
    }
    if (std::strlen(motorVinBuf) > 0) {
      config.MotorPins.Vin = static_cast<int8>(std::strtol(motorVinBuf, nullptr, 10));
    }
    if (std::strlen(motorGroundBuf) > 0) {
      config.MotorPins.Ground
        = static_cast<int8>(std::strtol(motorGroundBuf, nullptr, 10));
    }
    if (std::strlen(tempVinBuf) > 0) {
      config.TempVin = static_cast<int8>(std::strtol(tempVinBuf, nullptr, 10));
    }
    if (std::strlen(windowVinBuf) > 0) {
      config.WindowPins.Vin = static_cast<int8>(std::strtol(windowVinBuf, nullptr, 10));
    }
    if (std::strlen(windowGroundBuf) > 0) {
      config.WindowPins.Ground
        = static_cast<int8>(std::strtol(windowGroundBuf, nullptr, 10));
    }

    filesystem_.persistConfig();
  }

  return updateConfig;
}

void WebServer::updateSetTemp(const AsyncWebServerRequest* const request)
{
  static const char* paramSetTemp = "setTemp";
  const bool isPost = request->method() == HTTP_POST;
  if (request->hasParam(paramSetTemp, isPost)) {
    const AsyncWebParameter* const param = request->getParam(paramSetTemp, isPost);
    const auto newTemp = static_cast<float>(strtod(param->value().c_str(), nullptr));
    valve_.setConfiguredTemp(newTemp);
  } else {
    Logger::log(Logger::DEBUG, "updatingField, param not found %s", paramSetTemp);
  }
}

bool WebServer::updateField(
  AsyncWebServerRequest* request,
  const char* paramName,
  char* field,
  size_t fieldLen)
{
  const bool isPost = request->method() == HTTP_POST;
  if (!request->hasParam(paramName, isPost)) {
    Logger::log(open_heat::Logger::DEBUG, "updatingField, param not found %s", paramName);
    return false;
  }

  const AsyncWebParameter* const param = request->getParam(paramName, isPost);
  std::memset(field, 0, fieldLen);
  std::strcpy(field, param->value().c_str());
  Logger::log(
    Logger::DEBUG,
    "Updating field %s (len: %i), new value %s",
    paramName,
    fieldLen,
    param->value().c_str());
  return true;
}

void WebServer::togglePost(AsyncWebServerRequest* const pRequest)
{
  const auto newMode = valve_.getMode() != HEAT ? HEAT : OFF;
  valve_.setMode(newMode);

  pRequest->send(HTTP_OK, CONTENT_TYPE_HTML, HTML_REDIRECT_NOW);
}

String WebServer::indexHTMLProcessor(const String& var)
{
  const auto& config = filesystem_.getConfig();
  const auto setTemp = static_cast<uint8_t>(valve_.getConfiguredTemp());

  // Temp
  if (var == F("CURRENT_TEMP")) {
    return String(tempSensor_.getTemperature());
  } else if (var == F("SET_TEMP")) {
    return String(setTemp);
  }

  // MQTT
  else if (var == F("MQTT_HOST")) {
    return config.MQTT.Server;
  } else if (var == F("MQTT_PORT")) {
    return String(config.MQTT.Port);
  } else if (var == F("MQTT_TOPIC")) {
    return config.MQTT.Topic;
  } else if (var == F("MQTT_USER")) {
    return config.MQTT.Username;
  } else if (var == F("MQTT_PW")) {
    return config.MQTT.Password;
  }

  // Header
  else if (var == F("HOSTNAME")) {
    return config.Hostname;
  }

  // Mode
  else if (var == F("TURN_ON_OFF")) {
    if (valve_.getMode() == HEAT) {
      return F("Turn off");
    } else {
      return F("Turn on");
    }
  }

  // MotorPins
  else if (var == F("PIN_MOTOR_VIN")) {
    return String(config.MotorPins.Vin);
  } else if (var == F("PIN_MOTOR_GROUND")) {
    return String(config.MotorPins.Ground);
  }

  // Temp Pins
  else if (var == F("PIN_TEMP_VIN")) {
    return String(config.TempVin);
  }

  // Window pins
  else if (var == F("PIN_WINDOW_VIN")) {
    return String(config.WindowPins.Vin);
  } else if (var == F("PIN_WINDOW_GROUND")) {
    return String(config.WindowPins.Ground);
  }

  // Battery state
  else if (var == F("BATTERY_VOLTAGE")) {
    battery_.loop();
    return String(battery_.voltage());
  } else if (var == F("BATTERY_PERCENTAGE")) {
    battery_.loop();
    return String(battery_.percentage());
  }

  // Wi-Fi settings
  else if (var == F("SSID")) {
    return String(config.WifiCredentials.wifi_ssid);
  } else if (var == F("WIFI_PASSWORD")) {
    return String(config.WifiCredentials.wifi_pw);
  }

  // update settings
  else if (var == F("UPDATE_USERNAME")) {
    return String(config.Update.Username);
  } else if (var == F("UPDATE_PASSWORD")) {
    return String(config.Update.Password);
  }

  Logger::log(Logger::WARNING, "Invalid template: %s", var.c_str());
  return String();
}

bool WebServer::isCaptivePortal(AsyncWebServerRequest* request)
{
  if (m_hostname.isEmpty()) {
    return false;
  }

  const auto hostHeader = request->getHeader("host")->value();
  const auto hostIsIp = isIp(hostHeader);
  Logger::log(
    Logger::DEBUG,
    "host header: %s, isIP: %i, hostname: %s, wifi ip: %s",
    hostHeader.c_str(),
    hostIsIp,
    m_hostname.c_str(),
    WiFi.softAPIP().toString().c_str());

  const auto captive = !hostIsIp && (hostHeader != m_hostname || hostHeader != m_hostname + ".local");
  if (!captive) {
    return false;
  }

  Logger::log(Logger::INFO, "Sending redirect for captive portal");
  AsyncWebServerResponse* response = request->beginResponse(HTTP_FOUND, "text/plain", "");
  response->addHeader("Location", "http://" + WiFi.softAPIP().toString());
  request->send(response);
  return true;
}

void WebServer::onNotFound(AsyncWebServerRequest* request)
{
  if (isCaptivePortal(request)) {
    return;
  }

  request->send(HTTP_NOT_FOUND, CONTENT_TYPE_HTML, HTML_REDIRECT_NOW);
}
bool WebServer::isIp(const String& str)
{
  for (const auto& c : str) {
    if (c != '.' && (c < '0' || c > '9')) {
      return false;
    }
  }
  return true;
}

} // namespace network
} // namespace open_heat