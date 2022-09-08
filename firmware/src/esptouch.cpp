/* GuitarAMI ESP32 touch cpp file
 * Input Devices and Music Interaction Laboratory (IDMIL), McGill University
 * Edu Meneses (2022) - https://www.edumeneses.com
 */

#include "esptouch.h"

unsigned long IRAM_ATTR millis() {
    return (unsigned long) (esp_timer_get_time() / 1000LL);
}

void Touch::readTouch() {
    uint16_t touchValue;
    touch_pad_read_filtered(Touch::pin, &touchValue);
    Touch::value = touchValue;
    if (touchValue < Touch::sensitivity) {
        if (!Touch::touch) {
            Touch::touch = true;
            Touch::timer = millis();
        }
        if (millis() - Touch::timer > Touch::holdInterval) {
            Touch::hold = true;
        }
    }
    else if (Touch::hold) {
        Touch::hold = false;
        Touch::touch = false;
    }
    else {
        if (Touch::touch) {
            Touch::touch = false;
            Touch::pressTime = millis() - Touch::timer;
            Touch::timer = millis();
        }
    }
}

bool Touch::initTouch(void) {
        touch_pad_init();
        touch_pad_config(Touch::pin, Touch::sensitivity);
        touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
        touch_pad_filter_start(Touch::filterPeriod);
    return 1;
}

bool Touch::getTouch() {
    return Touch::touch;
}

unsigned int Touch::getValue() {
    return Touch::value;
};

unsigned int Touch::getSensitivity() {
    return Touch::sensitivity;
};

unsigned int Touch::setSensitivity(int value) {
    Touch::sensitivity = value;
    return 1;
};

unsigned int Touch::getPressTime() {
    return Touch::pressTime;
}

bool Touch::getHold() {
    return Touch::hold;
}

unsigned int Touch::getHoldInterval() {
    return Touch::holdInterval;
}

unsigned int Touch::setHoldInterval(int value) {
    Touch::holdInterval = value;
    return 1;
}