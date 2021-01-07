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


static constexpr char updateHTML[] PROGMEM
  = R"(<html lang='en'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1' /><style>html{background-color:#424242}*{margin:0;padding:0}body{font-size:16px;font-family:'Roboto',sans-serif;font-weight:300;color:#4a4a4a;border-radius:5%}input{border:none;border-radius:1%;padding:1rem;margin:0.25em;font-size:1rem;box-shadow:0 10px 20px rgba(0, 0, 0, 0.19), 0 6px 6px rgba(0,0,0,0.23)}label{margin-right:1em;font-size:1.15rem;display:inline-block;width:75px}.btn{background:#303F9F;color:#EEE}.flex-container{display:flex;flex-wrap:wrap}.flex-nav{flex-grow:1;flex-shrink:0;background:#303F9F;height:3rem}.flex-menu{padding:1rem 2rem;float:right}.featured{background:#3F51B5;color:#fff;padding:1em}.featured h1{font-size:3rem;margin-bottom:1rem;font-weight:300}.flex-card{flex:1;flex-grow:1;flex-shrink:0;flex-basis:400px;display:flex;flex-wrap:wrap;background:#fff;margin: .5rem;box-shadow:0 10px 20px rgba(0, 0, 0, 0.19), 0 6px 6px rgba(0,0,0,0.23)}.flex-card:hover{box-shadow:0 20px 28px rgba(0, 0, 0, 0.75), 0 10px 10px rgba(0, 0, 0, 0.22)}.flex-card div{flex:100%}.flex-card .hero{position:relative;color:#fff;height:75px;background:linear-gradient(rgba(0, 0, 0, 0.5), rgba(0, 0, 0, 0.5)) no-repeat;background-size:cover}.flex-card .hero h3{position:absolute;bottom:15px;left:0;padding:0 1rem}.flex-card .content{color:#555;padding:1.5rem 1rem 2rem 1rem}</style></head><body><div class="flex-container"><div class="flex-nav"></div></div><div class="featured"><h1>Open Heat</h1></div><div class="flex-container animated zoomIn"><div class="flex-card"><div class="hero"><h3>Temperature</h3></div><div class="content"> <label for="currentTemp">Measured</label> <input readonly="readonly" id="currentTemp" value="{CURRENT_TEMP}"><br><label for="setTemp">Set</label> <input id="setTemp" type="number" value="{SET_TEMP}"><br></div></div><div class="flex-card"><div class="hero"><h3>Update Firmware</h3></div><div class="content"><form method='POST' action='/installUpdate' enctype='multipart/form-data'> <input type='file' accept='.bin,.bin.gz' name='firmware' > <input type='submit' value='Update Firmware' class="btn" ></form></div></div><div class="flex-card"><div class="hero"><h3>Log</h3></div><div class="content"><form method='POST' action='/installUpdate' enctype='multipart/form-data'> <input type='file' accept='.bin,.bin.gz' name='firmware' > <input type='submit' value='Update Firmware' class="btn" ></form></div></div></div></body></html>)";

static const char installedResponse[] PROGMEM
  = R"(<html><style>html{background-color:#424242;font-size:16px;font-family:'Roboto',sans-serif;font-weight:300;color:#4a4a4a;color:#fefefe;text-align:center}</style><head><meta http-equiv="refresh" content="10;/" /></head><body><h1>Update %s, reloading in 10 seconds...</h1></body></html>)";

void open_heat::network::WebServer::setup()
{
  MDNS.begin(HOST_NAME);
  MDNS.addService("http", "tcp", 80);

  auto& config = filesystem_.getConfig();
  const char* rootPath = "/";
  const char* installUpdatePath = "/installUpdate";



  asyncWebServer_.on(rootPath, HTTP_GET, [this](AsyncWebServerRequest* request) {
    const float setTemp = 22;
    htmlBuffer_= String(updateHTML);
    htmlBuffer_.replace("{CURRENT_TEMP}", String(tempSensor_.getTemperature()));
    htmlBuffer_.replace("{SET_TEMP}", String(setTemp));
    request->send(200, CONTENT_TYPE_HTML, htmlBuffer_);
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
  response->printf(installedResponse, updateSuccess ? "succeeded" : "failed");
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

} // namespace network
} // namespace open_heat