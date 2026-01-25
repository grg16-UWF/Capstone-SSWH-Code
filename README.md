# UWF Capstone: Smart Solar Water Heater
Program for smart system

## [MATTER Docs](https://docs.espressif.com/projects/esp-matter/en/latest/esp32/index.html)

## [ESP32 API Docs](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/index.html)

## Pinout
![Pinout](ESP32_pinout.jpg)

## TODO
 - [ ] Read sensors
 - - [ ] [esp32-ds18b20 Library](https://github.com/DavidAntliff/esp32-ds18b20?tab=readme-ov-file)
 - [ ] Start matter connection
 - [ ] Send sensor data to matter server
 - [ ] Pump on timer and/or sensor control
 - [ ] Tracking Acutator 15 deg/hr based on solar noon array
 - - [ ] need local time somehow (WiFi? Builtin?)