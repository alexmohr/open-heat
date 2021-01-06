//
// Copyright (c) 2020 Alexander Mohr
// Licensed under the terms of the MIT license
//
#include "WebServer.hpp"
#include "hardware/HAL.hpp"
#include <Config.hpp>

void open_heat::network::WebServer::setup()
{
  MDNS.begin(HOST_NAME);
  // https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266HTTPUpdateServer/src/ESP8266HTTPUpdateServer-impl.h
  MDNS.addService("http", "tcp", 80);

}

void open_heat::network::WebServer::loop()
{
}


//void WebServer::setup()
//{
//  MDNS.begin(host);
//
//  // Add service to MDNS-SD
//  MDNS.addService("http", "tcp", HTTP_PORT);
//
//  // SERVER INIT
//  events.onConnect([](AsyncEventSourceClient *client) {
//    client->send("hello!", NULL, millis(), 1000);
//  });
//
//  server.addHandler(&events);
//
//  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request) {
//    request->send(200, "text/plain", String(ESP.getFreeHeap()));
//  });
//
//  server.addHandler(new SPIFFSEditor(http_username, http_password, FileFS));
//  server.serveStatic("/", FileFS, "/").setDefaultFile("index.htm");
//
//  server.onNotFound([](AsyncWebServerRequest *request) {
//    Serial.print("NOT_FOUND: ");
//
//    if (request->method() == HTTP_GET)
//      Serial.print("GET");
//    else if (request->method() == HTTP_POST)
//      Serial.print("POST");
//    else if (request->method() == HTTP_DELETE)
//      Serial.print("DELETE");
//    else if (request->method() == HTTP_PUT)
//      Serial.print("PUT");
//    else if (request->method() == HTTP_PATCH)
//      Serial.print("PATCH");
//    else if (request->method() == HTTP_HEAD)
//      Serial.print("HEAD");
//    else if (request->method() == HTTP_OPTIONS)
//      Serial.print("OPTIONS");
//    else
//      Serial.print("UNKNOWN");
//    Serial.println(" http://" + request->host() + request->url());
//
//    if (request->contentLength()) {
//      Serial.println("_CONTENT_TYPE: " + request->contentType());
//      Serial.println("_CONTENT_LENGTH: " + request->contentLength());
//    }
//
//    int headers = request->headers();
//    int i;
//
//    for (i = 0; i < headers; i++) {
//      AsyncWebHeader *h = request->getHeader(i);
//      Serial.println("_HEADER[" + h->name() + "]: " + h->value());
//    }
//
//    int params = request->params();
//
//    for (i = 0; i < params; i++) {
//      AsyncWebParameter *p = request->getParam(i);
//
//      if (p->isFile()) {
//        Serial.println("_FILE[" + p->name() + "]: " + p->value() +
//            ", size: " + p->size());
//      } else if (p->isPost()) {
//        Serial.println("_POST[" + p->name() + "]: " + p->value());
//      } else {
//        Serial.println("_GET[" + p->name() + "]: " + p->value());
//      }
//    }
//
//    request->send(404);
//  });
//
//  server.onFileUpload([](AsyncWebServerRequest *request, const String &filename,
//                         size_t index, uint8_t *data, size_t len, bool final) {
//    if (!index)
//      Serial.println("UploadStart: " + filename);
//
//    Serial.print((const char *)data);
//
//    if (final)
//      Serial.println("UploadEnd: " + filename + "(" + String(index + len) +
//          ")");
//  });
//
//  server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data,
//                          size_t len, size_t index, size_t total) {
//    if (!index)
//      Serial.println("BodyStart: " + total);
//
//    Serial.print((const char *)data);
//
//    if (index + len == total)
//      Serial.println("BodyEnd: " + total);
//  });
//
//  server.begin();
//
//  Serial.print("HTTP server started @ ");
//  Serial.println(WiFi.localIP());
//
//  Serial.println(separatorLine);
//  Serial.println("Open http://" + host + ".local/edit to see the file browser");
//  Serial.println("Using username = " + http_username +
//      " and password = " + http_password);
//  Serial.println(separatorLine);
//}