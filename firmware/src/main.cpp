//****************************************************************************//
// GuitarAMI Module                                                           //
// Input Devices and Music Interaction Laboratory (IDMIL), McGill University  //
// Edu Meneses (2026) - https://www.edumeneses.com                            //
//****************************************************************************//

/* 
 * Created using the Puara templates: https://github.com/Puara/puara-module-templates 
 * The template contains a fully commented version for the commonly used commands 
 */

#include "Arduino.h"
#include "esptouch.h"
#include "led.h"
#include <OSCBundle.h>
#include <OSCMessage.h>
#include "OSCTiming.h"
#include "puara.h"
#include "puara/gestures.h"
#include "puara/structs.h"
#include "ult.h"

#include <iostream>
#include <WiFiUdp.h>


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
  int ultDistance;
  int touch;
  int ultTrigger;
  int battery;
} sensors;

puara_gestures::Imu9Axis puaraIMU;
puara_gestures::Quaternion puaraQuat;
puara_gestures::Coord3D puaraYPR;
puara_gestures::Jab3D jab(&puaraIMU.accl);
puara_gestures::Shake3D shake(&puaraIMU.accl);
puara_gestures::Button button(&sensors.touch);

std::string oscIP{};
int oscPort{};

/////////////////////
// Pin definitions //
/////////////////////

struct Pin {
    int led;     // Built In LED pin
    int touch;   // Capacitive touch pin
    int battery; // To check battery level (voltage)
    int ultTrig; // connects to the trigger pin on the distance sensor
    int ultEcho; // connects to the echo pin on the distance sensor
};

struct led_variables {
    int ledValue = 0;
    uint8_t color = 0;
} led_var;

#ifdef ARDUINO_LOLIN_D32_PRO
    Pin pin{ 5, 15, 35, 32, 33 };
#elif defined(ARDUINO_TINYPICO)
    Pin pin{ 5, 4, 35, 32, 33 };
    // Disabling TinyPico helper as it needs update for version 3.x of the ESP32 Arduino Core
    //#include "TinyPICO.h"
    //TinyPICO tinypico = TinyPICO();
#endif

//////////////////////////////////
// Battery struct and functions //
//////////////////////////////////
 /* 
struct BatteryData {
    unsigned int percentage = 0;
    unsigned int lastPercentage = 0;
    float value;
    unsigned long timer = 0;
    int interval = 1000; // in ms (1/f)
    int queueAmount = 10; // # of values stored
    std::deque<int> filterArray; // store last values
} battery;

// // read battery level (based on https://www.youtube.com/watch?v=yZjpYmWVLh8&feature=youtu.be&t=88) 
void readBattery() {
  #ifdef ARDUINO_LOLIN_D32_PRO
    battery.value = analogRead(pin.battery) / 4096.0 * 7.445;
  #elif defined(ARDUINO_TINYPICO)
    battery.value = tinypico.GetBatteryVoltage();
  #endif
  battery.percentage = static_cast<int>((battery.value - 2.9) * 100 / (4.15 - 2.9));
  if (battery.percentage > 100)
    battery.percentage = 100;
  if (battery.percentage < 0)
    battery.percentage = 0;
}

void batteryFilter() {
  battery.filterArray.push_back(battery.percentage);
  if(battery.filterArray.size() > battery.queueAmount) {
    battery.filterArray.pop_front();
  }
  battery.percentage = 0;
  for (int i=0; i<battery.filterArray.size(); i++) {
    battery.percentage += battery.filterArray.at(i);
  }
  battery.percentage /= battery.filterArray.size();
}
*/

void onSettingsChanged() {
  Udp.begin(puara.getVarNumber("localPORT"));
  oscIP = puara.getVarText("oscIP");
  oscPort = puara.getVarNumber("oscPORT");
  touch.setSensitivity(std::round(puara.getVarNumber("touch_sensitivity")));
  std::cout << "touch_sensitivity updated to " << puara.getVarNumber("touch_sensitivity") << std::endl;
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
  puara.set_settings_changed_handler(onSettingsChanged);
  oscIP = puara.getVarText("oscIP");
  oscPort = puara.getVarNumber("oscPORT");

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

  // shorten button hold detection (original default was 5000ms)
  button.holdInterval = 1000;  // 1 second makes it easier to test
  // threshold stays 1 because sensors.touch is already binary

  std::cout << "    Initializing ultrasonic sensor... ";
  if (initUlt(pin.ultTrig, pin.ultEcho)) {
      std::cout << "done" << std::endl;
  } else {
    std::cout << "ultrasonic sensor initialization failed!" << std::endl;
  }

  // Initializing IMU
  std::cout << "    Initializing IMU... ";
  if (imu.initIMU()) {
      std::cout << "done" << std::endl;
  } else {
      std::cout << "IMU initialization failed!" << std::endl;
  }

  Serial.println(); 
  //Serial.println(puara.dmi_name().c_str());
  Serial.println("Edu Meneses\nSociété des Arts Technologiques (SAT)\nIDMIL - CIRMMT - McGill University");
  Serial.println(); 
}

void loop() {

  // Read Ultrasonic sensor distance
  readUlt();
  sensors.ultDistance = getUltDistance();

  // Read capacitive button
  touch.readTouch();
  sensors.touch = touch.getTouch() ? 1 : 0; // convert bool to int for OSC message
  button.update();
// Disabling TinyPico helper functions as it needs update for version 3.x of the ESP32 Arduino Core
//   // read battery
//   if (millis() - battery.interval > battery.timer) {
//     battery.timer = millis();
//     readBattery();
//     batteryFilter();
//   }

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

    OSCMessage &msgA = bundle.add(("/" + puara.dmi_name() + "/IMU").c_str());
    msgA.add(puaraIMU.accl.x)
        .add(puaraIMU.accl.y)
        .add(puaraIMU.accl.z);
    OSCMessage &msgB = bundle.add(("/" + puara.dmi_name() + "/gyro").c_str());
    msgB.add(puaraIMU.gyro.x)
        .add(puaraIMU.gyro.y)
        .add(puaraIMU.gyro.z);
    OSCMessage &msgC = bundle.add(("/" + puara.dmi_name() + "/magn").c_str());
    msgC.add(puaraIMU.magn.x)
        .add(puaraIMU.magn.y)
        .add(puaraIMU.magn.z);
    OSCMessage &msgD = bundle.add(("/" + puara.dmi_name() + "/quat").c_str());
    msgD.add(puaraQuat.w)
        .add(puaraQuat.x)
        .add(puaraQuat.y)
        .add(puaraQuat.z);
    OSCMessage &msgE = bundle.add(("/" + puara.dmi_name() + "/YPR").c_str());
    msgE.add(puaraYPR.x)
        .add(puaraYPR.y)
        .add(puaraYPR.z);
   
    OSCMessage &msgF = bundle.add( ("/" + puara.dmi_name() + "/ultrasonic").c_str());
    msgF.add(static_cast<int32_t>(sensors.ultDistance));

    OSCMessage &msgG = bundle.add(("/" + puara.dmi_name() + "/touch").c_str());
    msgG.add(touch.getValue());

    OSCMessage &msgH = bundle.add(("/" + puara.dmi_name() + "/button").c_str());
    msgH.add(button.press)
        .add(button.hold)
        .add(button.pressTime)
        .add(button.tap)
        .add(button.doubleTap)
        .add(button.tripleTap)
        .add(button.count);

    OSCMessage &msgI = bundle.add(("/" + puara.dmi_name() + "/jab").c_str());
    msgI.add(jab.x.current_value())
        .add(jab.y.current_value())
        .add(jab.z.current_value());

    OSCMessage &msgJ = bundle.add(("/" + puara.dmi_name() + "/shake").c_str());
    msgJ.add(shake.x.current_value())
        .add(shake.y.current_value())
        .add(shake.z.current_value());

    Udp.beginPacket(oscIP.c_str(), oscPort);
    bundle.setTimetag(oscTime());
    bundle.send(Udp);
    Udp.endPacket();
    bundle.empty();

  }

// Disabling TinyPico helper functions as it needs update for version 3.x of the ESP32 Arduino Core
//   // Set LED - connection status and battery level
//   #ifdef ARDUINO_LOLIN_D32_PRO
//     if (battery.percentage < 10) {        // low battery - flickering
//     led.setInterval(75);
//     led_var.ledValue = led.blink(255, 50);
//     ledcWrite(0, led_var.ledValue);
//     } else {
//         if (puara.get_StaIsConnected()) { // blinks when connected, cycle when disconnected
//             led.setInterval(1000);
//             led_var.ledValue = led.blink(255, 40);
//             ledcWrite(0, led_var.ledValue);
//         } else {
//             led.setInterval(4000);
//             led_var.ledValue = led.cycle(led_var.ledValue, 0, 255);
//             ledcWrite(0, led_var.ledValue);
//         }
//     }
//   #elif defined(ARDUINO_TINYPICO)
//     if (battery.percentage < 10) {                // low battery (red)
//         led.setInterval(20);
//         led_var.color = led.blink(255, 20);
//         tinypico.DotStar_SetPixelColor(led_var.color, 0, 0);
//     } else {
//         if (puara.get_StaIsConnected()) {         // blinks when connected, cycle when disconnected
//             led.setInterval(1000);                // RGB: 0, 128, 255 (Dodger Blue)
//             led_var.color = led.blink(255,20);
//             tinypico.DotStar_SetPixelColor(0, uint8_t(led_var.color/2), led_var.color);
//         } else {
//             led.setInterval(4000);
//             led_var.color = led.cycle(led_var.color, 0, 255);
//             tinypico.DotStar_SetPixelColor(0, uint8_t(led_var.color/2), led_var.color);
//         }
//     }
//   #endif    

  // run at 100 Hz
  vTaskDelay(10 / portTICK_PERIOD_MS);
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
