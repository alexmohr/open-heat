#!/usr/bin/env bash
platformio -c clion init --ide clion
platformio -c clion update

WIFI_MANAGER_SRC=".pio/libdeps/nodemcuv2/ESPAsync_WiFiManager/"
rm -rf "$WIFI_MANAGER_SRC/src"
cp -r "$WIFI_MANAGER_SRC/src_cpp" "$WIFI_MANAGER_SRC/src"