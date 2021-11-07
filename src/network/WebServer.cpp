//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#include "WebServer.hpp"
#include <cstring>

namespace open_heat {
namespace network {

static constexpr char HTML_INDEX[] PROGMEM
= R"(<html lang='en'><head> <title>Open Heat</title> <meta charset='utf-8'> <meta name="viewport" content="width=device-width, user-scalable=no"> <script>if (!!window.EventSource){const logEvents=new EventSource('/logEvents'); logEvents.addEventListener('message', function (e){const div=document.getElementById('logDiv'); const paragraph=document.createElement("p"); const text=document.createTextNode(e.data); const str=JSON.stringify(e.data); if (str.includes("[DEBUG]") || str.includes("[TRACE]")){paragraph.className="logDebug";}else if (str.includes("[INFO]")){paragraph.className="logInfo";}else{paragraph.className="logError";}paragraph.appendChild(text); div.insertBefore(paragraph, div.firstChild);}, false);}</script> <style>html{background-color: #212121;}p{font-weight: 500;}a:visited{text-decoration: none; color: #E0E0E0;}a{text-decoration: none;}*{margin: 0; padding: 0; color: #E0E0E0; overflow-x: hidden;}body{font-size: 16px; font-family: 'Roboto', sans-serif; font-weight: 300; color: #4a4a4a;}input{width: 120px; background: #121212; border: none; border-radius: 4px; padding: 1rem; height: 50px; margin: 0.25em; font-size: 1rem; box-shadow: 0 10px 20px rgba(0, 0, 0, 0.19), 0 6px 6px rgba(0, 0, 0, 0.23);}.input-group-text{width: 120px; background: #121212; border: none; border-radius: 4px; padding: 1rem; height: 50px; margin-left: -0.5em; z-index: -1; font-size: 1rem; box-shadow: 0 10px 20px -20px rgba(0, 0, 0, 0.19), 0 6px 6px rgba(0, 0, 0, 0.23);}.inputMedium{width: 155px;}.inputSmall{width: 85px;}.inputLarge{width: 260px;}label{margin-right: 1em; font-size: 1.15rem; display: inline-block; width: 85px;}.break{flex-basis: 100%%; height: 0;}.btn{background: #303F9F; color: #EEEEEE; border-radius: 4px;}.btnLarge{width: 262px;}.flex-container{display: flex; flex-wrap: wrap;}.flex-nav{flex-grow: 1; flex-shrink: 0; background: #303F9F; height: 3rem;}.flex-menu{padding: 1rem 2rem; float: right;}.featured{background: #3F51B5; color: #ffffff; padding: 1em;}.featured h1{font-size: 2rem; margin-bottom: 1rem; font-weight: 300;}.flex-card{overflow-y: hidden; flex: 1; flex-shrink: 0; flex-basis: 400px; display: flex; flex-wrap: wrap; background: #212121; margin: .5rem; box-shadow: 0 10px 20px rgba(0, 0, 0, 0.19), 0 6px 6px rgba(0, 0, 0, 0.23);}.flex-card div{flex: 100%%;}.fit-content{height: fit-content;}.flex-card .hero{position: relative; color: #ffffff; height: 70px; background: linear-gradient(rgba(0, 0, 0, 0.5), rgba(0, 0, 0, 0.5)) no-repeat; background-size: cover;}.flex-card .hero h3{position: absolute; bottom: 15px; left: 0; padding: 0 1rem;}.content{min-height: 100%%; min-width: 400px;}.flex-card .content{color: #BDBDBD; padding: 1.5rem 1rem 2rem 1rem;}.logInfo{color: #2E7D32;}.logDebug{color: #757575;}.logError{color: #b71c1c;}</style></head><body><div class="flex-container"> <div class="flex-nav"></div></div><div class="featured"> <h1><a href="/">Open Heat</a></h1></div><div class="flex-container animated zoomIn"> <div class="flex-card"> <div class="fit-content"> <div class="hero"> <h3>Temperature</h3> </div><div class="content"> <label for="currentTemp">Measured</label> <input class="inputSmall" readonly="readonly" id="currentTemp" value="%CURRENT_TEMP%"> <span class="input-group-text">°C</span><br><form method='POST' action='/' enctype='multipart/form-data'> <label for="setTemp">Set</label> <input class="inputSmall" id="setTemp" name="setTemp" type="number" value="%SET_TEMP%"> <span class="input-group-text">°C</span> <input type='submit' value='Confirm' class="btn"> </form> <form method='POST' action='/toggle' enctype='multipart/form-data'> <label></label> <input type='submit' value='%TURN_ON_OFF%' class="btn btnLarge"></form> <form method='POST' action='/fullOpen' enctype='multipart/form-data'><label></label> <input type='submit' value='Open valve fully' class="btn btnLarge"> </form> </div></div><div class="fit-content"> <div class="hero"> <h3>Battery</h3> </div><div class="content"> <label for="batteryVoltage">Voltage</label> <input class="inputSmall" readonly="readonly" id="batteryVoltage" value="%BATTERY_VOLTAGE%"> <span class="input-group-text">V</span><br><label for="batteryPercentage">Percent</label> <input class="inputSmall" readonly="readonly" id="batteryPercentage" value="%BATTERY_PERCENTAGE%"> <span class="input-group-text">%%</span><br></div></div></div><div class="flex-card"> <div class="hero"> <h3>Settings</h3> </div><div class="content"> <h3>Network</h3> <form method='POST' action='/' enctype='multipart/form-data'><label for="netHost">Hostname</label> <input id="netHost" class="inputLarge" name="netHost" value="%HOSTNAME%"><br><h3>MQTT</h3> <label for="mqttHost">Host</label> <input id="mqttHost" class="inputMedium" name="mqttHost" value="%MQTT_HOST%"> : <input size="3" id="mqttPort" class="inputSmall" name="mqttPort" value="%MQTT_PORT%"><br><label for="mqttTopic">Topic</label> <input id="mqttTopic" class="inputLarge" name="mqttTopic" value="%MQTT_TOPIC%"><br><label for="mqttUsername">Username</label> <input id="mqttUsername" class="inputLarge" name="mqttUsername" value="%MQTT_USER%"><br><label for="mqttPassword">Password</label> <input id="mqttPassword" class="inputLarge" name="mqttPassword" value="%MQTT_PW%"><br><h3>Motor Pins</h3> <label for="motorGround">Ground</label> <input id="motorGround" class="inputLarge" name="motorGround" value="%PIN_MOTOR_GROUND%"><br><label for="motorVIN">Power</label> <input id="motorVIN" class="inputLarge" name="motorVIN" value="%PIN_MOTOR_VIN%"><br><h3>Temp Pins</h3> <label for="tempVIN">Power</label> <input id="tempVIN" class="inputLarge" name="tempVIN" value="%PIN_TEMP_VIN%"><br><h3>Window Pins</h3> <label for="windowGround">Ground</label> <input id="windowGround" class="inputLarge" name="windowGround" value="%PIN_WINDOW_GROUND%"><br><label for="windowVin">Power</label> <input id="windowVin" class="inputLarge" name="windowVIN" value="%PIN_WINDOW_VIN%"><br><br><br><input type='submit' value="Update settings & Reboot" class="btn btnLarge"> </form> </div></div><div class="flex-card"> <div class="hero"> <h3>System</h3> </div><div class="content"> <h3>Firmware update</h3> <form method='POST' action='/installUpdate' enctype='multipart/form-data'><input type='file' class="input inputLarge" accept='.bin,.bin.gz' name='firmware'> <input type='submit' value='Update' class="btn"></form> <br></div></div><div class="break"></div><div class="flex-card"> <div class="hero"> <h3>Log</h3> </div><div class="content" id="logDiv"></div></div></div></body></html>)";

static const char HTML_REDIRECT_15[] PROGMEM
  = R"(<html><style>html{background-color:#424242;font-size:16px;font-family:'Roboto',sans-serif;font-weight:300;color:#4a4a4a;color:#fefefe;text-align:center}</style><head><meta http-equiv="refresh" content="15;/" /></head><body><h1>Operation %s, reloading in 15 seconds...</h1></body></html>)";

static const char HTML_REDIRECT_NOW[] PROGMEM
  = R"(<html><head><meta http-equiv="refresh" content="0;/" /></head></html>)";

String logBuffer_;

void open_heat::network::WebServer::setup()
{
  if (isSetup_) {
    return;
  }
  isSetup_ = true;

  auto& config = filesystem_.getConfig();
  MDNS.begin(config.Hostname);
  MDNS.addService("http", "tcp", 80);

  const char* rootPath = "/";
  const char* installUpdatePath = "/installUpdate";
  const char* togglePath = "/toggle";
  const char* fullOpen = "/fullOpen";

  asyncWebServer_.on(fullOpen, HTTP_POST, [this](AsyncWebServerRequest* request) {
    fullOpenHandlePost(request);
  });

  asyncWebServer_.on(togglePath, HTTP_POST, [this](AsyncWebServerRequest* request) {
    togglePost(request);
  });

  asyncWebServer_.on(togglePath, HTTP_GET, [this](AsyncWebServerRequest* request) {
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

  setupEvents();

  asyncWebServer_.begin();
  // Logger::log(Logger::DEBUG, "Web server ready");
}

void WebServer::setupEvents()
{
  /*Logger::addPrinter(
    [this](Logger::Level level, const char* module, const char* message) {
      eventsLogPrinter(level, module, message);
    });

  asyncWebServer_.addHandler(&logEvents_);*/
}

void WebServer::eventsLogPrinter(
  const Logger::Level& level,
  const char* module,
  const char* message)
{
  logBuffer_ = LOG_LEVEL_STRINGS[level];
  logBuffer_ += F(" ");

  if (std::strlen(module) > 0) {
    logBuffer_ += F(": ");
    logBuffer_ += module;
    logBuffer_ += F(": ");
  }

  logBuffer_ += message;
  logEvents_.send(logBuffer_.c_str());
}

void open_heat::network::WebServer::loop()
{
}

AsyncWebServer& open_heat::network::WebServer::getWebServer()
{
  return asyncWebServer_;
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
  request->send_P(HTTP_OK, CONTENT_TYPE_HTML, HTML_INDEX, [this](const String& data) {
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
    request->send_P(HTTP_OK, CONTENT_TYPE_HTML, HTML_INDEX, [this](const String& data) {
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

  Logger::log(Logger::WARNING, "Invalid template: %s", var.c_str());
  return String();
}

} // namespace network
} // namespace open_heat