//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#include "Filesystem.hpp"
#include "Logger.hpp"
#include "RTCMemory.hpp"
#include <cstring>

namespace open_heat {

void Filesystem::setup()
{
  if (setup_) {
    return;
  }

  // Format FileFS if not yet
#ifdef ESP32
  if (!FileFS.begin(true))
#else
  if (!FileFS.begin())
#endif
  {
    Logger::log(Logger::WARNING, "%s begin failed, formatting...", FS_Name);
#ifdef ESP8266
    if (!filesystem->format()) {
      Logger::log(Logger::ERROR, "Format of fs failed!");
    }
#endif
  }

  Logger::log(Logger::DEBUG, "FS setup done");

  if (isConfigValid()) {
    initConfig();
  } else {
    clearConfig();
  }

  setup_ = true;
}

void Filesystem::listFiles()
{
  Dir dir = filesystem->openDir("/");
  Logger::log(Logger::DEBUG, "Opening / directory");

  while (dir.next()) {
    const String fileName = dir.fileName();
    const size_t fileSize = dir.fileSize();
    Logger::log(
      Logger::DEBUG,
      "FS File: %s, size: %s",
      fileName.c_str(),
      Logger::formatBytes(fileSize).c_str());
  }
}

Config& Filesystem::getConfig()
{
  if (!setup_) {
    setup();
  }
  return config_;
}

void Filesystem::clearConfig()
{
  config_ = {};
  std::memset(&config_.WifiCredentials, 0, sizeof(config_.WifiCredentials));
  config_.MQTT = {};
  config_.Update = {};
}

void Filesystem::persistConfig()
{
  if (!setup_) {
    setup();
  }

  File file = FileFS.open(configFile_, "w");
  Logger::log(Logger::DEBUG, "Saving config");

  if (!file) {
    Logger::log(Logger::ERROR, "Failed to create config file on FS");
    return;
  }

  file.write((uint8_t*)&config_, sizeof(Config));

  file.close();

  auto rtcMem = readRTCMemory();

  Logger::log(Logger::DEBUG, "Configuration saved");
}

void Filesystem::initConfig()
{
  File file = FileFS.open(configFile_, "r");
  Logger::log(Logger::DEBUG, "Loading config");
  clearConfig();

  if (!file) {
    Logger::log(Logger::ERROR, "Failed to create read config from FS");
    return;
  }

  file.readBytes((char*)&config_, sizeof(Config));

  file.close();
  if (0 == std::strlen(config_.Hostname)) {
    std::strcpy(config_.Hostname, DEFAULT_HOST_NAME);
  }

  if (0 == std::strlen(config_.Update.Username)) {
    std::strcpy(config_.Update.Username, DEFAULT_USER);
  }

  if (0 == std::strlen(config_.Update.Password)) {
    std::strcpy(config_.Update.Username, DEFAULT_PW);
  }

  const auto topic = std::string(config_.MQTT.Topic);
  if (topic.size() > 1 && topic[topic.size() - 1] != '/') {
    std::strcpy(config_.MQTT.Topic, (topic + "/").c_str());
  }

  auto rtcMem = readRTCMemory();
  Logger::log(Logger::DEBUG, "Successfully loaded config");
}

bool Filesystem::isConfigValid()
{
  const File file = FileFS.open(configFile_, "r");
  if (file.size() != sizeof(Config)) {
    Logger::log(
      Logger::ERROR, "Config layout changed, invalidating and new config necessary");
    return false;
  }

  return true;
}
void Filesystem::format()
{
  filesystem->format();
}

} // namespace open_heat
