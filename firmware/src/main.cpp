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
#include "puara.h"
#include "puara/gestures.h"
#include "puara/structs.h"
#include <OSCBundle.h>
#include <OSCMessage.h>
#include <WiFiUdp.h>

#include <iostream>

/* (Un)comment the following lines as some GuitarAMi modules
 * (e.g., GuitarAMI module #003) use the BNO080 IMU
 */
#define imu_LSM9DS1
// #define imu_BNO080

Puara puara;

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

IMU_Orientation orientation;

struct Event {
    bool shake = false;
    bool jab = false;
    bool count = false;
    bool tap = false;
    bool dtap = false;
    bool ttap = false;
    bool ultTrigger = false;
    bool battery;
} event;

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
  
struct BatteryData {
    unsigned int percentage = 0;
    unsigned int lastPercentage = 0;
    float value;
    unsigned long timer = 0;
    int interval = 1000; // in ms (1/f)
    int queueAmount = 10; // # of values stored
    std::deque<int> filterArray; // store last values
} battery;

// Disabling TinyPico helper functions as it needs update for version 3.x of the ESP32 Arduino Core
// // read battery level (based on https://www.youtube.com/watch?v=yZjpYmWVLh8&feature=youtu.be&t=88) 
// void readBattery() {
//     #ifdef ARDUINO_LOLIN_D32_PRO
//         battery.value = analogRead(pin.battery) / 4096.0 * 7.445;
//     #elif defined(ARDUINO_TINYPICO)
//         battery.value = tinypico.GetBatteryVoltage();
//     #endif
//     battery.percentage = static_cast<int>((battery.value - 2.9) * 100 / (4.15 - 2.9));
//     if (battery.percentage > 100)
//         battery.percentage = 100;
//     if (battery.percentage < 0)
//         battery.percentage = 0;
// }

// void batteryFilter() {
//     battery.filterArray.push_back(battery.percentage);
//     if(battery.filterArray.size() > battery.queueAmount) {
//         battery.filterArray.pop_front();
//     }
//     battery.percentage = 0;
//     for (int i=0; i<battery.filterArray.size(); i++) {
//         battery.percentage += battery.filterArray.at(i);
//     }
//     battery.percentage /= battery.filterArray.size();
// }

///////////////
// OSC / UDP //
///////////////

WiFiUDP Udp;
std::string oscIP{};
int oscPort{};

//////////////////////////////////
// Include Touch function files //
//////////////////////////////////

#include "esptouch.h"

Touch touch;

//////////////////////////////////////////////
// Include ultrasonic sensor function files //
//////////////////////////////////////////////

#include "ult.h"

////////////////////////////////
// Include IMU function files //
////////////////////////////////
  
#ifdef imu_BNO080
    #include "bno080.h"
    Imu_BNO080 imu;
#endif
#ifdef imu_LSM9DS1
    #include "lsm9ds1.h"
    Imu_LSM9DS1 imu;
#endif

////////////////////////////////
// Include LED function files //
////////////////////////////////

#include "led.h"

Led led;

struct Led_variables {
    int ledValue = 0;
    uint8_t color = 0;
} led_var;

void onSettingsChanged() {
  Udp.begin(puara.getVarNumber("localPORT"));
  oscIP = puara.getVarText("oscIP");
  oscPort = puara.getVarNumber("oscPORT");
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
  std::cout << " getVarNumber " << std::endl;
  puara.set_settings_changed_handler(onSettingsChanged);
  oscIP = puara.getVarText("oscIP");
  std::cout << " getVarText " << std::endl;
  oscPort = puara.getVarNumber("oscPORT");
  std::cout << " getVarNumber " << std::endl;

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
    std::cout << "capacitive touch sensor initialization failed!" << std::endl;
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
  sensors.touch = touch.getValue();
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
/*  if (!oscIP.empty() && oscIP != "0.0.0.0") {*/

    OSCBundle bundle;
      std::cout << "OSC Bundle created" << std::endl;
    //add a new OSCMessage to the bundle with the address "/a"
    // store as a reference to avoid making a copy (copying causes double-free and
    // corrupts the heap when the temporary is destroyed).
    OSCMessage &msgA = bundle.add(("/" + puara.dmi_name() + "/IMU").c_str());
      std::cout << "OSCMessage created" << std::endl;
    msgA.add(puaraIMU.accl.x)
        .add(puaraIMU.accl.y)
        .add(puaraIMU.accl.z);
      std::cout << "OSCMessage populated" << std::endl;

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
    
    //  msg1.add(sensor_analog);
    //  msg1.add(button);

    Udp.beginPacket(oscIP.c_str(), oscPort);
      std::cout << "UDP begin packet" << std::endl;
    bundle.send(Udp);
      std::cout << "OSC Bundle sent" << std::endl;
    Udp.endPacket();
      std::cout << "UDP end packet" << std::endl;
    bundle.empty();
      std::cout << "OSC Bundle emptied" << std::endl;

 // }

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
