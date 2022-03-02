// Button (or button) function

#ifndef BUTTONGAMI_H
#define BUTTONGAMI_H

#include "esp_timer.h"

class ButtonGuitarAMI {
    private:
        unsigned int count;
        unsigned int countInterval = 200;
        int buttonState = 0;
        bool button = false;
        unsigned int tap;
        unsigned int dtap;
        unsigned int ttap;
        bool hold;
        unsigned int holdInterval = 5000;
        unsigned int filterPeriod = 10;
        long timer;
        unsigned long pressTime;
    public:
        void readButton(int value);
        unsigned int getCount();
        bool getButton();
        unsigned int getPressTime();
        unsigned int getState();
        unsigned int getTap();
        unsigned int getDTap();
        unsigned int getTTap();
        bool getHold();
        unsigned int getHoldInterval();
        unsigned int setHoldInterval(int value);
};

#endif