#include "ultrasonic.h"

Ultrasonic::Ultrasonic(uint8_t triggerPin, uint8_t echoPin) : triggerPin(triggerPin), echoPin(echoPin) {}

void Ultrasonic::begin() {
  pinMode(triggerPin, OUTPUT);
  digitalWrite(triggerPin, LOW);
  pinMode(echoPin, INPUT);
  attachInterruptArg(echoPin, onEcho, this, CHANGE);
}

void Ultrasonic::update() {
  if (millis() - readTimer < readPeriodMs) return;
  readTimer = millis();

  unsigned int previous = raw;
  raw = readMillimeters();
  ping();

  if (previous == 0) {
    presenceTimer = millis();
    if (raw == 0) {
      distance = 0;
      trigger = false;
    }
    return;
  }

  bool objectSettled = millis() - presenceTimer > triggerWindowMs;
  if (objectSettled) {
    distance = raw;
    trigger = false;
  } else if (raw == 0 && distance != 0) {
    trigger = true;
  }
}

unsigned int Ultrasonic::readMillimeters() {
  if (!echoDone) return 0;
  echoDone = false;
  unsigned int mm = echoMicros / microsPerMm;
  return mm < minRangeMm || mm > maxRangeMm ? 0 : mm;
}

void Ultrasonic::ping() {
  if (digitalRead(echoPin) == HIGH) return;  // previous echo still in flight
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin, LOW);
}

void IRAM_ATTR Ultrasonic::onEcho(void* self) {
  Ultrasonic& sensor = *static_cast<Ultrasonic*>(self);
  if (digitalRead(sensor.echoPin) == HIGH) {
    sensor.echoStart = micros();
  } else {
    sensor.echoMicros = micros() - sensor.echoStart;
    sensor.echoDone = true;
  }
}
