#include "status_led.h"

#include <SPI.h>

StatusLed::StatusLed(TinyPICO& board) : board(board) {}

void StatusLed::begin() {
  board.DotStar_SetPower(true);
  SPI.begin(DOTSTAR_CLK, -1, DOTSTAR_DATA, -1);
  show(0, 0, 0);
}

void StatusLed::update(Link link, bool lowBattery) {
  if (millis() - refreshTimer < refreshPeriodMs) return;
  refreshTimer = millis();

  if (lowBattery) {
    show(blink(100, 20), 0, 0);
    return;
  }

  switch (link) {
    case Link::Station: {
      uint8_t level = blink(1000, 20);
      show(0, level / 2, level);
      break;
    }
    case Link::AccessPoint: {
      show(0, breathe(4000), 0);
      break;
    }
  }
}

uint8_t StatusLed::blink(unsigned long periodMs, unsigned int dutyPercent) {
  return millis() % periodMs < periodMs * dutyPercent / 100 ? 255 : 0;
}

uint8_t StatusLed::breathe(unsigned long periodMs) {
  unsigned long phase = millis() % periodMs;
  unsigned long half = periodMs / 2;
  unsigned long rising = phase < half ? phase : periodMs - phase;
  return rising * 255 / half;
}

void StatusLed::show(uint8_t red, uint8_t green, uint8_t blue) {
  uint32_t color = board.Color(red, green, blue);
  if (color == lastColor) return;
  lastColor = color;

  uint8_t frame[] = {0x00, 0x00, 0x00, 0x00, 0xFF, uint8_t(blue * brightness >> 8), uint8_t(green * brightness >> 8),
                     uint8_t(red * brightness >> 8), 0xFF};
  SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
  SPI.writeBytes(frame, sizeof(frame));
  SPI.endTransaction();
}
