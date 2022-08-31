/* GuitarAMI Ultrassonic Sensor cpp file
 * Input Devices and Music Interaction Laboratory (IDMIL), McGill University
 * Edu Meneses (2022) - https://www.edumeneses.com
 */

#include "ult.h"

UltData ultData;

int initUlt(int trig, int echo) {
    pinMode(trig, OUTPUT); // Sets the trigger pin as an OUTPUT
    pinMode(echo, INPUT); // Sets the echo pin as an INPUT
    ultData.trig = trig;
    ultData.echo = echo;
    return 1;
}

void readUlt() {
    if (millis() - ultData.interval > ultData.timer) {
        ultData.lastDistance = ultData.tempDistance;
        digitalWrite(ultData.trig, LOW);
        delayMicroseconds(2);
        // Sets the trigger pin HIGH (ACTIVE) for 10 microseconds
        digitalWrite(ultData.trig, HIGH);
        delayMicroseconds(10);
        digitalWrite(ultData.trig, LOW);
        // Reads the echo pin, returns the sound wave travel time in microseconds
        ultData.duration = pulseIn(ultData.echo, HIGH);
        // Calculating the distance in mm
        ultData.tempDistance = ultData.duration * 0.34 / 2;
        if (ultData.tempDistance < ultData.ultMinDistance || ultData.tempDistance > ultData.ultMaxDistance) {
            ultData.distance = 0;
        } else {
            ultData.distance = ultData.tempDistance;
        }
        if (ultData.lastDistance == 0) {
            ultData.trigTimer = millis();
            if (ultData.tempDistance == 0) {
                ultData.distance = 0;
                ultData.trigger = 0;
            } 
        } else {
            if (millis() - ultData.trigInterval > ultData.trigTimer) {
                ultData.distance = ultData.tempDistance;
                ultData.trigger = 0;
            } else if (ultData.tempDistance == 0 && ultData.distance != 0) {
                ultData.trigger = 1;
            }
        }
        ultFilter();
        ultData.timer = millis();
    }
}

void ultFilter() {
  ultData.filterArray.push_back(ultData.distance);
  if(ultData.filterArray.size() > ultData.queueAmount) {
    ultData.filterArray.pop_front();
  }
  ultData.distance = 0;
  for (int i=0; i<ultData.filterArray.size(); i++) {
    ultData.distance += ultData.filterArray.at(i);
  }
  ultData.distance /= ultData.filterArray.size();
}