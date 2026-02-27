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
#include "ult.h"

#include <iostream>

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
  }

  /*
   * Sending OSC messages.
   * This sends the sensor value to the defined OSC IP : port.
   */
  if (!oscIP.empty() && oscIP != "0.0.0.0") {

    OSCBundle bundle;
    osctime_t timetag;

    bundle.add((osc_prefix + "/IMU").c_str())
      .add(puaraIMU.accl.x).add(puaraIMU.accl.y).add(puaraIMU.accl.z);

    bundle.add((osc_prefix + "/gyro").c_str())
      .add(puaraIMU.gyro.x).add(puaraIMU.gyro.y).add(puaraIMU.gyro.z);

    bundle.add((osc_prefix + "/magn").c_str())
      .add(puaraIMU.magn.x).add(puaraIMU.magn.y).add(puaraIMU.magn.z);
    
    bundle.add((osc_prefix + "/quat").c_str())
      .add(puaraQuat.w).add(puaraQuat.x).add(puaraQuat.y).add(puaraQuat.z);

    bundle.add((osc_prefix + "/YPR").c_str())
      .add(puaraYPR.x).add(puaraYPR.y).add(puaraYPR.z);

    bundle.add((osc_prefix + "/ultrasonic").c_str())
      .add(static_cast<int32_t>(sensors.ultDistance));

    bundle.add((osc_prefix + "/touch").c_str())
      .add(touch.getValue());
    
    bundle.add((osc_prefix+"/button/press").c_str()).add(button.press);
    bundle.add((osc_prefix+"/button/hold").c_str()).add(button.hold);
    bundle.add((osc_prefix+"/button/pressTime").c_str()).add(button.pressTime);
    bundle.add((osc_prefix+"/button/tap").c_str()).add(button.tap);
    bundle.add((osc_prefix+"/button/doubleTap").c_str()).add(button.doubleTap);
    bundle.add((osc_prefix+"/button/tripleTap").c_str()).add(button.tripleTap);
    bundle.add((osc_prefix+"/button/count").c_str()).add(button.count);
    
    bundle.add(("/" + puara.dmi_name() + "/jab").c_str())
      .add(jab.x.current_value())
      .add(jab.y.current_value())
      .add(jab.z.current_value());
    
    bundle.add(("/" + puara.dmi_name() + "/shake").c_str())
      .add(shake.x.current_value())
      .add(shake.y.current_value())
      .add(shake.z.current_value());
    
    bundle.add(("/" + puara.dmi_name() + "/battery").c_str())
      .add(battery.percentage);

    Udp.beginPacket(oscIP.c_str(), oscPort);
    bundle.setTimetag(oscTime());
    bundle.send(Udp);
    Udp.endPacket();
    bundle.empty();
  }

  // Set LED - connection status and battery level
  #ifdef ARDUINO_LOLIN_D32_PRO
    if (battery.percentage < 10) {        // low battery - flickering
      led.setInterval(75);
      led_var.ledValue = led.blink(255, 50);
      ledcWrite(0, led_var.ledValue); // why is this at 0 but led pin is defined above...
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
    if (battery.percentage < 10) {                // low battery (red)
      led.setInterval(20);
      led_var.color = led.blink(255, 20);
      tinypico.DotStar_SetPixelColor(led_var.color, 0, 0);
    } else {
      if (puara.get_StaIsConnected()) {         
        led.setInterval(1000);                // RGB: 0, 128, 255
       // (Dodger Blue) 
        led_var.color = led.blink(255,20);
        tinypico.DotStar_SetPixelColor(0, uint8_t(led_var.color/2),
               led_var.color);
    } else {
        led.setInterval(4000);
        led_var.color = led.cycle(led_var.color, 0, 255);
        tinypico.DotStar_SetPixelColor(0, uint8_t(led_var.color/2),
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
