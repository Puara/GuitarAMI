/* GuitarAMI ESP32 touch header file
 * Input Devices and Music Interaction Laboratory (IDMIL), McGill University
 * Edu Meneses (2022) - https://www.edumeneses.com
 */

#ifndef ESPTOUCH_H
#define ESPTOUCH_H

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/touch_pad.h"
#include "esp_log.h"

class Touch {
    private:
        touch_pad_t pin = TOUCH_PAD_NUM3; // pin 15 on Wemos D32 Pro and TinyPICO 
        unsigned int countInterval = 200;
        uint16_t value;
        bool touch = false;
        bool hold;
        unsigned int holdInterval = 5000;
        unsigned int filterPeriod = 10;
        long timer;
        unsigned long pressTime;
        unsigned int sensitivity = 800;
    public:
        bool initTouch(void);
        void readTouch();
        bool getTouch();
        unsigned int getPressTime();
        unsigned int getValue();
        bool getHold();
        unsigned int getHoldInterval();
        unsigned int setHoldInterval(int value);
        unsigned int getSensitivity();
        unsigned int setSensitivity(int value);
};

#endif