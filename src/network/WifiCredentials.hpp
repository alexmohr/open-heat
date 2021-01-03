//
// Copyright (c) 2020 Alexander Mohr 
// Licensed under the terms of the MIT license
//
#ifndef WIFICREDENTIALS_HPP_
#define WIFICREDENTIALS_HPP_


#define MIN_AP_PASSWORD_SIZE    8
#define SSID_MAX_LEN            32
#define PASS_MAX_LEN            64

typedef struct {
  char wifi_ssid[SSID_MAX_LEN];
  char wifi_pw[PASS_MAX_LEN];
} WiFi_Credentials;

typedef struct {
  String wifi_ssid;
  String wifi_pw;
} WiFi_Credentials_String;

#define NUM_WIFI_CREDENTIALS      2



#endif //WIFICREDENTIALS_HPP_
