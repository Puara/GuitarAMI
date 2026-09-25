#pragma once

#include <Arduino.h>

// HC-SR04 style sensor timed from the echo pin interrupt so the loop never waits for the echo.
class Ultrasonic {
public:
  Ultrasonic(uint8_t triggerPin, uint8_t echoPin);
  void begin();
  void update();

  unsigned int distance = 0;  // mm, 0 when nothing is in range
  bool trigger = false;       // object entered and left within the trigger window

private:
  static constexpr unsigned int maxRangeMm = 200;
  static constexpr unsigned int minRangeMm = 20;
  static constexpr unsigned long readPeriodMs = 20;
  static constexpr unsigned long triggerWindowMs = 200;
  static constexpr float microsPerMm = 5.83f;

  static void onEcho(void* self);
  unsigned int readMillimeters();
  void ping();

  uint8_t triggerPin;
  uint8_t echoPin;
  volatile unsigned long echoStart = 0;
  volatile unsigned long echoMicros = 0;
  volatile bool echoDone = false;
  unsigned int raw = 0;
  unsigned long readTimer = 0;
  unsigned long presenceTimer = 0;
};
