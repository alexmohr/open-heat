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
    (std::strlen(config.WifiCredentials.wifi_ssid) == 0)
    || (std::strlen(config.WifiCredentials.wifi_pw) < MIN_AP_PASSWORD_SIZE)) {
    Logger::log(Logger::DEBUG, "Wifi config is invalid");
    return false;
  }

  Logger::log(
    Logger::Level::TRACE,
    "Wifi config is valid: SSID: %s, PW: %s",
    config.WifiCredentials.wifi_ssid,
    config.WifiCredentials.wifi_pw);

  wifiMulti_.addAP(config.WifiCredentials.wifi_ssid, config.WifiCredentials.wifi_pw);

  return true;
}

bool WifiManager::showConfigurationPortal()
{
  Logger::log(Logger::DEBUG, "Starting access point");

  DNSServer dnsServer;
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);

  // SSID and PW for Config Portal
  const String ssid = "OpenHeat_ESP_" + String(ESP_getChipId(), HEX);
  const char* password = "OpenHeat";
  WiFi.softAP(ssid, password);

  const auto hostname = "OpenHeat";
  WiFi.setHostname(hostname);

  IPAddress ip = WiFi.softAPIP();
  Logger::log(Logger::INFO, "IP address: %s", ip.toString().c_str());

  const auto dnsPort = 53;
  if (!dnsServer.start(dnsPort, "*", WiFi.softAPIP())) {
    // No socket available
    Logger::log(Logger::ERROR, "Can't start dns server");
  }

  Logger::log(Logger::INFO, "Starting Config Portal");
  webServer_.setup(hostname);

  Logger::log(Logger::INFO, "Waiting for user configuration");
  while (true) {
    dnsServer.processNextRequest();
    delay(100);
  }
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

uint8_t WifiManager::connectMultiWiFi()
{
  WiFi.forceSleepWake();
  Logger::log(Logger::INFO, "Connecting MultiWifi...");

  // STA = client mode
  WiFi.mode(WIFI_STA);

  int i = 0;
  uint8_t status = wifiMulti_.run();

  delay(WIFI_MULTI_1ST_CONNECT_WAITING_MS);

  while ((i++ < 10) && (status != WL_CONNECTED)) {
    status = wifiMulti_.run();

    if (status == WL_CONNECTED) {
      break;
    }
    delay(WIFI_MULTI_CONNECT_WAITING_MS);
  }

  auto connectTime
    = (i * WIFI_MULTI_CONNECT_WAITING_MS) + WIFI_MULTI_1ST_CONNECT_WAITING_MS;

  if (status == WL_CONNECTED) {
    //@formatter:off
    Logger::log(
      Logger::INFO,
      "Wifi connected:\n"
      "\tTime: %i\n"
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
    Logger::log(Logger::WARNING, "WiFi connect timeout: %i", connectTime);
  }

  return status;
}

} // namespace network
} // namespace open_heat
