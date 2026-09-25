#include "imu.h"

#include <Wire.h>

#include "config.h"

#ifdef GUITARAMI_IMU_BNO080

#include <SparkFun_BNO080_Arduino_Library.h>

namespace {
BNO080 sensor;
constexpr uint16_t reportPeriodMs = 10;
}

bool Imu::begin() {
  Wire.begin();
  if (!sensor.begin()) return false;
  Wire.setClock(400000);
  sensor.enableAccelerometer(reportPeriodMs);
  sensor.enableGyro(reportPeriodMs);
  sensor.enableMagnetometer(reportPeriodMs);
  sensor.enableGameRotationVector(reportPeriodMs);
  sensor.calibrateAll();
  return true;
}

bool Imu::update() {
  if (!sensor.dataAvailable()) return false;
  reading.accl = {sensor.getAccelX(), sensor.getAccelY(), sensor.getAccelZ()};
  reading.gyro = {sensor.getGyroX(), sensor.getGyroY(), sensor.getGyroZ()};
  reading.magn = {sensor.getMagX(), sensor.getMagY(), sensor.getMagZ()};
  quaternion = {sensor.getQuatReal(), sensor.getQuatI(), sensor.getQuatJ(), sensor.getQuatK()};
  euler.roll = degrees(sensor.getRoll());
  euler.pitch = degrees(sensor.getPitch());
  euler.yaw = degrees(sensor.getYaw());
  return true;
}

#else

#include <SparkFunLSM9DS1.h>

namespace {
LSM9DS1 sensor;
puara_gestures::MadgwickQuaternionFilter fusion(0.6);
constexpr uint16_t gyroScaleDps = 2000;
constexpr uint8_t accelScaleG = 16;
constexpr uint8_t accelGyroOdr238Hz = 4;
constexpr uint8_t magOdr80Hz = 7;
constexpr unsigned int warmupSamples = 20;  // power-up readings are garbage
unsigned int samples = 0;
}

bool Imu::begin() {
  Wire.begin();
  if (sensor.begin() == 0) return false;
  Wire.setClock(400000);
  sensor.setGyroScale(gyroScaleDps);
  sensor.setAccelScale(accelScaleG);
  sensor.setGyroODR(accelGyroOdr238Hz);
  sensor.setMagODR(magOdr80Hz);
  return true;
}

bool Imu::update() {
  sensor.readGyro();
  sensor.readAccel();
  sensor.readMag();
  if (samples < warmupSamples) {
    samples++;
    return false;
  }

  using namespace puara_gestures::utils::convert;
  reading.accl = {g_to_ms2(sensor.calcAccel(sensor.ax)), g_to_ms2(sensor.calcAccel(sensor.ay)),
                  g_to_ms2(sensor.calcAccel(sensor.az))};
  reading.gyro = {dps_to_rads(sensor.calcGyro(sensor.gx)), dps_to_rads(sensor.calcGyro(sensor.gy)),
                  dps_to_rads(sensor.calcGyro(sensor.gz))};
  reading.magn = {gauss_to_utesla(sensor.calcMag(sensor.mx)), gauss_to_utesla(sensor.calcMag(sensor.my)),
                  gauss_to_utesla(sensor.calcMag(sensor.mz))};

  fusion.update(reading);
  quaternion = fusion.getQuaternion();
  fusion.getEulerDegrees(euler.roll, euler.pitch, euler.yaw);
  return true;
}

#endif
