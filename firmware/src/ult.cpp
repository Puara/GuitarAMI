/* GuitarAMI Ultrassonic Sensor cpp file
 * Input Devices and Music Interaction Laboratory (IDMIL), McGill University
 * Edu Meneses (2022) - https://www.edumeneses.com
 */

#include "ult.h"

UltData ult;

int initUlt(int trig, int echo) {
    pinMode(trig, OUTPUT); // Sets the trigger pin as an OUTPUT
    pinMode(echo, INPUT); // Sets the echo pin as an INPUT
    ult.trig = trig;
    ult.echo = echo;
    return 1;
}

void readUlt() {
    if (millis() - ult.interval > ult.timer) {
        ult.lastDistance = ult.tempDistance;
        digitalWrite(ult.trig, LOW);
        delayMicroseconds(2);
        // Sets the trigger pin HIGH (ACTIVE) for 10 microseconds
        digitalWrite(ult.trig, HIGH);
        delayMicroseconds(10);
        digitalWrite(ult.trig, LOW);
        // Reads the echo pin, returns the sound wave travel time in microseconds
        ult.duration = pulseIn(ult.echo, HIGH);
        // Calculating the distance in mm
        ult.tempDistance = ult.duration * 0.34 / 2;
        if (ult.tempDistance < ult.ultMinDistance || ult.tempDistance > ult.ultMaxDistance) {
            ult.distance = 0;
        } else {
            ult.distance = ult.tempDistance;
        }
        if (ult.lastDistance == 0) {
            ult.trigTimer = millis();
            if (ult.tempDistance == 0) {
                ult.distance = 0;
                ult.trigger = 0;
            } 
        } else {
            if (millis() - ult.trigInterval > ult.trigTimer) {
                ult.distance = ult.tempDistance;
                ult.trigger = 0;
            } else if (ult.tempDistance == 0 && ult.distance != 0) {
                ult.trigger = 1;
            }
        }
        ultFilter();
        ult.timer = millis();
    }
}

void ultFilter() {
  ult.filterArray.push_back(ult.distance);
  if(ult.filterArray.size() > ult.queueAmount) {
    ult.filterArray.pop_front();
  }
  ult.distance = 0;
  for (int i=0; i<ult.filterArray.size(); i++) {
    ult.distance += ult.filterArray.at(i);
  }
  ult.distance /= ult.filterArray.size();
}

int getUltTrigger() {
    return ult.trigger;
}


int getUltDistance() {
    return ult.distance;
}