# GuitarAMI module firmware (Arduino IDE)

Firmware for the GuitarAMI module built with the Arduino IDE. It streams sensor data over Open Sound Control (OSC) at 100 Hz and uses the [Puara](https://github.com/Puara) framework for WiFi, configuration, and gesture extraction.

## Hardware

- TinyPICO (ESP32-PICO-D4)
- LSM9DS1 nine-axis IMU on I2C (module #003 uses a BNO080 instead: uncomment `GUITARAMI_IMU_BNO080` in `config.h`)
- HC-SR04+ or RCWL-1601 ultrasonic sensor (trigger on GPIO 32, echo on GPIO 33)
- Capacitive touch pad on GPIO 15
- LiPo battery read through the TinyPICO helper

## Build

Board manager: `esp32` by Espressif (tested with 3.3.11).

Tools menu:

- Board: **UM TinyPICO**
- Partition Scheme: **Minimal SPIFFS (Large APPS with OTA)**
- Core Debug Level: **None** (higher levels print an error for every packet that cannot be routed and break the 100 Hz timing)

Libraries (Library Manager):

| Library | Tested version |
| --- | --- |
| puara-module | 1.0.1 |
| puara-gestures | 1.0.0 |
| boost-embedded-190 | 1.90.1 (headers required by puara-gestures) |
| MicroOsc | 0.2.1 |
| SparkFun LSM9DS1 IMU | 2.0.0 |
| SparkFun BNO080 Cortex Based IMU | 1.1.12 |
| TinyPICO Helper Library | 1.5.0 |

## Filesystem

The `data/` folder holds the device configuration and the web pages served by puara-module. Upload it as a LittleFS image with the [arduino-littlefs-upload](https://github.com/earlephilhower/arduino-littlefs-upload) extension (Command Palette, "Upload LittleFS to Pico/ESP8266/ESP32").

- `config.json`: device name and id (they form the OSC namespace and the network name), WiFi credentials, access point password, `persistentAP`.
- `settings.json`: `oscIP1`, `oscPORT1`, `oscIP2`, `oscPORT2`, `localPORT`, `touchThreshold`. All of them can be changed at runtime from the settings page.

## Usage

The module connects to the network stored in `config.json` and always offers its own access point named after the device (default password `mappings`). The configuration pages are available at `http://GuitarAMI_module_XXX.local/` on either network. Firmware updates over the network use ArduinoOTA: the module appears as a network port in the Arduino IDE.

LED (TinyPICO DotStar):

- Dodger blue slow blink: connected to the configured WiFi network
- Lime breathing: not connected, use the module's own access point
- Red flicker: battery below 10 %

## OSC namespace

Continuous, 100 Hz:

- `/GuitarAMI_module_XXX/accl fff` (m/s^2)
- `/GuitarAMI_module_XXX/gyro fff` (rad/s)
- `/GuitarAMI_module_XXX/magn fff` (uT)
- `/GuitarAMI_module_XXX/quat ffff` (x, y, z, w)
- `/GuitarAMI_module_XXX/ypr fff` (degrees)
- `/GuitarAMI_module_XXX/touch i` (raw capacitive value)

On change:

- `/GuitarAMI_module_XXX/ult i` (mm, 0 when nothing is in range)
- `/GuitarAMI_module_XXX/ultTrig i` (0 or 1)
- `/GuitarAMI_module_XXX/count i`, `/tap i`, `/dtap i`, `/ttap i`
- `/GuitarAMI_module_XXX/jab fff`, `/shake fff`
- `/GuitarAMI_module_XXX/battery i` (percent)

Incoming: `/state/info` makes the module reply with `/GuitarAMI_module_XXX/info s i` (device name, firmware version).
