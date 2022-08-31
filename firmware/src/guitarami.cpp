/* GuitarAMI cpp file
 * Input Devices and Music Interaction Laboratory (IDMIL), McGill University
 * Edu Meneses (2022) - https://www.edumeneses.com
 */

#include "guitarami.h"


#ifdef imu_BNO080
    Imu_BNO080 imu;
#endif
#ifdef imu_LSM9DS1
    Imu_LSM9DS1 imu;
#endif

#ifdef board_WEMOS
    Pin pin{ 5, 15, 35, 32, 33 };
#elif defined(board_TINYPICO)
    Pin pin{ 5, 4, 35, 32, 33 };
    TinyPICO tinypico = TinyPICO();
#endif

Led led;
Global global;
Lm lm;
BatteryData battery;

/////////////
// Battery //
/////////////

// read battery level (based on https://www.youtube.com/watch?v=yZjpYmWVLh8&feature=youtu.be&t=88) 
void readBattery() {
    #ifdef board_WEMOS
        battery.value = analogRead(pin.battery) / 4096.0 * 7.445;
    #elif defined(board_TINYPICO)
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