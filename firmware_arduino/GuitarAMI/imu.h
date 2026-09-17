#pragma once

#include <puara-gestures.h>

// accl in m/s^2, gyro in rad/s, magn in uT, euler in degrees
class Imu {
public:
  bool begin();
  bool update();

  puara_gestures::Imu9Axis reading;
  puara_gestures::Quaternion quaternion;
  struct Euler {
    double yaw = 0;
    double pitch = 0;
    double roll = 0;
  } euler;
};
