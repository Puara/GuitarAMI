#include "battery.h"

#include <algorithm>
#include <cmath>

Battery::Battery(TinyPICO& board) : board(board) {}

void Battery::update() {
  if (millis() - readTimer < readPeriodMs) return;
  readTimer = millis();

  float level = (board.GetBatteryVoltage() - emptyVolts) / (fullVolts - emptyVolts) * 100.0f;
  percentage = std::lround(std::clamp(average.smooth(level), 0.0, 100.0));
}
