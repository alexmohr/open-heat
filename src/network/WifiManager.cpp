//
// Copyright (c) 2020 Alexander Mohr
// Licensed under the terms of the MIT license
//
#include "WifiManager.hpp"
#include <Logger.hpp>

namespace open_heat {
namespace network {

void WifiManager::setup() {

  // Indicates whether ESP has WiFi credentials saved from previous session, or
  // double reset detected
  bool initialConfig = false;

  digitalWrite(LED_BUILTIN, LED_ON);

  ESPAsync_WiFiManager espWifiManager(webServer_, dnsServer_, "OpenHeat");

  String ssid = espWifiManager.WiFi_SSID();
  String passwd = espWifiManager.WiFi_Pass();
  if ((ssid == "") || (passwd == "")) {
    Logger::log(Logger::INFO, "No AP credentials");
    initialConfig = true;
  }

  if (drd_->detectDoubleReset()) {
    Logger::log(Logger::INFO, "Detected double reset");
    initialConfig = true;
  }

  if (initialConfig) {
    // Starts an access point
    setupConfigAP(&espWifiManager);
    saveConfigToFS();
  } else {
    // Load stored data, the addAP ready for MultiWiFi reconnection
    loadConfigFromFS();

    for (uint8_t i = 0; i < NUM_WIFI_CREDENTIALS; i++) {
      addAccessPointFromConfig(i);
    }

    if (WiFi.status() != WL_CONNECTED) {
      connectMultiWiFi();
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    String ip = WiFi.localIP().toString();
    Logger::log(Logger::INFO, "Connected to wifi, local ip: %s", ip.c_str());
  } else {
    Logger::log(Logger::WARNING, "Not connected: %s",
                espWifiManager.getStatus(WiFi.status()));
  }

  digitalWrite(LED_BUILTIN, LED_OFF);
}

void WifiManager::addAccessPointFromConfig(uint8_t i) {
  // Don't permit NULL SSID and password len < // MIN_AP_PASSWORD_SIZE (8)
  if ((String(wmConfig_->WiFi_Creds[i].wifi_ssid) == "") ||
      (strlen(wmConfig_->WiFi_Creds[i].wifi_pw) < MIN_AP_PASSWORD_SIZE)) {
    return;
  }

  Logger::log(Logger::Level::TRACE, "Adding SSID = %s, PW = %s",
              wmConfig_->WiFi_Creds[i].wifi_ssid,
              wmConfig_->WiFi_Creds[i].wifi_pw);

  wifiMulti_->addAP(wmConfig_->WiFi_Creds[i].wifi_ssid,
                    wmConfig_->WiFi_Creds[i].wifi_pw);
}

void WifiManager::setupConfigAP(ESPAsync_WiFiManager *espWifiManager) {
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

  // SSID and PW for Config Portal
  String ssid = "OpenHeat_ESP_" + String(ESP_getChipId(), HEX);
  const char *password = "OpenHeat";

  espWifiManager->setConfigPortalChannel(0);
  espWifiManager->setConfigPortalTimeout(0);
  if (!espWifiManager->startConfigPortal(ssid.c_str(), password)) {
    Serial.println(F("Not connected to WiFi"));
  } else {
    Serial.println(F("connected"));
  }

  // Clear old configuration
  memset(&wmConfig_, 0, sizeof(WM_Config));

  for (uint8_t i = 0; i < NUM_WIFI_CREDENTIALS; i++) {
    updateConfig(espWifiManager, i);
    addAccessPointFromConfig(i);
  }
}

void WifiManager::updateConfig(ESPAsync_WiFiManager *espWifiManager,
                               uint8_t i) const {
  String tempSSID = espWifiManager->getSSID(i);
  String tempPW = espWifiManager->getPW(i);

  if (strlen(tempSSID.c_str()) <
      sizeof(wmConfig_->WiFi_Creds[i].wifi_ssid) - 1) {
    strcpy(wmConfig_->WiFi_Creds[i].wifi_ssid, tempSSID.c_str());
  } else {
    strncpy(wmConfig_->WiFi_Creds[i].wifi_ssid, tempSSID.c_str(),
            sizeof(wmConfig_->WiFi_Creds[i].wifi_ssid) - 1);
  }

  if (strlen(tempPW.c_str()) < sizeof(wmConfig_->WiFi_Creds[i].wifi_pw) - 1) {
    strcpy(wmConfig_->WiFi_Creds[i].wifi_pw, tempPW.c_str());
  } else {
    strncpy(wmConfig_->WiFi_Creds[i].wifi_pw, tempPW.c_str(),
            sizeof(wmConfig_->WiFi_Creds[i].wifi_pw) - 1);
  }
}

void WifiManager::saveConfigToFS() {
  File file = FileFS.open(wifiConfigFile_, "w");
  Logger::log(Logger::DEBUG, "Saving wifi config");

  if (!file) {
    Logger::log(Logger::ERROR, "Failed to create config file on FS");
    return;
  }

  file.write((uint8_t *)&wmConfig_, sizeof(WM_Config));
  file.write((uint8_t *)&staticIpConfig_, sizeof(WiFi_STA_IPConfig));

  file.close();
  Logger::log(Logger::DEBUG, "Wifi config saved");
}

void WifiManager::loadConfigFromFS() {}

void WifiManager::checkStatus() {
  static ulong current_millis = millis();

  // Check WiFi periodically.
  if (current_millis > lastWifiCheckMillis_) {
    checkWifi();
    lastWifiCheckMillis_ = current_millis + checkInterval_.count();
  }
}

void WifiManager::checkWifi() {
  if ((WiFi.status() != WL_CONNECTED)) {
    Logger::log(Logger::WARNING, "WIFI disconnected, reconnecting...");
    connectMultiWiFi();
  }
}

uint8_t WifiManager::connectMultiWiFi() {
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
  uint8_t status = wifiMulti_->run();
  delay(WIFI_MULTI_1ST_CONNECT_WAITING_MS);

  while ((i++ < 10) && (status != WL_CONNECTED)) {
    status = wifiMulti_->run();

    if (status == WL_CONNECTED) {
      break;
    }

    delay(WIFI_MULTI_CONNECT_WAITING_MS);
  }

  if (status == WL_CONNECTED) {
    //@formatter:off
    Logger::log(Logger::INFO,
                "WiFi connected after time: %i\n"
                "SSID: %s\n"
                "RSSI=%s\n"
                "Channel: %s\n"
                "IP address: %s",
                (i * WIFI_MULTI_CONNECT_WAITING_MS) +
                    WIFI_MULTI_1ST_CONNECT_WAITING_MS,
                WiFi.SSID().c_str(), WiFi.RSSI(), WiFi.channel(),
                WiFi.localIP().toString().c_str());
    //@formatter:on
  } else {
    Logger::log(Logger::WARNING, "Could not connect to wifi in time!");
  }

  return status;
}

void WifiManager::initSTAIPConfigStruct(WiFi_STA_IPConfig &ipConfig) {
  ipConfig._sta_static_ip = stationIP;
  ipConfig._sta_static_gw = gatewayIP;
  ipConfig._sta_static_sn = netMask;
#if USE_CONFIGURABLE_DNS
  ipConfig._sta_static_dns1 = dns1IP;
  ipConfig._sta_static_dns2 = dns2IP;
#endif
}

} // namespace network
} // namespace open_heat
