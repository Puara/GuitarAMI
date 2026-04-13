//****************************************************************************//
// GuitarAMI Module                                                           //
// Input Devices and Music Interaction Laboratory (IDMIL), McGill University  //
// Edu Meneses (2026) - https://www.edumeneses.com                            //
// Created using the Puara templates:                                         //
//                https://github.com/Puara/puara-module-templates             //
//****************************************************************************//

#include "Arduino.h"
#include "OSCBundle.h"
#include "OSCMessage.h"
#include "OSCTiming.h"
#include "WiFiUdp.h"
#include "esptouch.h"
#include "led.h"
#include "puara.h"
#include "puara/gestures.h"
#include "puara/structs.h"
#include "puara/utils/magnetometerCalibration.h"
#include "ult.h"

#include <iostream>
#include <array>
#include <vector>

#define ASSUMED_EMPTY_BATTERY_VOLTAGE 2.9
#define ASSUMED_FULL_BATTERY_VOLTAGE 4.15

// (Un)comment the proper module for your system (LSM9DS1 or BNO080).
#define imu_LSM9DS1
// #define imu_BNO080

#ifdef imu_BNO080
#include "bno080.h"
Imu_BNO080 imu;
#endif
#ifdef imu_LSM9DS1
#include "lsm9ds1.h"
Imu_LSM9DS1 imu;
#endif

Led led;
Puara puara;
Touch touch;
WiFiUDP Udp;

struct Sensors {
  int battery;
  int touch;
  int ultDistance;
  int ultTrigger;
};

struct led_variables {
  int ledValue = 0;
  uint8_t color = 0;
};

Sensors sensors;
led_variables led_var;

puara_gestures::Button button(&sensors.touch);
puara_gestures::Coord3D puaraYPR;
puara_gestures::Imu9Axis puaraIMU;
puara_gestures::Jab3D jab(&puaraIMU.accl);
puara_gestures::Quaternion puaraQuat;
puara_gestures::Shake3D shake(&puaraIMU.accl);

std::string oscIP{};
int oscPort{};
std::string osc_prefix{};

puara_gestures::utils::Calibration magCalibration;
static bool calibrationMode = false;
static bool magnetometerCalibrated = false;
static const size_t maxCalibrationSamples = 512;
static std::array<puara_gestures::Coord3D, maxCalibrationSamples> calibrationRawMagData;
static size_t calibrationRawMagCount = 0;

// Pin definitions
struct Pin {
  int led;     // Built In LED pin
  int touch;   // Capacitive touch pin
  int battery; // To check battery level (voltage)
  int ultTrig; // connects to the trigger pin on the distance sensor
  int ultEcho; // connects to the echo pin on the distance sensor
};

#ifdef ARDUINO_LOLIN_D32_PRO
#define ADC12BIT_MAX 4095.0
#define VOLTAGE_DIVIDER_RATIO 7.445
Pin pin{5, 15, 35, 32, 33};
#elif defined(ARDUINO_TINYPICO)
Pin pin{5, 4, 35, 32, 33};
#include "TinyPICO.h"
TinyPICO tinypico = TinyPICO();
#endif

//////////////////////////////////
// Battery struct and functions //
//////////////////////////////////

struct BatteryData {
  unsigned long timer = 0;
  float value = 0.0f;
  unsigned int percentage = 0;
  unsigned int lastPercentage = 0;
  int interval = 1000;         // ms between readings
  int queueAmount = 10;        // # of samples to average
  std::deque<int> filterArray; // history buffer
};

static BatteryData battery;
void readBattery();
void batteryFilter();
void startMagnetometerCalibration();
void processMagnetometerCalibration();

//////////////////////////////////////////////
// Updates when settings saved in webserver //
//////////////////////////////////////////////
void onSettingsChanged() {
  Udp.begin(puara.getVarNumber("localPORT"));
  oscIP = puara.getVarText("oscIP");
  oscPort = puara.getVarNumber("oscPORT");
  touch.setSensitivity(std::round(puara.getVarNumber("touch_sensitivity")));
}

///////////
// setup //
///////////

void setup() {
#ifdef Arduino_h
  Serial.begin(115200);
#endif

  puara.start();
  Udp.begin(puara.getVarNumber("localPORT"));
  oscIP = puara.getVarText("oscIP");
  oscPort = puara.getVarNumber("oscPORT");

  puara.set_settings_changed_handler(onSettingsChanged);

  osc_prefix = "/" + puara.dmi_name();

#ifdef ARDUINO_LOLIN_D32_PRO // LED init for WEMOS boards
  ledcSetup(0, 5000, 8);
  ledcAttachPin(pin.led, 0);
#endif

  std::cout << "    Initializing capacitive touch sensor... ";
  touch.setSensitivity(std::round(puara.getVarNumber("touch_sensitivity")));
  if (touch.initTouch()) {
    touch.setHoldInterval(10000);
    std::cout << "done" << std::endl;
  } else {
    std::cout << "capacitive touch sensor initialization failed!" << std::endl;
  }

  std::cout << "    Initializing ultrasonic sensor... ";
  if (initUlt(pin.ultTrig, pin.ultEcho)) {
    std::cout << "done" << std::endl;
  } else {
    std::cout << "ultrasonic sensor initialization failed!" << std::endl;
  }

  std::cout << "    Initializing IMU... ";
  if (imu.initIMU()) {
    std::cout << "done" << std::endl;
  } else {
    std::cout << "IMU initialization failed!" << std::endl;
  }

  Serial.println();
  Serial.println("Edu Meneses\nSociété des Arts Technologiques (SAT)\nIDMIL - "
                 "CIRMMT - McGill University");
  Serial.println();
}

void loop() {

  // Read Ultrasonic sensor distance
  readUlt();
  sensors.ultDistance = getUltDistance();

  // Read capacitive button
  touch.readTouch();
  sensors.touch =
      touch.getTouch() ? 1 : 0; // convert bool to int for OSC message
  button.update();

  if (touch.getHold() && !calibrationMode) {
    startMagnetometerCalibration();
  }

  // Read battery
  if (millis() - battery.interval > battery.timer) {
    battery.timer = millis();
    readBattery();
    batteryFilter();
  }

  // read IMU and update puara-gestures
  if (imu.dataAvailable()) {
    puaraIMU.accl.x = imu.getAccelX();
    puaraIMU.accl.y = imu.getAccelY();
    puaraIMU.accl.z = imu.getAccelZ();
    puaraIMU.gyro.x = imu.getGyroX();
    puaraIMU.gyro.y = imu.getGyroY();
    puaraIMU.gyro.z = imu.getGyroZ();
    puaraIMU.magn.x = imu.getMagX();
    puaraIMU.magn.y = imu.getMagY();
    puaraIMU.magn.z = imu.getMagZ();
    puaraQuat.w = imu.getQuatI();
    puaraQuat.x = imu.getQuatJ();
    puaraQuat.y = imu.getQuatK();
    puaraQuat.z = imu.getQuatReal();
    puaraYPR.x = imu.getYaw();
    puaraYPR.y = imu.getPitch();
    puaraYPR.z = imu.getRoll();
    jab.update();
    shake.update();

    if (calibrationMode) {
      processMagnetometerCalibration();
    }

    if (magnetometerCalibrated) {
      magCalibration.applyMagnetometerCalibration(puaraIMU);
      puaraIMU.magn = magCalibration.myCalIMU.magn;
    }
  }

  /*
   * Sending OSC messages.
   * This sends the sensor value to the defined OSC IP : port.
   */
  if (!oscIP.empty() && oscIP != "0.0.0.0") {

    OSCBundle bundle;
    osctime_t timetag;

    bundle.add((osc_prefix + "/IMU/accl/x").c_str()).add(puaraIMU.accl.x);
    bundle.add((osc_prefix + "/IMU/accl/y").c_str()).add(puaraIMU.accl.y);
    bundle.add((osc_prefix + "/IMU/accl/z").c_str()).add(puaraIMU.accl.z);

    bundle.add((osc_prefix + "/IMU/gyro/x").c_str()).add(puaraIMU.gyro.x);
    bundle.add((osc_prefix + "/IMU/gyro/y").c_str()).add(puaraIMU.gyro.y);
    bundle.add((osc_prefix + "/IMU/gyro/z").c_str()).add(puaraIMU.gyro.z);

    bundle.add((osc_prefix + "/IMU/magn/x").c_str()).add(puaraIMU.magn.x);
    bundle.add((osc_prefix + "/IMU/magn/y").c_str()).add(puaraIMU.magn.y);
    bundle.add((osc_prefix + "/IMU/magn/z").c_str()).add(puaraIMU.magn.z);

    bundle.add((osc_prefix + "/IMU/quat/w").c_str()).add(puaraQuat.w);
    bundle.add((osc_prefix + "/IMU/quat/x").c_str()).add(puaraQuat.x);
    bundle.add((osc_prefix + "/IMU/quat/y").c_str()).add(puaraQuat.y);
    bundle.add((osc_prefix + "/IMU/quat/z").c_str()).add(puaraQuat.z);

    bundle.add((osc_prefix + "/IMU/YPR/Roll").c_str()).add(puaraYPR.x);
    bundle.add((osc_prefix + "/IMU/YPR/Pitch").c_str()).add(puaraYPR.y);
    bundle.add((osc_prefix + "/IMU/YPR/Yaw").c_str()).add(puaraYPR.z);

    bundle.add((osc_prefix + "/ultrasonic/distance").c_str())
        .add(static_cast<int32_t>(sensors.ultDistance));

    bundle.add((osc_prefix + "/touch").c_str()).add(touch.getValue());

    bundle.add((osc_prefix + "/button/press").c_str()).add(button.press);
    bundle.add((osc_prefix + "/button/hold").c_str()).add(button.hold);
    bundle.add((osc_prefix + "/button/pressTime").c_str())
        .add(button.pressTime);
    bundle.add((osc_prefix + "/button/tap").c_str()).add(button.tap);
    bundle.add((osc_prefix + "/button/doubleTap").c_str())
        .add(button.doubleTap);
    bundle.add((osc_prefix + "/button/tripleTap").c_str())
        .add(button.tripleTap);
    bundle.add((osc_prefix + "/button/count").c_str()).add(button.count);

    bundle.add(("/" + puara.dmi_name() + "/jab/x").c_str()).add(jab.x.current_value());
    bundle.add(("/" + puara.dmi_name() + "/jab/y").c_str()).add(jab.y.current_value());
  bundle.add(("/" + puara.dmi_name() + "/jab/z").c_str()).add(jab.z.current_value());

    bundle.add(("/" + puara.dmi_name() + "/shake/x").c_str()).add(shake.x.current_value());
    bundle.add(("/" + puara.dmi_name() + "/shake/y").c_str()).add(shake.y.current_value());
    bundle.add(("/" + puara.dmi_name() + "/shake/z").c_str()).add(shake.z.current_value());

    bundle.add(("/" + puara.dmi_name() + "/battery/percentage").c_str())
        .add(battery.percentage);

    Udp.beginPacket(oscIP.c_str(), oscPort);
    bundle.setTimetag(oscTime());
    bundle.send(Udp);
    Udp.endPacket();
    bundle.empty();
  }

// Set LED - connection status and battery level
#ifdef ARDUINO_LOLIN_D32_PRO
  if (battery.percentage < 10) { // low battery - flickering
    led.setInterval(75);
    led_var.ledValue = led.blink(255, 50);
    ledcWrite(
        0,
        led_var.ledValue); // why is this at 0 but led pin is defined above...
  } else {
    // blinks when connected, cycle when disconnected
    if (puara.get_StaIsConnected()) {
      led.setInterval(1000);
      led_var.ledValue = led.blink(255, 40);
      ledcWrite(0, led_var.ledValue);
    } else {
      led.setInterval(4000);
      led_var.ledValue = led.cycle(led_var.ledValue, 0, 255);
      ledcWrite(0, led_var.ledValue);
    }
  }
#elif defined(ARDUINO_TINYPICO)
  if (battery.percentage < 10) { // low battery (red)
    led.setInterval(20);
    led_var.color = led.blink(255, 20);
    tinypico.DotStar_SetPixelColor(led_var.color, 0, 0);
  } else {
    if (puara.get_StaIsConnected()) {
      led.setInterval(1000); // RGB: 0, 128, 255
                             // (Dodger Blue)
      led_var.color = led.blink(255, 20);
      tinypico.DotStar_SetPixelColor(0, uint8_t(led_var.color / 2),
                                     led_var.color);
    } else {
      led.setInterval(4000);
      led_var.color = led.cycle(led_var.color, 0, 255);
      tinypico.DotStar_SetPixelColor(0, uint8_t(led_var.color / 2),
                                     led_var.color);
    }
  }
#endif

  // run at 100 Hz
  vTaskDelay(10 / portTICK_PERIOD_MS);
}

void readBattery() {
#ifdef ARDUINO_LOLIN_D32_PRO
  battery.value =
      analogRead(pin.battery) / ADC12BIT_MAX * VOLTAGE_DIVIDER_RATIO;
#elif defined(ARDUINO_TINYPICO)
  battery.value = tinypico.GetBatteryVoltage();
#endif
  /*
   * Calculate battery percentage based on voltage, assuming linear discharge
   * between empty and full voltage levels. This simplification may not reflect
   * actual battery discharge curves but provides useful approximation for
   * monitoring battery status.
   *The percentage is clamped between 0% and 100% to avoid unrealistic values
   * due to measurement noise or voltage fluctuations.
   */
  battery.percentage = static_cast<int>(
      (battery.value - ASSUMED_EMPTY_BATTERY_VOLTAGE) * 100 /
      (ASSUMED_FULL_BATTERY_VOLTAGE - ASSUMED_EMPTY_BATTERY_VOLTAGE));

  if (battery.percentage > 100) {
    battery.percentage = 100;
  } else if (battery.percentage < 0) {
    battery.percentage = 0;
  }
}

void batteryFilter() {
  battery.filterArray.push_back(battery.percentage);
  if (battery.filterArray.size() > battery.queueAmount) {
    battery.filterArray.pop_front();
  }
  battery.percentage = 0;
  for (int i = 0; i < battery.filterArray.size(); i++) {
    battery.percentage += battery.filterArray.at(i);
  }
  battery.percentage /= battery.filterArray.size();
}

void startMagnetometerCalibration() {
  calibrationMode = true;
  calibrationRawMagCount = 0;

  Serial.println();
  Serial.println("=== MAGNETOMETER CALIBRATION START ===");
  Serial.println("Keep the module still for a few seconds, then rotate slowly through all axes.");
  Serial.print("Collected 512 samples.");
}

void processMagnetometerCalibration() {
  if (!calibrationMode) {
    return;
  }

  if (calibrationRawMagCount < maxCalibrationSamples) {
    calibrationRawMagData[calibrationRawMagCount++] = {puaraIMU.magn.x, puaraIMU.magn.y, puaraIMU.magn.z};
    return;
  }
  Serial.println("Reached max calibration samples, processing data...");
  Serial.println("Transforming array into vector");

  std::vector<puara_gestures::Coord3D> sampleVec(calibrationRawMagData.begin(), calibrationRawMagData.begin() + calibrationRawMagCount);
  Serial.println("generatingMagnetometerMatrices() ...");

  int result = magCalibration.generateMagnetometerMatrices(sampleVec);

  if (result == 1) {
    magnetometerCalibrated = true;
    Serial.println("Calibration completed successfully.");
    Serial.println("Calibrated magnetometer data will now be applied to live readings.");
    Serial.print("Hard iron bias: ");
    Serial.print(magCalibration.hardIronBias(0));
    Serial.print(", ");
    Serial.print(magCalibration.hardIronBias(1));
    Serial.print(", ");
    Serial.println(magCalibration.hardIronBias(2));
    Serial.println("Soft iron matrix:");
    for (int i = 0; i < 3; ++i) {
      Serial.print("  ");
      Serial.print(magCalibration.softIronMatrix(i, 0));
      Serial.print(", ");
      Serial.print(magCalibration.softIronMatrix(i, 1));
      Serial.print(", ");
      Serial.println(magCalibration.softIronMatrix(i, 2));
    }
    Serial.println("Calibration data ready.");
  } else {
    Serial.println("Calibration failed. Retry by holding the touch for 10 seconds and rotating the sensor more evenly.");
  }

  calibrationMode = false;
}

#ifndef Arduino_h
extern "C" {
void app_main(void);
}

void app_main() {
  setup();
  while (1) {
    loop();
  }
}
#endif
