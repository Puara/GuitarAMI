# GuitarAMI Firmware

ESP32 firmware for the GuitarAMI module, built with [PlatformIO](https://platformio.org/) (Arduino framework). It reads onboard sensors, computes gestures using the [Puara gestures](https://github.com/Puara/puara-gestures) library, and streams everything as OSC bundles over WiFi UDP at approximately 100 Hz.

## Supported Boards

| Board | LED feedback | Battery reading |
|-------|-------------|-----------------|
| **TinyPICO** | DotStar RGB LED | `TinyPICO::GetBatteryVoltage()` |
| **LOLIN D32 PRO** | PWM on GPIO 5 | ADC with voltage divider |

## Sensors

- **IMU** — either LSM9DS1 or BNO080 (selected at compile time via `#define imu_LSM9DS1` / `#define imu_BNO080` in `main.cpp`). Provides accelerometer, gyroscope, magnetometer, quaternion, and yaw/pitch/roll data.
- **Capacitive touch** — single-channel touch sensor for detecting physical interaction.
- **Ultrasonic distance** — trigger/echo-based distance measurement (e.g., RCWL-1601 or HC-SR04).
- **Battery** — voltage is read periodically (every 1 s), converted to a percentage (assuming 2.9 V empty / 4.15 V full), and smoothed with a 10-sample moving-average filter.

## Gesture Processing

The firmware uses [puara-gestures](https://github.com/Puara/puara-gestures) to derive high-level descriptors from the raw sensor streams:

| Gesture | Source | Description |
|---------|--------|-------------|
| **Jab** (x, y, z) | Accelerometer | Detects sharp directional impulses |
| **Shake** (x, y, z) | Accelerometer | Detects sustained shaking motion |
| **Button** | Touch sensor | Derives press, hold, pressTime, tap, doubleTap, tripleTap, and count |

## OSC Namespace

All messages are prefixed with `/<device_name>` (configured through the Puara web interface). The following OSC addresses are sent in each bundle:

| Address | Type | Content |
|---------|------|---------|
| `/<name>/IMU` | `fff` | Accelerometer x, y, z |
| `/<name>/gyro` | `fff` | Gyroscope x, y, z |
| `/<name>/magn` | `fff` | Magnetometer x, y, z |
| `/<name>/quat` | `ffff` | Quaternion w, x, y, z |
| `/<name>/YPR` | `fff` | Yaw, pitch, roll |
| `/<name>/ultrasonic` | `i` | Distance (integer) |
| `/<name>/touch` | `f` | Touch sensor value |
| `/<name>/button/press` | `i` | Button press state |
| `/<name>/button/hold` | `i` | Button hold state |
| `/<name>/button/pressTime` | `i` | Press duration |
| `/<name>/button/tap` | `i` | Single tap |
| `/<name>/button/doubleTap` | `i` | Double tap |
| `/<name>/button/tripleTap` | `i` | Triple tap |
| `/<name>/button/count` | `i` | Tap count |
| `/<name>/jab` | `fff` | Jab x, y, z |
| `/<name>/shake` | `fff` | Shake x, y, z |
| `/<name>/battery` | `i` | Battery percentage |

## LED Behaviour

| Condition | LOLIN D32 PRO | TinyPICO |
|-----------|--------------|----------|
| **Low battery** (< 10 %) | Fast flicker (75 ms) | Fast red blink |
| **WiFi connected** | Slow blink (1 s) | Slow blue blink |
| **WiFi disconnected** | Slow breathing cycle (4 s) | Slow blue breathing cycle |

## Configuration

The module exposes a web interface (provided by the [Puara module](https://github.com/Puara/puara-module) library) for configuring:

- WiFi credentials
- OSC target IP and port
- Local listening port
- Touch sensor sensitivity

Settings take effect immediately when saved.

## Building

1. Install [PlatformIO](https://platformio.org/).
2. Open the `firmware/` directory as a PlatformIO project.
3. Select the desired IMU in `src/main.cpp` by uncommenting the appropriate `#define` (`imu_LSM9DS1` or `imu_BNO080`).
4. Build and upload:
   ```bash
   pio run -e tinypico -t upload
   ```
5. Upload the filesystem (web interface files):
   ```bash
   pio run -e tinypico -t uploadfs
   ```

## Dependencies

Managed automatically by PlatformIO (see `platformio.ini`):

- [puara-module](https://github.com/Puara/puara-module) v1.0.1
- [puara-gestures](https://github.com/Puara/puara-gestures)
- [CNMAT OSC](https://github.com/cnmat/OSC) v3.5.8
- [SparkFun BNO080](https://github.com/sparkfun/SparkFun_BNO080_Arduino_Library) v1.1.8
- [SparkFun LSM9DS1](https://github.com/sparkfun/SparkFun_LSM9DS1_Arduino_Library) v2.0.0
- [TinyPICO Helper](https://github.com/UnexpectedMaker/tinypico-helper) v1.5

## References

- [CNMAT OSC library on GitHub](https://github.com/cnmat/OSC)
- [Puara framework](https://github.com/Puara)
- [PlatformIO documentation](https://docs.platformio.org/) 


