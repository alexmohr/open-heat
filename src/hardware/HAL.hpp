//
// Copyright (c) 2020 Alexander Mohr 
// Licensed under the terms of the MIT license
//
#ifndef HAL_HPP_
#define HAL_HPP_

#if !( defined(ESP8266) ||  defined(ESP32) )
#error This code is intended to run on the ESP8266 or ESP32 platform! Please check your Tools->Board setting.
#endif


#ifdef ESP32
#include "ESP32.h"
#else
#include "ESP8266.h"
#endif

#define WIFI_MULTI_CONNECT_WAITING_MS           100L


#include "DoubleResetDetector.hpp"
#include "pins.h"

#endif //HAL_HPP_
