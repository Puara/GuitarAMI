//****************************************************************************//
// GuitarAMI Module                                                           //
// Input Devices and Music Interaction Laboratory (IDMIL), McGill University  //
// Edu Meneses (2022) - https://www.edumeneses.com                            //
//****************************************************************************//

/* Created using the Puara template: https://github.com/Puara/puara-module-template 
 * The template contains a fully commented version for the commonly used commands 
 */


unsigned int firmware_version = 220906;

#include "Arduino.h"

// For disabling power saving
#include "esp_wifi.h"

#include <puara.h>
#include <puara_gestures.h>
#include <mapper.h>

#include <deque>
#include <cmath>
#include <algorithm>

/* (Un)comment the following lines as some GuitarAMi modules
 * (e.g., GuitarAMI module #003) use the BNO080 IMU
 */
#define imu_LSM9DS1
// #define imu_BNO080

// initializing libmapper, puara, puara-gestures, and liblo client
mpr_dev lm_dev = 0;
Puara puara;
PuaraGestures gestures;
lo_address osc1;
lo_address osc2;
std::string baseNamespace = "/";
std::string oscNamespace;

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
    #include "TinyPICO.h"
    Pin pin{ 5, 4, 35, 32, 33 };
    TinyPICO tinypico = TinyPICO();
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

// read battery level (based on https://www.youtube.com/watch?v=yZjpYmWVLh8&feature=youtu.be&t=88) 
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

//////////////////////
// Liblo OSC server //
//////////////////////

void error(int num, const char *msg, const char *path) {
    printf("Liblo server error %d in path %s: %s\n", num, path, msg);
    fflush(stdout);
}
lo_server_thread osc_server;

int generic_handler(const char *path, const char *types, lo_arg ** argv,
                    int argc, lo_message data, void *user_data) {
    for (int i = 0; i < argc; i++) {
        printf("arg %d '%c' ", i, types[i]);
        lo_arg_pp((lo_type)types[i], argv[i]);
        printf("\n");
    }
    printf("\n");
    fflush(stdout);

    return 1;
}

////////////////////////////////////////////////////////
// Libmapper stuff with specific forward declarations //
////////////////////////////////////////////////////////

struct Lm {
    mpr_sig ult = 0;
    int ultMax = 500;
    int ultMin = 0;
    mpr_sig accel = 0;
    float accelMax = 50;
    float accelMin = -50;
    mpr_sig gyro = 0;
    float gyroMax = 25;
    float gyroMin = -25;
    mpr_sig mag = 0;
    float magMax = 25;
    float magMin = -25;
    mpr_sig quat = 0;
    float quatMax = 1;
    float quatMin = -1;
    mpr_sig euler = 0;
    float eulerMax = 180;
    float eulerMin = -180;
    mpr_sig shake = 0;
    float shakeMax =  50;
    float shakeMin = -50;
    mpr_sig jab = 0;
    float jabMax = 50;
    float jabMin = -50;
    mpr_sig touch = 0;
    int touchMax = 1024;
    int touchMin = 0;
    mpr_sig count = 0;
    int countMax = 100;
    int countMin = 0;
    mpr_sig tap = 0;
    mpr_sig ttap = 0;
    mpr_sig dtap = 0;
    int tapMax = 1;
    int tapMin = 0;
    mpr_sig bat = 0;
    int batMax = 100;
    int batMin = 0;
} lm;

struct Sensors {
    float accl [3];
    float gyro [3];
    float mag [3];
    float quat [4];
    float euler [3];
    float shake [3];
    float jab [3];
    int touch; 
    int count;
    int tap;
    int dtap;
    int ttap;
    int ultDistance;
    int ultTrigger;
    int battery;
} sensors;

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

///////////
// setup //
///////////

void setup() {
    #ifdef Arduino_h
        Serial.begin(115200);
    #endif

    // Disable WiFi power save
    esp_wifi_set_ps(WIFI_PS_NONE);

    puara.set_version(firmware_version);
    puara.start();
    baseNamespace.append(puara.get_dmi_name());
    baseNamespace.append("/");
    oscNamespace = baseNamespace;

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

    std::cout << "    Initializing Liblo server/client... ";
    osc1 = lo_address_new(puara.getIP1().c_str(), puara.getPORT1Str().c_str());
    osc2 = lo_address_new(puara.getIP2().c_str(), puara.getPORT2Str().c_str());
    osc_server = lo_server_thread_new(puara.getLocalPORTStr().c_str(), error);
    lo_server_thread_add_method(osc_server, NULL, NULL, generic_handler, NULL);
    lo_server_thread_start(osc_server);
    std::cout << "done" << std::endl;

    std::cout << "    Initializing Libmapper device/signals... ";
    lm_dev = mpr_dev_new(puara.get_dmi_name().c_str(), 0);
    lm.ult = mpr_sig_new(lm_dev, MPR_DIR_OUT, "ult", 1, MPR_FLT, "mm", &lm.ultMin, &lm.ultMax, 0, 0, 0);
    lm.accel = mpr_sig_new(lm_dev, MPR_DIR_OUT, "accel", 3, MPR_FLT, "m/s^2",  &lm.accelMin, &lm.accelMax, 0, 0, 0);
    lm.gyro = mpr_sig_new(lm_dev, MPR_DIR_OUT, "gyro", 3, MPR_FLT, "rad/s", &lm.gyroMin, &lm.gyroMax, 0, 0, 0);
    lm.mag = mpr_sig_new(lm_dev, MPR_DIR_OUT, "mag", 3, MPR_FLT, "uTesla", &lm.magMin, &lm.magMax, 0, 0, 0);
    lm.quat = mpr_sig_new(lm_dev, MPR_DIR_OUT, "quat", 4, MPR_FLT, "qt", &lm.quatMin, &lm.quatMax, 0, 0, 0);
    lm.euler = mpr_sig_new(lm_dev, MPR_DIR_OUT, "euler", 3, MPR_FLT, "fl", &lm.eulerMin, &lm.eulerMax, 0, 0, 0);
    lm.shake = mpr_sig_new(lm_dev, MPR_DIR_OUT, "shake", 3, MPR_FLT, "fl", &lm.shakeMin, &lm.shakeMax, 0, 0, 0);
    lm.jab = mpr_sig_new(lm_dev, MPR_DIR_OUT, "jab", 3, MPR_FLT, "fl", &lm.jabMin, &lm.jabMax, 0, 0, 0);
    lm.touch = mpr_sig_new(lm_dev, MPR_DIR_OUT, "touch", 1, MPR_INT32, "un", &lm.touchMin, &lm.touchMax, 0, 0, 0);
    lm.count = mpr_sig_new(lm_dev, MPR_DIR_OUT, "count", 1, MPR_INT32, "un", &lm.countMin, &lm.countMax, 0, 0, 0);
    lm.tap = mpr_sig_new(lm_dev, MPR_DIR_OUT, "tap", 1, MPR_INT32, "un", &lm.tapMin, &lm.tapMax, 0, 0, 0);
    lm.ttap = mpr_sig_new(lm_dev, MPR_DIR_OUT, "triple tap", 1, MPR_INT32, "un", &lm.tapMin, &lm.tapMax, 0, 0, 0);
    lm.dtap = mpr_sig_new(lm_dev, MPR_DIR_OUT, "double tap", 1, MPR_INT32, "un", &lm.tapMin, &lm.tapMax, 0, 0, 0);
    lm.bat = mpr_sig_new(lm_dev, MPR_DIR_OUT, "battery", 1, MPR_FLT, "percent", &lm.batMin, &lm.batMax, 0, 0, 0);
    std::cout << "done" << std::endl;
    
    delay(500);
    Serial.println(); 
    Serial.println(puara.get_dmi_name().c_str());
    Serial.println("Edu Meneses\nMetalab - Société des Arts Technologiques (SAT)\nIDMIL - CIRMMT - McGill University");
    Serial.print("Firmware version: "); Serial.println(firmware_version); Serial.println("\n");
}

//////////
// loop //
//////////

void loop() {

    mpr_dev_poll(lm_dev, 0);

    // Read Ultrasonic sensor distance
    readUlt();

    // Read capacitive button
    touch.readTouch();

    // read battery
    if (millis() - battery.interval > battery.timer) {
      battery.timer = millis();
      readBattery();
      batteryFilter();
    }

    gestures.updateJabShake(imu.getGyroX(), imu.getGyroY(), imu.getGyroZ());
    gestures.updateButton(touch.getValue());

    // Preparing arrays for libmapper signals
        sensors.ultDistance = getUltDistance();
        sensors.touch = touch.getValue();
        sensors.accl[0] = imu.getAccelX();
        sensors.accl[1] = imu.getAccelY();
        sensors.accl[2] = imu.getAccelZ();
        sensors.gyro[0] = imu.getGyroX();
        sensors.gyro[1] = imu.getGyroY();
        sensors.gyro[2] = imu.getGyroZ();
        sensors.mag[0] = imu.getMagX();
        sensors.mag[1] = imu.getMagY();
        sensors.mag[2] = imu.getMagZ();
        sensors.quat[0] = imu.getQuatI();
        sensors.quat[1] = imu.getQuatJ();
        sensors.quat[2] = imu.getQuatK();
        sensors.quat[3] = imu.getQuatReal();
        sensors.euler[0] = imu.getYaw();
        sensors.euler[2] = imu.getPitch();
        sensors.euler[3] = imu.getRoll();
    if (sensors.shake[0] != gestures.getShakeX() || sensors.shake[1] != gestures.getShakeY() || sensors.shake[2] != gestures.getShakeZ()) {
        sensors.shake[0] = gestures.getShakeX();
        sensors.shake[1] = gestures.getShakeY();
        sensors.shake[2] = gestures.getShakeZ();
        event.shake = true;
    } else { event.shake = false; }
    if (sensors.jab[0] != gestures.getJabX() || sensors.jab[1] != gestures.getJabY() || sensors.jab[2] != gestures.getJabZ()) {
        sensors.jab[0] = gestures.getJabX();
        sensors.jab[1] = gestures.getJabY();
        sensors.jab[2] = gestures.getJabZ();
        event.jab = true;
    } else { event.jab = false; }
    if (sensors.count != gestures.getButtonCount()) {sensors.count = gestures.getButtonCount(); event.count = true; } else { event.count = false; }
    if (sensors.tap != gestures.getButtonTap()) {sensors.tap = gestures.getButtonTap(); event.tap = true; } else { event.tap = false; }
    if (sensors.dtap != gestures.getButtonDTap()) {sensors.dtap = gestures.getButtonDTap(); event.dtap = true; } else { event.dtap = false; }
    if (sensors.ttap != gestures.getButtonTTap()) {sensors.ttap = gestures.getButtonTTap(); event.ttap = true; } else { event.ttap = false; }
    if (sensors.ultTrigger != getUltTrigger()) {sensors.ultTrigger = getUltTrigger(); event.ultTrigger = true; } else { event.ultTrigger = false; }
    if (sensors.battery != battery.percentage) {sensors.battery = battery.percentage; event.battery = true; } else { event.battery = false; }

    // updating libmapper signals
    mpr_sig_set_value(lm.ult, 0, 1, MPR_FLT, &sensors.ultDistance);
    mpr_sig_set_value(lm.accel, 0, 3, MPR_FLT, &sensors.accl);
    mpr_sig_set_value(lm.gyro, 0, 3, MPR_FLT, &sensors.gyro);
    mpr_sig_set_value(lm.mag, 0, 3, MPR_FLT, &sensors.mag);
    mpr_sig_set_value(lm.quat, 0, 4, MPR_FLT, &sensors.quat);
    mpr_sig_set_value(lm.euler, 0, 3, MPR_FLT, &sensors.euler);
    mpr_sig_set_value(lm.shake, 0, 3, MPR_FLT, &sensors.shake);
    mpr_sig_set_value(lm.jab, 0, 3, MPR_FLT, &sensors.jab);
    mpr_sig_set_value(lm.touch, 0, 1, MPR_INT32, &sensors.touch);
    mpr_sig_set_value(lm.count, 0, 1, MPR_INT32, &sensors.count);
    mpr_sig_set_value(lm.tap, 0, 1, MPR_INT32, &sensors.tap);
    mpr_sig_set_value(lm.ttap, 0, 1, MPR_INT32, &sensors.dtap);
    mpr_sig_set_value(lm.dtap, 0, 1, MPR_INT32, &sensors.ttap);
    mpr_sig_set_value(lm.bat, 0, 1, MPR_FLT, &sensors.battery);

    // Sending continuous OSC messages
    if (puara.IP1_ready()) {
            oscNamespace.replace(oscNamespace.begin()+baseNamespace.size(),oscNamespace.end(), "ult");
            lo_send(osc1, oscNamespace.c_str(), "i", sensors.ultDistance);
            oscNamespace.replace(oscNamespace.begin()+baseNamespace.size(),oscNamespace.end(), "touch");
            lo_send(osc1, oscNamespace.c_str(), "i", sensors.touch);
            oscNamespace.replace(oscNamespace.begin()+baseNamespace.size(),oscNamespace.end(), "accl");
            lo_send(osc1, oscNamespace.c_str(), "fff", sensors.accl[0], sensors.accl[1], sensors.accl[2]);
            oscNamespace.replace(oscNamespace.begin()+baseNamespace.size(),oscNamespace.end(), "gyro");
            lo_send(osc1, oscNamespace.c_str(), "fff", sensors.gyro[0], sensors.gyro[1], sensors.gyro[2]);
            oscNamespace.replace(oscNamespace.begin()+baseNamespace.size(),oscNamespace.end(), "mag");
            lo_send(osc1, oscNamespace.c_str(), "fff", sensors.mag[0], sensors.mag[1], sensors.mag[2]);
            oscNamespace.replace(oscNamespace.begin()+baseNamespace.size(),oscNamespace.end(), "quat");
            lo_send(osc1, oscNamespace.c_str(), "ffff", sensors.quat[0], sensors.quat[1], sensors.quat[2], sensors.quat[3]);
            oscNamespace.replace(oscNamespace.begin()+baseNamespace.size(),oscNamespace.end(), "euler");
            lo_send(osc1, oscNamespace.c_str(), "fff", sensors.euler[0], sensors.euler[1], sensors.euler[2]);
    }
    if (puara.IP2_ready()) {
            oscNamespace.replace(oscNamespace.begin()+baseNamespace.size(),oscNamespace.end(), "ult");
            lo_send(osc2, oscNamespace.c_str(), "i", sensors.ultDistance);
            oscNamespace.replace(oscNamespace.begin()+baseNamespace.size(),oscNamespace.end(), "touch");
            lo_send(osc2, oscNamespace.c_str(), "i", sensors.touch);
            oscNamespace.replace(oscNamespace.begin()+baseNamespace.size(),oscNamespace.end(), "accl");
            lo_send(osc2, oscNamespace.c_str(), "fff", sensors.accl[0], sensors.accl[1], sensors.accl[2]);
            oscNamespace.replace(oscNamespace.begin()+baseNamespace.size(),oscNamespace.end(), "gyro");
            lo_send(osc2, oscNamespace.c_str(), "fff", sensors.gyro[0], sensors.gyro[1], sensors.gyro[2]);
            oscNamespace.replace(oscNamespace.begin()+baseNamespace.size(),oscNamespace.end(), "mag");
            lo_send(osc2, oscNamespace.c_str(), "fff", sensors.mag[0], sensors.mag[1], sensors.mag[2]);
            oscNamespace.replace(oscNamespace.begin()+baseNamespace.size(),oscNamespace.end(), "quat");
            lo_send(osc2, oscNamespace.c_str(), "ffff", sensors.quat[0], sensors.quat[1], sensors.quat[2], sensors.quat[3]);
            oscNamespace.replace(oscNamespace.begin()+baseNamespace.size(),oscNamespace.end(), "euler");
            lo_send(osc2, oscNamespace.c_str(), "fff", sensors.euler[0], sensors.euler[1], sensors.euler[2]);
    }

    // Sending discrete OSC messages
    if (puara.IP1_ready()) {
        if (event.shake) {
            oscNamespace.replace(oscNamespace.begin()+baseNamespace.size(),oscNamespace.end(), "shake");
            lo_send(osc1, oscNamespace.c_str(), "fff", sensors.shake[0], sensors.shake[1], sensors.shake[2]);
        }
        if (event.jab) {
            oscNamespace.replace(oscNamespace.begin()+baseNamespace.size(),oscNamespace.end(), "jab");
            lo_send(osc1, oscNamespace.c_str(), "fff", sensors.jab[0], sensors.jab[1], sensors.jab[2]);
        }
        if (event.count) {
            oscNamespace.replace(oscNamespace.begin()+baseNamespace.size(),oscNamespace.end(), "count");
            lo_send(osc1, oscNamespace.c_str(), "i", sensors.count);
        }
        if (event.tap) {
            oscNamespace.replace(oscNamespace.begin()+baseNamespace.size(),oscNamespace.end(), "tap");
            lo_send(osc1, oscNamespace.c_str(), "i", sensors.tap);
        }
        if (event.dtap) {
            oscNamespace.replace(oscNamespace.begin()+baseNamespace.size(),oscNamespace.end(), "dtap");
            lo_send(osc1, oscNamespace.c_str(), "i", sensors.dtap);
        }
        if (event.ttap) {
            oscNamespace.replace(oscNamespace.begin()+baseNamespace.size(),oscNamespace.end(), "ttap");
            lo_send(osc1, oscNamespace.c_str(), "i", sensors.ttap);
        }
        if (event.battery) {
            oscNamespace.replace(oscNamespace.begin()+baseNamespace.size(),oscNamespace.end(), "battery");
            lo_send(osc1, oscNamespace.c_str(), "i", sensors.battery);
        }
    }
    if (puara.IP2_ready()) {
        if (event.shake) {
            oscNamespace.replace(oscNamespace.begin()+baseNamespace.size(),oscNamespace.end(), "shake");
            lo_send(osc2, oscNamespace.c_str(), "fff", sensors.shake[0], sensors.shake[1], sensors.shake[2]);
        }
        if (event.jab) {
            oscNamespace.replace(oscNamespace.begin()+baseNamespace.size(),oscNamespace.end(), "jab");
            lo_send(osc2, oscNamespace.c_str(), "fff", sensors.jab[0], sensors.jab[1], sensors.jab[2]);
        }
        if (event.count) {
            oscNamespace.replace(oscNamespace.begin()+baseNamespace.size(),oscNamespace.end(), "count");
            lo_send(osc2, oscNamespace.c_str(), "i", sensors.count);
        }
        if (event.tap) {
            oscNamespace.replace(oscNamespace.begin()+baseNamespace.size(),oscNamespace.end(), "tap");
            lo_send(osc2, oscNamespace.c_str(), "i", sensors.tap);
        }
        if (event.dtap) {
            oscNamespace.replace(oscNamespace.begin()+baseNamespace.size(),oscNamespace.end(), "dtap");
            lo_send(osc2, oscNamespace.c_str(), "i", sensors.dtap);
        }
        if (event.ttap) {
            oscNamespace.replace(oscNamespace.begin()+baseNamespace.size(),oscNamespace.end(), "ttap");
            lo_send(osc2, oscNamespace.c_str(), "i", sensors.ttap);
        }
        if (event.battery) {
            oscNamespace.replace(oscNamespace.begin()+baseNamespace.size(),oscNamespace.end(), "battery");
            lo_send(osc2, oscNamespace.c_str(), "i", sensors.battery);
        }
    }

    // Set LED - connection status and battery level
    #ifdef ARDUINO_LOLIN_D32_PRO
        if (battery.percentage < 10) {        // low battery - flickering
        led.setInterval(75);
        led_var.ledValue = led.blink(255, 50);
        ledcWrite(0, led_var.ledValue);
        } else {
            if (puara.get_StaIsConnected()) { // blinks when connected, cycle when disconnected
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
            if (puara.get_StaIsConnected()) {         // blinks when connected, cycle when disconnected
                led.setInterval(1000);                // RGB: 0, 128, 255 (Dodger Blue)
                led_var.color = led.blink(255,20);
                tinypico.DotStar_SetPixelColor(0, uint8_t(led_var.color/2), led_var.color);
            } else {
                led.setInterval(4000);
                led_var.color = led.cycle(led_var.color, 0, 255);
                tinypico.DotStar_SetPixelColor(0, uint8_t(led_var.color/2), led_var.color);
            }
        }
    #endif    

    // run at 100 Hz
    //vTaskDelay(10 / portTICK_PERIOD_MS);
}
