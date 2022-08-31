/* GuitarAMI header file
 * Input Devices and Music Interaction Laboratory (IDMIL), McGill University
 * Edu Meneses (2022) - https://www.edumeneses.com
 */


#ifndef GUITARAMI_H
#define GUITARAMI_H

// The GuitarAMI module #003 uses the bno080
//#define imu_BNO080
#define imu_LSM9DS1

// If using TinyPICO (current HW) version or Wemos D32 Pro
#define board_TINYPICO
//#define board_WEMOS

#include "Arduino.h"
#include <mapper.h>  // libmapper
#include <deque>
#include <cmath>          // std::abs
#include <algorithm>      // std::min_element, std::max_element

// internal files
#include "ult.h"
#include "esptouch.h"


/////////////////////
// Pin definitions //
/////////////////////

struct Pin {
    int led;    // Built In LED pin
    int touch;  // Capacitive touch pin
    int battery;// To check battery level (voltage)
    int ultTrig;// connects to the trigger pin on the distance sensor
    int ultEcho;// connects to the echo pin on the distance sensor
};

#ifdef board_TINYPICO
    #include "TinyPICO.h"
#endif

///////////////////////////////
// Firmware global variables //
///////////////////////////////

struct Global {
    int ledValue = 0;
    uint8_t color = 0;
};

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
};

///////////////////
// Battery stuff //
///////////////////
  
struct BatteryData {
    unsigned int percentage = 0;
    unsigned int lastPercentage = 0;
    float value;
    unsigned long timer = 0;
    int interval = 1000; // in ms (1/f)
    int queueAmount = 10; // # of values stored
    std::deque<int> filterArray; // store last values
};

void readBattery();
void batteryFilter();

////////////////////////////////
// Include IMU function files //
////////////////////////////////

  //Choose the file to include according to the IMU used
  
#ifdef imu_BNO080
    #include "bno080.h" 
#endif
#ifdef imu_LSM9DS1
    #include "lsm9ds1.h"
#endif

/////////////////////////////////////////
// Include LED libraries and functions //
/////////////////////////////////////////

#include "led.h"

float hardBlink(unsigned long &timer, int interval, float currentValue);
float rampBlink(unsigned long &timer, int interval, float currentValue, bool &direction);

#endif