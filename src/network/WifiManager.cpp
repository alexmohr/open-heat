//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//

#include "WifiManager.hpp"
#include <Logger.hpp>
#include <cstring>

namespace open_heat {
namespace network {

void WifiManager::setup()
{
  const auto& config = filesystem_->getConfig();
  ESPAsync_WiFiManager espWifiManager(&webServer_, dnsServer_, config.Hostname);

  // Check if any config is valid.
  auto startConfigPortal = !loadAPsFromConfig();
  if (drd_->detectDoubleReset()) {
    Logger::log(Logger::WARNING, ">>> Detected double reset <<<");
    startConfigPortal = true;
  }

  if (startConfigPortal || connectMultiWiFi() != WL_CONNECTED) {
    // Starts an access point
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LED_ON);
    while (!showConfigurationPortal(&espWifiManager)) {
      Logger::log(Logger::WARNING, "Configuration did not yield valid wifi, retrying");
    }
    digitalWrite(LED_PIN, LED_OFF);
  }

  checkWifi();
}

bool WifiManager::loadAPsFromConfig()
{
  auto anyConfigValid = false;
  for (uint8_t i = 0; i < NUM_WIFI_CREDENTIALS; i++) {
    // Don't permit NULL SSID and password len < // MIN_AP_PASSWORD_SIZE (8)
    auto& config = filesystem_->getConfig();
    if (
      (std::strlen(config.WifiCredentials[i].wifi_ssid) == 0)
      || (std::strlen(config.WifiCredentials[i].wifi_pw) < MIN_AP_PASSWORD_SIZE)) {
      Logger::log(Logger::DEBUG, "Wifi config in slot %i is invalid", i);
      continue;
    }

    anyConfigValid = true;
    Logger::log(
      Logger::Level::TRACE,
      "Wifi config in slot %i is valid: SSID: %s, PW: %s",
      i,
      config.WifiCredentials[i].wifi_ssid,
      config.WifiCredentials[i].wifi_pw);

    wifiMulti_.addAP(
      config.WifiCredentials[i].wifi_ssid, config.WifiCredentials[i].wifi_pw);
  }

  return anyConfigValid;
}

bool WifiManager::showConfigurationPortal(ESPAsync_WiFiManager* espWifiManager)
{
  Logger::log(Logger::INFO, "Starting Config Portal");

#if !USE_DHCP_IP
  // Set (static IP, Gateway, Subnetmask, DNS1 and DNS2) or (IP, Gateway,
  // Subnetmask). New in v1.0.5 New in v1.4.0
  initSTAIPConfigStruct(staticIpConfig_);
  espWifiManager->setSTAStaticIPConfig(staticIpConfig_);
#endif

#if USING_CORS_FEATURE
  espWifiManager->setCORSHeader("Your Access-Control-Allow-Origin");
#endif

  initAdditionalParams();
  for (auto& param : additionalParameters_) {
    espWifiManager->addParameter(param);
  }

  // SSID and PW for Config Portal
  const String ssid = "OpenHeat_ESP_" + String(ESP_getChipId(), HEX);
  const char* password = "OpenHeat";

  espWifiManager->setConfigPortalChannel(0);
  espWifiManager->setConfigPortalTimeout(0);
  const auto& config = filesystem_->getConfig();

  wifiMulti_.cleanAPlist();
  espWifiManager->startConfigPortal(ssid.c_str(), password);

  for (uint8_t i = 0; i < NUM_WIFI_CREDENTIALS; i++) {
    const String tempSSID = espWifiManager->getSSID(i);
    const String tempPW = espWifiManager->getPW(i);

    // Re-insert old config if user did not enter new credentials
    if (tempSSID.isEmpty() && tempPW.isEmpty()) {
      Logger::log(Logger::DEBUG, "WiFi config slot %i restored from config", i);
      wifiMulti_.addAP(
        config.WifiCredentials[i].wifi_ssid, config.WifiCredentials[i].wifi_pw);
    } else {
      Logger::log(Logger::DEBUG, "WiFi config slot %i updated from portal", i);
      wifiMulti_.addAP(tempSSID.c_str(), tempPW.c_str());
    }
  }

  updateConfig(espWifiManager);
  return connectMultiWiFi() == WL_CONNECTED;
}

void WifiManager::updateConfig(ESPAsync_WiFiManager* espWifiManager)
{
  updateWifiCredentials(espWifiManager);

  auto& config = filesystem_->getConfig();
  clearSettings(config);
  updateSettings(config);
}

void WifiManager::updateSettings(Config& config)
{
  // Update
  std::strcpy(config.Update.Username, paramUpdateUsername_.getValue());
  std::strcpy(config.Update.Password, paramUpdatePassword_.getValue());

  filesystem_->persistConfig();
}

void WifiManager::clearSettings(Config& config)
{
  // Clear settings - Host
  std::memset(&config.Hostname, 0, sizeof(config.Hostname));

  // Clear settings - MQTT
  std::memset(&config.MQTT.Server, 0, sizeof(config.MQTT.Server));
  std::memset(&config.MQTT.Topic, 0, sizeof(config.MQTT.Topic));
  std::memset(&config.MQTT.Username, 0, sizeof(config.MQTT.Username));
  std::memset(&config.MQTT.Password, 0, sizeof(config.MQTT.Password));

  // Clear settings - Update
  std::memset(&config.Update.Username, 0, sizeof(config.Update.Username));
  std::memset(&config.Update.Password, 0, sizeof(config.Update.Password));
}

void WifiManager::updateWifiCredentials(ESPAsync_WiFiManager* espWifiManager) const
{
  auto& config = filesystem_->getConfig();
  String tempSSID;
  String tempPW;
  for (uint8_t i = 0; i < NUM_WIFI_CREDENTIALS; i++) {
    tempSSID.clear();
    tempPW.clear();

    tempSSID = espWifiManager->getSSID(i);
    tempPW = espWifiManager->getPW(i);

    auto ssidLen = tempSSID.length();
    auto pwLen = tempPW.length();
    if (
      (ssidLen <= SSID_MAX_LEN) && (pwLen <= PASS_MAX_LEN) && (ssidLen > 0)
      && (pwLen >= MIN_AP_PASSWORD_SIZE)) {

      std::memset(&config.WifiCredentials[i], 0, sizeof(WiFi_Credentials));
      std::strcpy((config.WifiCredentials[i].wifi_ssid), tempSSID.c_str());
      std::strcpy((config.WifiCredentials[i].wifi_pw), tempPW.c_str());
    }
    Logger::log(
      Logger::DEBUG,
      "WiFi config slot %i: SSID: %s, PW: %s",
      i,
      config.WifiCredentials[i].wifi_ssid,
      config.WifiCredentials[i].wifi_pw);
  }
}

void WifiManager::checkWifi()
{
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  Logger::log(Logger::WARNING, "WIFi disconnected, reconnecting...");
  if (connectMultiWiFi() == WL_CONNECTED) {
    reconnectCount_ = 0;
    return;
  }

  Logger::log(Logger::WARNING, "WiFi reconnection failed, %i times", ++reconnectCount_);
}

uint8_t WifiManager::connectMultiWiFi()
{
  Logger::log(Logger::INFO, "Connecting MultiWifi...");

  // STA = client mode
  WiFi.mode(WIFI_STA);

#if !USE_DHCP_IP
#if USE_CONFIGURABLE_DNS
  // Set static IP, Gateway, Subnetmask, DNS1 and DNS2. New in v1.0.5
  WiFi.config(stationIP, gatewayIP, netMask, dns1IP, dns2IP);
#else
  // Set static IP, Gateway, Subnetmask, Use auto DNS1 and DNS2.
  WiFi.config(stationIP, gatewayIP, netMask);
#endif
#endif

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

void WifiManager::initSTAIPConfigStruct(WiFi_STA_IPConfig& ipConfig)
{
  ipConfig._sta_static_ip = stationIP;
  ipConfig._sta_static_gw = gatewayIP;
  ipConfig._sta_static_sn = netMask;
#if USE_CONFIGURABLE_DNS
  ipConfig._sta_static_dns1 = dns1IP;
  ipConfig._sta_static_dns2 = dns2IP;
#endif
}

void WifiManager::initAdditionalParams()
{
  auto& config = filesystem_->getConfig();
  WMParam_Data paramData;

  // Host
  paramHostname_.getWMParam_Data(paramData);
  std::strcpy(paramData._value, config.Hostname);
  paramHostname_.setWMParam_Data(paramData);

  // Update
  paramUpdateUsername_.getWMParam_Data(paramData);
  std::strcpy(paramData._value, config.Update.Username);
  paramUpdateUsername_.setWMParam_Data(paramData);

  paramUpdatePassword_.getWMParam_Data(paramData);
  std::strcpy(paramData._value, config.Update.Password);
  paramUpdatePassword_.setWMParam_Data(paramData);
}

} // namespace network
} // namespace open_heat
