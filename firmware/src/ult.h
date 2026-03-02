/* GuitarAMI Ultrassonic Sensor header file
 * Input Devices and Music Interaction Laboratory (IDMIL), McGill University
 * Edu Meneses (2022) - https://www.edumeneses.com
 */

#ifndef ULT_H
#define ULT_H

#include "Arduino.h"
#include <deque>

struct UltData {
    unsigned int trig;
    unsigned int echo;
    unsigned int distance;
    unsigned int tempDistance;
    unsigned int lastDistance;
    unsigned int ultMaxDistance = 200;
    unsigned int ultMinDistance = 20;
    int trigger;
    unsigned long duration;
    unsigned long timer = 0;
    unsigned long trigTimer = 0;
    int interval = 20; // in ms (1/f)
    int trigInterval = 200; // in ms (1/f)
    int queueAmount = 10; // # of values stored
    std::deque<int> filterArray; // store last values
};

int initUlt(int trig, int echo);
void ultFilter();
void readUlt();
int getUltTrigger();
int getUltDistance();

#endif