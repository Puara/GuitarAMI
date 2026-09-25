#pragma once

#include <TinyPICO.h>

// Drives the TinyPICO DotStar over hardware SPI; the helper library's bit-bang write takes 9 ms.
class StatusLed {
public:
  enum class Link { Station, AccessPoint };

  explicit StatusLed(TinyPICO& board);
  void begin();
  void update(Link link, bool lowBattery);

private:
  static constexpr unsigned long refreshPeriodMs = 20;
  static constexpr uint8_t brightness = 128;

  static uint8_t blink(unsigned long periodMs, unsigned int dutyPercent);
  static uint8_t breathe(unsigned long periodMs);
  void show(uint8_t red, uint8_t green, uint8_t blue);

  TinyPICO& board;
  unsigned long refreshTimer = 0;
  uint32_t lastColor = 0;
};
