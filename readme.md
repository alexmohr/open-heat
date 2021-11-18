# Open Heat
![BPlatformIO CI](https://github.com/alexmohr/open-heat/workflows/PlatformIO%20CI/badge.svg)

This projects provides a firmware for an ESP8266 to control radiator valves.


## Status 
Early beta

## Hardware
  * ESP8266
  * [Radiator Valve](https://www.amazon.de/-/en/Eqiva-Bluetooth-Smart-Radiator-Thermostat/dp/B085LW2K1M/)
    * The linked valve is taken apart and only the motor and temperature sensor is used.
    Therefore the cheapest version without support or bluetooth for wifi is sufficient
  * BME280
    * This can be used instead the temperature sensor from the Eqiva valve.
    Using this requires less intricate soldering.
  * HT7333 voltage regulator

### Pin configuration
Pins can be configured via the webinterface.
The current defaults are:

| Function | Ground  | VIN     |
|----------|---------|---------|
| Motor    | D6 (12) | D5 (14) |
| Window   | D8 (15) | D7 (13) |

### Battery 
To extend the battery lifetime this project is using the following batteries 

Battery holder:
* https://de.aliexpress.com/item/32969695165.html

Battery:
* https://de.aliexpress.com/item/1005003394481523.html

To monitor their voltage the ADC of the ESP is used. 
This requires to solder a voltage divider, with 100k and 33k Ohm resistance. 
For details, see the schematics.
Please note that at this point it's not possible to configure the resistance
values via the web interface. 
Please note that a nodemcu already comes with a voltage divider for the ADC.
To use the battery management you have to de-solder these resistors

## Building
Run `init.sh` to properly initialize the project. 
To build run 
```
platformio -c clion run --target release -e nodemcuv2
```

## Setup
Connect to the WiFi OpenHeatESP... with password "OpenHeat".
Open 192.168.4.1 in your browser and start configuration. 
Save config and the ESP will reboot and connect to your Wifi.


## Uploading
```bash
esptool.py --before no_reset --after hard_reset --chip esp8266 --port "/dev/ttyUSB0" --baud 921600 write_flash 0x0 ".pio/build/nodemcuv2/firmware.bin"

```

## Updating
Download the latest firmware from releases and upload it on the web-ui.
Make sure to download the correct version, otherwise your MCU has to be flashed via USB again.

The initial configuration allows the configuration of a user name and password 
for update purposes. 
This is to improve security a bit and not anyone in your network can flash a new firmware. 
For security purposes this data only can be changed in the configuration mode 
and not via the webinterface. 
To enable the configuration mode again restart your device twice in 10s. 


## MQTT 
The heater can be controlled via mqtt and integrated into home assistant.
It offers the following topics, all of them are prefixed with the configured topic (`$TOPIC`):
* Set target temp: `$TOPIC/temperature/target/set`
* Get target temp: `$TOPIC/temperature/target/get`
* Get measured temp: `$TOPIC/temperature/measured/get`
* Get measured humidity: `$TOPIC/humidity/measured/get`
* Get battery percentage: `$TOPIC/battery/percentage`
* Get battery voltage: `$TOPIC/battery/voltage`
* Get current mode (can be off or heating): `$TOPIC/mode/get`
* Set current mode (can be off or heating): `$TOPIC/mode/set`
* Get current modem sleep time: `$TOPIC/modemsleep/get` (time is milliseconds)
* Set current modem sleep time: `$TOPIC/modemsleep/set` (time is milliseconds)
  * Be careful when setting this. 
  * If it's set to a small value the device will consume a lot of battery
  * If it's set to a large value you won't receive temperatures and battery 
    updates in a high frequency and you won't be able to change the operation mode
  * This feature is intended to set the sleep time overnight to something like 1h
    to save battery

The battery percentage assumes a voltage between 3.1 and 4.2 volts

Example configuration for home assistant:
```yaml
climate:
  - platform: mqtt
    modes:
      - "off"
      - "heat"
    name: Living room
    temperature_command_topic: "$TOPIC/temperature/target/set"
    temperature_state_topic: "$TOPIC/temperature/target/get"
    current_temperature_topic: "$TOPIC/temperature/measured/get"
    mode_command_topic: "$TOPIC/mode/set"
    mode_state_topic: "$TOPIC/mode/get"
    retain: true
```

optional if you also want to monitor the battery state:
```yaml
  - platform: mqtt
    state_topic: "$TOPIC/battery/voltage"
    name: "Bed Heater Voltage"

  - platform: mqtt
    state_topic: "$TOPIC/battery/percent"
    name: "Bed Heater Percent"

```

### Debugging
All commands must be sent with retain flag to allow the device to read the value 
as soon as it comes out of sleep

* Enable debugging and web interface, WARNING this consumes a lot more power is 
not intended to be used for battery operation:
  ``` 
  mosquitto_pub -h hassbian -t "$TOPIC/debug/enable" -m "true" -r
  ```
* Set log level
    ```
     mosquitto_pub -h hassbian -t "$TOPIC/debug/loglevel" -m "$LEVEL" -r   
    ```
  Level can be a value between 0 (highest log level = all logs) and 5 (lowest log level = only fatal errors)

* Receive logs
    ```
    mosquitto_sub -h hassbian -t "$TOPIC/log"
    ```

## Receiving logs 
### Serial
``platformio -c clion device monitor -e nodemcuv2 -f esp8266_exception_decoder``

### MQTT 
Subscribe to topic `$TOPIC/log`

### Browser
Logs are displayed in the web ui at the bottom of the page.

## Code analysis
````
cmake -DCMAKE_BUILD_TYPE=nodemcuv2 --CMAKE_EXPORT_COMPILE_COMMANDS=YES ..
scan-build-11 make
````

## Case 

The case is designed to hold to valve, a dual 18650 battery holder and an 60x40mm circuit board. 
At this moment the case has not been printed nor tested yet. 

## Contributions
As this project is still in a very early stage no contributions will be accepted at the moment.

## Attributions and libraries
* Big thanks to @Anycubic for continued bug reports, testing and several improvements and ideas
* Special thanks to traumflug and their regulator implementation in [ISTAtrol](https://github.com/Traumflug/ISTAtrol/blob/master/firmware/main.c).
  Although it was heavily modified and improved the I-Regulator still forms the base of the valve control.
* WiFi Manager is inspired by https://github.com/roberttidey/WiFiManager/blob/feature_fastconnect/WiFiManager.cpp
### Libraries
* [ESP Async WebServer](https://github.com/me-no-dev/ESPAsyncWebServer)
* [ESP AsyncTCP](https://github.com/me-no-dev/ESPAsyncTCP)
* [AsyncTCP](https://github.com/me-no-dev/AsyncTCP)
* [ESP_DoubleResetDetector](https://github.com/khoih-prog/ESP_DoubleResetDetector)
* [Adafruit BME280 Library](https://github.com/adafruit/Adafruit_BME280_Library/)
* [MQTT](https://github.com/256dpi/arduino-mqtt)


