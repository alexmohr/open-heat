//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#include "WifiManager.hpp"
#include <Logger.hpp>
#include <RTCMemory.hpp>
#include <cstring>

namespace open_heat {
namespace network {

void WifiManager::setup(bool doubleReset)
{
  // Check if any config is valid.
  auto startConfigPortal = !loadAPsFromConfig();
  if (doubleReset) {
    Logger::log(Logger::WARNING, ">>> Detected double reset <<<");
    startConfigPortal = true;
  }

  if (startConfigPortal || connectMultiWiFi() != WL_CONNECTED) {
    // Starts access point
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LED_ON);
    while (!showConfigurationPortal()) {
      Logger::log(Logger::WARNING, "Configuration did not yield valid wifi, retrying");
    }
    digitalWrite(LED_PIN, LED_OFF);
  }

  checkWifi();
}

bool WifiManager::loadAPsFromConfig()
{
  // Don't permit NULL SSID and password len < // MIN_AP_PASSWORD_SIZE (8)
  auto& config = filesystem_->getConfig();
  if (
    (std::strlen(config.WifiCredentials.ssid) == 0)
    || (std::strlen(config.WifiCredentials.password) < MIN_AP_PASSWORD_SIZE)) {
    Logger::log(Logger::DEBUG, "Wifi config is invalid");
    return false;
  }

  Logger::log(
    Logger::Level::TRACE,
    "Wifi config is valid: SSID: %s, PW: %s",
    config.WifiCredentials.ssid,
    config.WifiCredentials.password);

  return true;
}

[[noreturn]] bool WifiManager::showConfigurationPortal()
{
  auto accessPoints = getApList();
  Logger::log(Logger::DEBUG, "Starting access point");

  DNSServer dnsServer;
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);

  // SSID and PW for Config Portal
  const String ssid = "OpenHeat_ESP_" + String(ESP_getChipId(), HEX);
  const char* password = "OpenHeat";
  WiFi.softAP(ssid, password);

  const auto hostname = "OpenHeat";
  WiFi.setHostname(hostname);

  const auto ip = WiFi.softAPIP();
  Logger::log(Logger::INFO, "IP address: %s", ip.toString().c_str());

  const auto dnsPort = 53;
  if (!dnsServer.start(dnsPort, "*", WiFi.softAPIP())) {
    // No socket available
    Logger::log(Logger::ERROR, "Can't start dns server");
  }

  Logger::log(Logger::INFO, "Starting Config Portal");
  webServer_.setup(hostname);
  webServer_.setApList(std::move(accessPoints));

  Logger::log(Logger::INFO, "Waiting for user configuration");
  // WebServer will restart ESP after configuration is done
  while (true) {
    dnsServer.processNextRequest();
    delay(100);
  }
}
std::vector<String> WifiManager::getApList() const
{
  Logger::log(Logger::DEBUG, "Searching for available networks");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  const auto apCount = WiFi.scanNetworks();
  std::vector<String> accessPoints;
  for (auto i = 0; i < apCount; ++i) {
    const auto ssid = WiFi.SSID(i);
    Logger::log(Logger::DEBUG, "Found SSID '%s'", ssid.c_str());
    accessPoints.push_back(std::move(ssid));
  }

  return accessPoints;
}

bool WifiManager::checkWifi()
{
  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }

  Logger::log(Logger::WARNING, "WIFi disconnected, reconnecting...");
  if (connectMultiWiFi() == WL_CONNECTED) {
    reconnectCount_ = 0;
    return true;
  }

  Logger::log(Logger::WARNING, "WiFi reconnection failed, %i times", ++reconnectCount_);
  return false;
}

wl_status_t WifiManager::connectMultiWiFi()
{
  WiFi.forceSleepWake();
  Logger::log(Logger::INFO, "Connecting WiFi...");
  const auto startTime = rtc::offsetMillis();

  const auto& config = filesystem_->getConfig();

  // STA = client mode
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(config.Hostname);

  fastConfig connectConfig{};
  const auto hasFastConfig
    = getFastConnectConfig(config.WifiCredentials.ssid, connectConfig);
  wl_status_t status;
  if (hasFastConfig) {
    Logger::log(Logger::DEBUG, "Using fast connect");
    status = WiFi.begin(
      config.WifiCredentials.ssid,
      config.WifiCredentials.password,
      connectConfig.channel,
      connectConfig.bssid,
      true);
  } else {
    Logger::log(Logger::DEBUG, "Using standard connect");
    status = WiFi.begin(config.WifiCredentials.ssid, config.WifiCredentials.password);
  }

  int i = 0;
  while ((i++ < 60) && (status != WL_CONNECTED)) {
    delay(100);
    status = WiFi.status();
    if (status == WL_CONNECTED) {
      break;
    }
  }

  const auto connectTime = rtc::offsetMillis() - startTime;
  if (status == WL_CONNECTED) {
    //@formatter:off
    Logger::log(
      Logger::INFO,
      "Wifi connected:\n"
      "\ttime: %llu\n"
      "\tSSID: %s\n"
      "\tRSSI=%i\n"
      "\tChannel: %i\n"
      "\tIP address: %s",
      connectTime,
      WiFi.SSID().c_str(),
      WiFi.RSSI(),
      WiFi.channel(),
      WiFi.localIP().toString().c_str());
    //@formatter:on
  } else {
    Logger::log(Logger::WARNING, "WiFi connect timeout");
  }

  return status;
}

bool WifiManager::getFastConnectConfig(const String& ssid, fastConfig& config)
{
  // adopted from
  // https://github.com/roberttidey/WiFiManager/blob/feature_fastconnect/WiFiManager.cpp
  int networksFound = WiFi.scanNetworks();
  int32_t scan_rssi = -200;
  for (auto i = 0; i < networksFound; i++) {
    if (ssid == WiFi.SSID(i)) {
      if (WiFi.RSSI(i) > scan_rssi) {
        config.bssid = WiFi.BSSID(i);
        config.channel = WiFi.channel(i);
        return true;
      }
    }
  }
  return false;
}

} // namespace network
} // namespace open_heat
