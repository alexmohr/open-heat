# Open Heat
![BPlatformIO CI](https://github.com/alexmohr/open-heat/workflows/PlatformIO%20CI/badge.svg)

This projects provides a firmware for an ESP8266 to control radiator valves.

## Status 
Early alpha

## Hardware
  * ESP8266
(ESP32 can be supported in the future)
  * [Radiator Valve](https://www.amazon.de/-/en/Eqiva-Bluetooth-Smart-Radiator-Thermostat/dp/B085LW2K1M/ref=sr_1_1?crid=3I212STBS18JX&dchild=1&keywords=eqiva%2Bheizk%C3%B6rperthermostat&qid=1609525097&sprefix=eqiva%2Caps%2C280&sr=8-1&th=1)
    * The linked valve is taken apart and only the motor and temperature sensor is used.
    Therefore the cheapest version without support or bluetooth for wifi is sufficient
  * BME280
    * This can be used instead the temperature sensor from the Eqiva valve.
    Using this requires less intricate soldering.

### Pin configuration
Currently set via `platformio.ini`, will be added to configuration later.

## Building
Run `init.sh` to properly initialize the project. 
To build run 
```
platformio -c clion run --target release -e nodemcuv2
```

## Updating
Download the latest firmware from releases and upload it on the web-ui.
Make sure to download the correct version, otherwise your MCU has to be flashed via USB again.

## MQTT 
The heater can be controlled via mqtt and integrated into home assistant.
It offers the following topics, all of them are prefixed with the configured topic (`$TOPIC`):
* Set target temp: `$TOPIC/temperature/target/set`
* Get target temp: `$TOPIC/temperature/target/get`
* Get measured temp: `$TOPIC/temperature/measured/get`
* Get current mode (can be off or heating): `$TOPIC/mode/get`
* Set current mode (can be off or heating): `$TOPIC/mode/set`

Example configuration for home assistant:
```yaml
climate:
  - platform: mqtt
    modes:
      - "off"
      - "heat"
    name: Living room
    temperature_command_topic: "living_room/front/temperature/target/set"
    temperature_state_topic: "living_room/front/temperature/target/get"
    current_temperature_topic: "living_room/front/temperature/measured/get"
    mode_command_topic: "living_room/front/mode/set"
    mode_state_topic: "living_room/front/mode/get"

```

## Receiving logs 
### Serial
``platformio -c clion device monitor -e nodemcuv2 -f esp8266_exception_decoder``

### MQTT 
Subscribe to topic `$CONFIGURED_TOPIC/log`

### Browser
Logs are displayed in the web ui at the bottom of the page.

## Installation

## Contributions
As this project is still in a very early stage no contributions will be accepted at the moment.

## Attributions and libraries
Special thanks to traumflug and their regulator implementation in [ISTAtrol](https://github.com/Traumflug/ISTAtrol/blob/master/firmware/main.c).
Parts of the regulator algorithm where taken from there and adapted to this project.

### Libraries
* [ESP Async WebServer](https://github.com/me-no-dev/ESPAsyncWebServer)
* [ESP AsyncTCP](https://github.com/me-no-dev/ESPAsyncTCP)
* [AsyncTCP](https://github.com/me-no-dev/AsyncTCP)
* [ESP_DoubleResetDetector](https://github.com/khoih-prog/ESP_DoubleResetDetector)
* [ESPAsync_WiFiManager](https://github.com/khoih-prog/ESPAsync_WiFiManager)
* [Adafruit BME280 Library](https://github.com/adafruit/Adafruit_BME280_Library/)
* [MQTT](https://github.com/256dpi/arduino-mqtt)


