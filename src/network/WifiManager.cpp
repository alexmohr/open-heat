//
// Copyright (c) 2020 Alexander Mohr
// Licensed under the terms of the MIT license
//
#include "WifiManager.hpp"
#include <Logger.hpp>
#include <string>

namespace open_heat {
namespace network {

void WifiManager::setup()
{
  digitalWrite(LED_BUILTIN, LED_ON);

  ESPAsync_WiFiManager espWifiManager(&webServer_, dnsServer_, HOST_NAME);

  // Check if any config is valid.
  bool startConfigPortal = !loadAPsFromConfig();
  if (drd_->detectDoubleReset()) {
    Logger::log(Logger::WARNING, ">>> Detected double reset <<<");
    startConfigPortal = true;
  }

  if (startConfigPortal || connectMultiWiFi() != WL_CONNECTED) {
    // Starts an access point
    while (!showConfigurationPortal(&espWifiManager)) {
      Logger::log(Logger::WARNING, "Configuration did not yield valid wifi, retrying");
    }
  }

  checkWifi();

  digitalWrite(LED_BUILTIN, LED_OFF);
}

bool WifiManager::loadAPsFromConfig()
{
  bool anyConfigValid = false;
  for (uint8_t i = 0; i < NUM_WIFI_CREDENTIALS; i++) {
    // Don't permit NULL SSID and password len < // MIN_AP_PASSWORD_SIZE (8)
    auto& config = filesystem_->getConfig();
    if (
      (String(config.WifiCredentials[i].wifi_ssid) == "")
      || (strlen(config.WifiCredentials[i].wifi_pw) < MIN_AP_PASSWORD_SIZE)) {
      Logger::log(Logger::INFO, "Wifi config in slot %i is not valid", i);
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
  String ssid = "OpenHeat_ESP_" + String(ESP_getChipId(), HEX);
  const char* password = "OpenHeat";

  espWifiManager->setConfigPortalChannel(0);
  espWifiManager->setConfigPortalTimeout(0);
  auto& config = filesystem_->getConfig();

  wifiMulti_.cleanAPlist();
  espWifiManager->startConfigPortal(ssid.c_str(), password);

  for (uint8_t i = 0; i < NUM_WIFI_CREDENTIALS; i++) {
    String tempSSID = espWifiManager->getSSID(i);
    String tempPW = espWifiManager->getPW(i);

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

  // Clear settings - MQTT
  memset(&config.MQTT.Server, 0, sizeof(config.MQTT.Server));
  memset(&config.MQTT.Topic, 0, sizeof(config.MQTT.Topic));
  memset(&config.MQTT.Username, 0, sizeof(config.MQTT.Username));
  memset(&config.MQTT.Password, 0, sizeof(config.MQTT.Password));

  // Clear settings - Update
  memset(&config.Update.Username, 0, sizeof(config.Update.Username));
  memset(&config.Update.Password, 0, sizeof(config.Update.Password));

  // Update settings - MQTT
  strcpy(config.MQTT.Server, paramMqttServer_.getValue());
  strcpy(config.MQTT.Topic, paramMqttTopic_.getValue());
  strcpy(config.MQTT.Username, paramMqttUsername_.getValue());
  strcpy(config.MQTT.Password, paramMqttPassword_.getValue());

  // Update settings - Update
  strcpy(config.Update.Username, paramUpdateUsername_.getValue());
  strcpy(config.Update.Password, paramUpdatePassword_.getValue());

  char* pEnd;
  const int newPort = std::strtol(paramMqttPortString_.getValue(), &pEnd, 10);
  config.MQTT.Port = newPort == 0 ? MQTT_DEFAULT_PORT : newPort;

  filesystem_->persistConfig();
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

      memset(&config.WifiCredentials[i], 0, sizeof(WiFi_Credentials));
      strcpy((config.WifiCredentials[i].wifi_ssid), tempSSID.c_str());
      strcpy((config.WifiCredentials[i].wifi_pw), tempPW.c_str());
    }
    Logger::log(
      Logger::DEBUG,
      "WiFi config slot %i: SSID: %s, PW: %s",
      i,
      config.WifiCredentials[i].wifi_ssid,
      config.WifiCredentials[i].wifi_pw);
  }
}

void WifiManager::loop()
{
  ulong currentMillis = millis();

  // Check WiFi periodically.
  if (currentMillis > nextWifiCheckMillis_) {
    checkWifi();
    nextWifiCheckMillis_ = currentMillis + checkInterval_.count();
  }
}

void WifiManager::checkWifi()
{
  if (WiFi.status() == WL_CONNECTED){
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

  auto connectTime = (i * WIFI_MULTI_CONNECT_WAITING_MS) + WIFI_MULTI_1ST_CONNECT_WAITING_MS;

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

  // MQTT
  paramMqttServer_.getWMParam_Data(paramData);
  strcpy(paramData._value, config.MQTT.Server);
  paramMqttServer_.setWMParam_Data(paramData);

  paramMqttPortString_.getWMParam_Data(paramData);
  strcpy(paramData._value, String(config.MQTT.Port).c_str());
  paramMqttPortString_.setWMParam_Data(paramData);

  paramMqttTopic_.getWMParam_Data(paramData);
  strcpy(paramData._value, config.MQTT.Topic);
  paramMqttTopic_.setWMParam_Data(paramData);

  paramMqttUsername_.getWMParam_Data(paramData);
  strcpy(paramData._value, config.MQTT.Username);
  paramMqttUsername_.setWMParam_Data(paramData);

  paramMqttPassword_.getWMParam_Data(paramData);
  strcpy(paramData._value, config.MQTT.Password);
  paramMqttPassword_.setWMParam_Data(paramData);

  // Update
  paramUpdateUsername_.getWMParam_Data(paramData);
  strcpy(paramData._value, config.Update.Username);
  paramUpdateUsername_.setWMParam_Data(paramData);

  paramUpdatePassword_.getWMParam_Data(paramData);
  strcpy(paramData._value, config.Update.Password);
  paramUpdatePassword_.setWMParam_Data(paramData);
}

} // namespace network
} // namespace open_heat
