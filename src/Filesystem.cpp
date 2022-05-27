//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#include "Filesystem.hpp"
#include "RTCMemory.hpp"
#include <yal/yal.hpp>
#include <cstring>

namespace open_heat {

bool Filesystem::setup()
{
  if (m_setup) {
    return true;
  }

  // Format FileFS if not yet
#ifdef ESP32
  if (!FileFS.begin(true))
#else
  if (!FileFS.begin())
#endif
  {
    m_logger.log(yal::Level::WARNING, "% begin failed, formatting...", FS_Name);
#ifdef ESP8266
    if (!m_filesystem->format()) {
      m_logger.log(yal::Level::ERROR, "Format of fs failed!");
    }
#endif
  }

  m_logger.log(yal::Level::DEBUG, "FS setup done");

  auto configValid = isConfigValid();
  if (configValid) {
    initConfig();
  } else {
    clearConfig();
  }

  m_setup = true;
  return configValid;
}

void Filesystem::listFiles()
{
  Dir dir = m_filesystem->openDir("/");
  m_logger.log(yal::Level::DEBUG, "Opening / directory");

  while (dir.next()) {
    const String fileName = dir.fileName();
    const size_t fileSize = dir.fileSize();
    m_logger.log(
      yal::Level::DEBUG,
      "FS File: %, size: %",
      fileName.c_str(),
      formatBytes(fileSize).c_str());
  }
}

Config& Filesystem::getConfig()
{
  if (!m_setup) {
    setup();
  }
  return m_config;
}

void Filesystem::clearConfig()
{
  m_config = {};
  std::memset(&m_config.WifiCredentials, 0, sizeof(m_config.WifiCredentials));
  m_config.MQTT = {};
  m_config.Update = {};
}

void Filesystem::persistConfig()
{
  if (!m_setup) {
    setup();
  }

  File file = FileFS.open(configFile_, "w");
  m_logger.log(yal::Level::DEBUG, "Saving config");

  if (!file) {
    m_logger.log(yal::Level::ERROR, "Failed to create config file on FS");
    return;
  }

  file.write((uint8_t*)&m_config, sizeof(Config));

  file.close();

  m_logger.log(yal::Level::DEBUG, "Configuration saved");
}

void Filesystem::initConfig()
{
  File file = FileFS.open(configFile_, "r");
  m_logger.log(yal::Level::DEBUG, "Loading config");
  clearConfig();

  if (!file) {
    m_logger.log(yal::Level::ERROR, "Failed to create read config from FS");
    return;
  }

  file.readBytes((char*)&m_config, sizeof(Config));

  file.close();
  if (0 == std::strlen(m_config.Hostname)) {
    std::strcpy(m_config.Hostname, DEFAULT_HOST_NAME);
  }

  if (0 == std::strlen(m_config.Update.Username)) {
    std::strcpy(m_config.Update.Username, DEFAULT_USER);
  }

  if (0 == std::strlen(m_config.Update.Password)) {
    std::strcpy(m_config.Update.Username, DEFAULT_PW);
  }

  const auto topic = std::string(m_config.MQTT.Topic);
  if (topic.size() > 1 && topic[topic.size() - 1] != '/') {
    std::strcpy(m_config.MQTT.Topic, (topic + "/").c_str());
  }

  m_logger.log(yal::Level::DEBUG, "Successfully loaded config");
}

bool Filesystem::isConfigValid()
{
  const File file = FileFS.open(configFile_, "r");
  if (file.size() != sizeof(Config)) {
    m_logger.log(
      yal::Level::ERROR, "Config layout changed, invalidating and new config necessary");
    return false;
  }

  return true;
}
void Filesystem::format()
{
  m_filesystem->format();
}

String Filesystem::formatBytes(size_t bytes)
{
  static constexpr const auto bitsPerByte = 1024.0;
  if (bytes <bitsPerByte ) {
    return String(bytes) + "B";
  }
  if (bytes < (bitsPerByte * bitsPerByte)) {
    return String(bytes / bitsPerByte) + "KB";
  }
  return String(bytes / bitsPerByte / bitsPerByte) + "MB";
}

} // namespace open_heat
