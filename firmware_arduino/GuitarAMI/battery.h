#pragma once

#include <TinyPICO.h>
#include <puara-gestures.h>

class Battery {
public:
  explicit Battery(TinyPICO& board);
  void update();

  unsigned int percentage = 100;

private:
  static constexpr float emptyVolts = 2.9f;
  static constexpr float fullVolts = 4.15f;
  static constexpr unsigned long readPeriodMs = 1000;
  static constexpr std::size_t averageWindow = 10;

  TinyPICO& board;
  puara_gestures::utils::Smooth average{averageWindow};
  unsigned long readTimer = 0;
};
