//
// Copyright (c) 2021 Alexander Mohr
// Licensed under the terms of the GNU General Public License v3.0
//
#ifndef DOUBLERESETDETECTOR_HPP_
#define DOUBLERESETDETECTOR_HPP_

#define DOUBLERESETDETECTOR_DEBUG false

// Number of seconds after reset during which a
// subsequent reset will be considered a double reset.
#define DRD_TIMEOUT 10

// RTC Memory Address for the DoubleResetDetector to use
#define DRD_ADDRESS 0

// Double reset detector
#define ESP_DRD_USE_LITTLEFS true
#define ESP_DRD_USE_SPIFFS false
#define ESP8266_DRD_USE_RTC false

// must be at the bottom of this file
#include <ESP_DoubleResetDetector.h>

#endif // DOUBLERESETDETECTOR_HPP_
