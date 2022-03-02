
// Button (or button) function

#include "buttonGuitarAMI.h"

void ButtonGuitarAMI::readButton(int value) {
    buttonState = value;
    if (buttonState) {
        if (!ButtonGuitarAMI::button) {
            ButtonGuitarAMI::button = true;
            ButtonGuitarAMI::timer = esp_timer_get_time();
        }
        if (esp_timer_get_time() - ButtonGuitarAMI::timer > ButtonGuitarAMI::holdInterval) {
            ButtonGuitarAMI::hold = true;
        }
    }
    else if (ButtonGuitarAMI::hold) {
        ButtonGuitarAMI::hold = false;
        ButtonGuitarAMI::button = false;
        ButtonGuitarAMI::count = 0;
    }
    else {
        if (ButtonGuitarAMI::button) {
            ButtonGuitarAMI::button = false;
            ButtonGuitarAMI::pressTime = esp_timer_get_time() - ButtonGuitarAMI::timer;
            ButtonGuitarAMI::timer = esp_timer_get_time();
            ButtonGuitarAMI::count++;
        }
    }
    if (!ButtonGuitarAMI::button && (esp_timer_get_time() - ButtonGuitarAMI::timer > ButtonGuitarAMI::countInterval)) {
        switch (ButtonGuitarAMI::count) {
            case 0:
                ButtonGuitarAMI::tap = 0;
                ButtonGuitarAMI::dtap = 0;
                ButtonGuitarAMI::ttap = 0;
                break;
            case 1: 
                ButtonGuitarAMI::tap = 1;
                ButtonGuitarAMI::dtap = 0;
                ButtonGuitarAMI::ttap = 0;
                break;
            case 2:
                ButtonGuitarAMI::tap = 0;
                ButtonGuitarAMI::dtap = 1;
                ButtonGuitarAMI::ttap = 0;
                break;
            case 3:
                ButtonGuitarAMI::tap = 0;
                ButtonGuitarAMI::dtap = 0;
                ButtonGuitarAMI::ttap = 1;
                break;
        }
        ButtonGuitarAMI::count = 0;
    }
}

unsigned int ButtonGuitarAMI::getCount() {
    return ButtonGuitarAMI::count;
};

bool ButtonGuitarAMI::getButton() {
    return ButtonGuitarAMI::button;
}

unsigned int ButtonGuitarAMI::getState() {
    return ButtonGuitarAMI::buttonState;
};

unsigned int ButtonGuitarAMI::getTap() {
    return ButtonGuitarAMI::tap;
};

unsigned int ButtonGuitarAMI::getDTap() {
    return ButtonGuitarAMI::dtap;
};

unsigned int ButtonGuitarAMI::getTTap() {
    return ButtonGuitarAMI::ttap;
};

unsigned int ButtonGuitarAMI::getPressTime() {
    return ButtonGuitarAMI::pressTime;
}

bool ButtonGuitarAMI::getHold() {
    return ButtonGuitarAMI::hold;
}

unsigned int ButtonGuitarAMI::getHoldInterval() {
    return ButtonGuitarAMI::holdInterval;
}

unsigned int ButtonGuitarAMI::setHoldInterval(int value) {
    ButtonGuitarAMI::holdInterval = value;
    return 1;
}

