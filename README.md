# UWF Capstone: Smart Solar Water Heater
Program for smart system

## [MATTER Docs](https://docs.espressif.com/projects/esp-matter/en/latest/esp32/index.html)

## [ESP32 API Docs](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/index.html)

## Pinout
### ESP32
![Pinout](ESP32_pinout.jpg)

#### Used Pins
- 4 [IN]: Temp Sensor OneWire Data
- 6 [IN]: Inclinometer
- 31 [OUT]: Pump Relay
- 26 [OUT]: Arm Enable
- 25 [OUT]: Arm Extend
- 24 [OUT]: Arm Retract

### Temp Sensor
- Red: VCC
- Black - GND
- Yellow - Data
- Pullup Resistor - 4.7k ohm Data/VCC

### H-Bridge Motor Driver - Linear Actuator "Arm"
- ENA: Enable motor
- IN1: Direction 1
- IN2: Direction 2

## Adruino IDE Setup

Preferences > Additional boards manager URLs:
 - `https://dl.espressif.com/dl/package_esp32_index.json`

Boards Manager
 -  **esp32** by Espressif Systems `3.3.6`

Library Manager
 - **esp32-ds18b20** by junkfix `2.0.3`

Select Board **ESP32 Dev Module**

## TODO
 - [ ] Read sensors
 - - [ ] [esp32-ds18b20 Library](https://github.com/DavidAntliff/esp32-ds18b20?tab=readme-ov-file)
 - [ ] Start matter connection
 - [ ] Send sensor data to matter server
 - [ ] Pump on timer and/or sensor control
 - [ ] Tracking Acutator 15 deg/hr based on solar noon array
 - - [ ] need local time somehow (WiFi? Builtin?)
 - - [ ] use inclinometer / gyroscope for actual angle
