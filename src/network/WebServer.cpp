//
// Copyright (c) 2020 Alexander Mohr
// Licensed under the terms of the MIT license
//
#include "WebServer.hpp"
#include "hardware/HAL.hpp"
#include <Config.hpp>
#include <Logger.hpp>

namespace open_heat {
namespace network {

static constexpr char HTML_INDEX[] PROGMEM
  = R"(<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,user-scalable=no"><style>html{background-color:#212121}a{text-decoration:none}*{margin:0;padding:0;color:#E0E0E0;overflow-x:hidden}body{font-size:16px;font-family:Roboto,sans-serif;font-weight:300;color:#4a4a4a}input{width:120px;background:#212121;border:none;border-radius:1%;padding:1rem;height:50px;margin:.25em;font-size:1rem;box-shadow:0 10px 20px rgba(0,0,0,.19),0 6px 6px rgba(0,0,0,.23)}.input-group-text{width:120px;background:#212121;border:none;border-radius:1%;padding:1rem;height:50px;margin-left:-.5em;z-index:-1;font-size:1rem;box-shadow:0 10px 20px -20px rgba(0,0,0,.19),0 6px 6px rgba(0,0,0,.23)}.inputMedium{width:155px}.inputSmall{width:85px}.inputLarge{width:260px}label{margin-right:1em;font-size:1.15rem;display:inline-block;width:85px}.break{flex-basis:100%;height:0}.btn{background:#303F9F;color:#EEE;border-radius:4px}.btnLarge{width:262px}.flex-container{display:flex;flex-wrap:wrap}.flex-nav{flex-grow:1;flex-shrink:0;background:#303F9F;height:3rem}.flex-menu{padding:1rem 2rem;float:right}.featured{background:#3F51B5;color:#fff;padding:1em}.featured h1{font-size:2rem;margin-bottom:1rem;font-weight:300}.flex-card{overflow-y:hidden;flex:1;flex-shrink:0;flex-basis:400px;display:flex;flex-wrap:wrap;background:#263238;margin:.5rem;box-shadow:0 10px 20px rgba(0,0,0,.19),0 6px 6px rgba(0,0,0,.23)}.flex-card:hover{box-shadow:0 20px 28px rgba(0,0,0,.75),0 10px 10px rgba(0,0,0,.22)}.flex-card div{flex:100%}.flex-card .hero{position:relative;color:#fff;height:70px;background:linear-gradient(rgba(0,0,0,.5),rgba(0,0,0,.5)) no-repeat;background-size:cover}.flex-card .hero h3{position:absolute;bottom:15px;left:0;padding:0 1rem}.content{min-height:100%;min-width:400px}.flex-card .content{color:#BDBDBD;padding:1.5rem 1rem 2rem 1rem}</style><body><div class="flex-container"><div class="flex-nav"></div></div><div class="featured"><h1><a href="/">Open Heat</a></h1></div><div class="flex-container animated zoomIn"><div class="flex-card"><div class="hero"><h3>Temperature</h3></div><div class="content"><label for="currentTemp">Measured</label><input class="inputSmall" readonly id="currentTemp" value="{CURRENT_TEMP}"> <span class="input-group-text">°C</span><br><form method="POST" action="/" enctype="multipart/form-data"><label for="setTemp">Set</label><input class="inputSmall" id="setTemp" name="setTemp" type="number" value="{SET_TEMP}"> <span class="input-group-text">°C</span> <input type="submit" value="Confirm" class="btn"></form><form method="POST" action="/off" enctype="multipart/form-data"><label></label><input type="submit" value="Turn off" class="btn btnLarge"></form></div></div><div class="flex-card"><div class="hero"><h3>Settings</h3></div><div class="content"><h3>Network</h3><form method="POST" action="/" enctype="multipart/form-data"><label for="netHost">Hostname</label><input id="netHost" class="inputLarge" name="netHost" value="{HOSTNAME}"><br><h3>MQTT</h3><label for="mqttHost">Host</label><input id="mqttHost" class="inputMedium" name="mqttHost" value="{MQTT_HOST}"> : <input size="3" id="mqttPort" class="inputSmall" name="mqttPort" value="{MQTT_PORT}"><br><label for="mqttTopic">Topic</label><input id="mqttTopic" class="inputLarge" name="mqttTopic" value="{MQTT_TOPIC}"><br><label for="mqttUsername">Username</label><input id="mqttUsername" class="inputLarge" name="mqttUsername" value="{MQTT_USER}"><br><label for="mqttPassword">Password</label><input id="mqttPassword" class="inputLarge" name="mqttPassword" value="{MQTT_PW}"><br><br><input type="submit" value="Update settings" class="btn btnLarge"></form></div></div><div class="flex-card"><div class="hero"><h3>System</h3></div><div class="content"><h3>Firmware update</h3><form method="POST" action="/installUpdate" enctype="multipart/form-data"><input type="file" class="input inputLarge" accept=".bin,.bin.gz" name="firmware"> <input type="submit" value="Update" class="btn"></form><br></div></div><div class="break"></div><div class="flex-card"><div class="hero"><h3>Log</h3></div><div class="content">{LOG_DATA}</div></div></div>)";

static const char HTML_INSTALLED[] PROGMEM
  = R"(<html><style>html{background-color:#424242;font-size:16px;font-family:'Roboto',sans-serif;font-weight:300;color:#4a4a4a;color:#fefefe;text-align:center}</style><head><meta http-equiv="refresh" content="15;/" /></head><body><h1>Update %s, reloading in 15 seconds...</h1></body></html>)";

static const char HTML_REDIRECT_NOW[] PROGMEM
  = R"(<html><head><meta http-equiv="refresh" content="0;/" /></head></html>)";


void open_heat::network::WebServer::setup()
{
  auto &config = filesystem_.getConfig();
  MDNS.begin(config.Hostname);
  MDNS.addService("http", "tcp", 80);

  const char* rootPath = "/";
  const char* installUpdatePath = "/installUpdate";
  const char* offPath = "/off";

  asyncWebServer_.on(offPath, HTTP_POST, [this](AsyncWebServerRequest* request) {
    offHandlePost(request);
  });

  asyncWebServer_.on(offPath, HTTP_GET, [this](AsyncWebServerRequest* request) {
    request->send(HTTP_OK, CONTENT_TYPE_HTML, HTML_REDIRECT_NOW);
  });

  asyncWebServer_.on(rootPath, HTTP_POST, [this](AsyncWebServerRequest* request) {
    rootHandlePost(request);
  });

  asyncWebServer_.on(rootPath, HTTP_GET, [this](AsyncWebServerRequest* request) {
    rootHandleGet(request);
  });

  asyncWebServer_.on(installUpdatePath, HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(403, CONTENT_TYPE_HTML, "Access denied");
  });

  asyncWebServer_.on(
    installUpdatePath,
    HTTP_POST,
    [this, &config](AsyncWebServerRequest* request) {
      installUpdateHandlePost(request, config);
    },
    [this](
      AsyncWebServerRequest* request,
      const String& filename,
      size_t index,
      uint8_t* data,
      size_t len,
      bool final) { installUpdateHandleUpload(filename, index, data, len, final); });

  asyncWebServer_.begin();
}

void WebServer::updateHTML()
{
  const auto &config = filesystem_.getConfig();
  const float setTemp = valve_.getConfiguredTemp();
  htmlBuffer_ = String(network::HTML_INDEX);

  // Temp
  htmlBuffer_.replace("{CURRENT_TEMP}", String(tempSensor_.getTemperature()));
  htmlBuffer_.replace("{SET_TEMP}", String(setTemp));

  // MQTT
  htmlBuffer_.replace("{MQTT_HOST}", config.MQTT.Server);
  htmlBuffer_.replace("{MQTT_PORT}", String(config.MQTT.Port));
  htmlBuffer_.replace("{MQTT_TOPIC}", config.MQTT.Topic);
  htmlBuffer_.replace("{MQTT_USER}", config.MQTT.Username);
  htmlBuffer_.replace("{MQTT_PW}", config.MQTT.Password);

  // Header
  htmlBuffer_.replace("{HOSTNAME}", config.Hostname);
}


void open_heat::network::WebServer::loop()
{
}

AsyncWebServer& open_heat::network::WebServer::getWebServer()
{
  return asyncWebServer_;
}

void WebServer::installUpdateHandlePost(AsyncWebServerRequest* request, Config& config)
{
  if (strlen(config.Update.Username) > 0 && strlen(config.Update.Password) > 0) {
    if (!request->authenticate(config.Update.Username, config.Update.Password)) {
      return request->requestAuthentication();
    }
  }

  bool updateSuccess = !Update.hasError();
  AsyncResponseStream* response = request->beginResponseStream(CONTENT_TYPE_HTML);
  response->printf(HTML_INSTALLED, updateSuccess ? "succeeded" : "failed");
  response->addHeader("Connection", "close");
  if (updateSuccess) {
    request->onDisconnect([]() {
      Logger::log(Logger::WARNING, "Restarting");
      ESP.reset();
    });
  }

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

void WebServer::rootHandleGet(AsyncWebServerRequest* request)
{
  updateHTML();
  request->send(HTTP_OK, CONTENT_TYPE_HTML, htmlBuffer_);
}

void WebServer::rootHandlePost(AsyncWebServerRequest* request)
{
  updateSetTemp(request);
  updateConfig(request);
  updateHTML();
  request->send(HTTP_OK, CONTENT_TYPE_HTML, htmlBuffer_);
}
void WebServer::updateConfig(AsyncWebServerRequest* request)
{
  bool updateConfig = false;
  char portBuf[MQTT_PORT_STR_MAX_SIZE];
  auto& config = filesystem_.getConfig();
  std::vector<std::tuple<const char*, char*>> params = {
    std::tuple<const char*, char*>{"mqttHost", config.MQTT.Server},
    std::tuple<const char*, char*>{"mqttTopic", config.MQTT.Topic},
    std::tuple<const char*, char*>{"mqttUsername", config.MQTT.Username},
    std::tuple<const char*, char*>{"mqttPassword", config.MQTT.Password},
    std::tuple<const char*, char*>{"mqttPort", portBuf},
    std::tuple<const char*, char*>{"netHost", config.Hostname},
  };

  for (const auto& param: params) {
    updateConfig |= updateField(
      request, std::get<0>(param), std::get<1>(param), sizeof(std::get<1>(param)));
  }

  if (updateConfig) {
    filesystem_.persistConfig();
  }
}

void WebServer::updateSetTemp(
  const AsyncWebServerRequest* request)
{
  static const char* paramSetTemp = "setTemp";
  bool isPost = request->method() == HTTP_POST;
  if (request->hasParam(paramSetTemp, isPost)) {
    AsyncWebParameter* param = request->getParam(paramSetTemp, isPost);
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
  int fieldLen)
{
  bool isPost = request->method() == HTTP_POST;
  if (!request->hasParam(paramName, isPost)) {
    Logger::log(open_heat::Logger::DEBUG, "updatingField, param not found %s", paramName);
    return false;
  }

  AsyncWebParameter* param = request->getParam(paramName, isPost);
  memset(field, 0, fieldLen);
  strcpy(field, param->value().c_str());
  Logger::log(
    Logger::DEBUG, "Updating field %s, new value %s", field, param->value().c_str());
  return true;
}

void WebServer::offHandlePost(AsyncWebServerRequest* request)
{
  valve_.setConfiguredTemp(0);
  request->send(HTTP_OK, CONTENT_TYPE_HTML, HTML_REDIRECT_NOW);
}

} // namespace network
} // namespace open_heat