#pragma once

// Module #003 carries a BNO080 instead of the LSM9DS1. Uncomment to build for it.
// #define GUITARAMI_IMU_BNO080

constexpr unsigned int FIRMWARE_VERSION = 260917;

namespace pins {
constexpr int touch = 15;
constexpr int ultrasonicTrigger = 32;
constexpr int ultrasonicEcho = 33;
}

namespace rates {
constexpr unsigned long oscPeriodMs = 10;
constexpr unsigned long linkCheckPeriodMs = 500;
}

namespace defaults {
constexpr unsigned int touchThreshold = 950;
constexpr int jabThreshold = 10;
constexpr unsigned int lowBatteryPercent = 10;
}
