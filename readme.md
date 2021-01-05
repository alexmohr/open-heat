# Open Heat
This projects provides a firmware for an ESP8266 to control radiator valves.

## Status 
Not usable yet!

## Hardware
  * ESP8266
(ESP32 can be supported in the future)
  * [Radiator Valve](https://www.amazon.de/-/en/Eqiva-Bluetooth-Smart-Radiator-Thermostat/dp/B085LW2K1M/ref=sr_1_1?crid=3I212STBS18JX&dchild=1&keywords=eqiva%2Bheizk%C3%B6rperthermostat&qid=1609525097&sprefix=eqiva%2Caps%2C280&sr=8-1&th=1)
    * The linked valve is taken apart and only the motor and temperature sensor is used.
    Therefore the cheapest version without support for bluetooth for wifi is sufficient
  * BME280
    * This can be used instead the temperature sensor from the Eqiva valve.
    Using this requires less intricate soldering.

## Building
Run `init.sh` to properly initialize the project. 
To build run 
```
platformio -c clion run --target release -e nodemcuv2
```

## Updating
OTA updates will be supported.|

## Contributions
As this project is still in a very early stage no contributions will be accpeted at the moment.

Copyright 2020 Alexander Mohr, MIT

