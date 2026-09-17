// GuitarAMI module firmware
// Edu Meneses - SAT / IDMIL
// https://github.com/Puara/GuitarAMI

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <TinyPICO.h>
#include <WiFiUdp.h>
#include <esp_wifi.h>
#include <puara.h>
#include <boost-embedded-190.h>  // headers required by puara-gestures
#include <puara-gestures.h>

#include "battery.h"
#include "config.h"
#include "imu.h"
#include "osc_out.h"
#include "status_led.h"
#include "ultrasonic.h"

Puara puara;
WiFiUDP udp;
TinyPICO board;
OscOut osc(udp);
Imu imu;
Ultrasonic ultrasonic(pins::ultrasonicTrigger, pins::ultrasonicEcho);
Battery battery(board);
StatusLed led(board);
puara_gestures::Button touch;
puara_gestures::Shake3D shake(&imu.reading.gyro);
puara_gestures::Jab3D jab(&imu.reading.gyro);

unsigned int touchThreshold = defaults::touchThreshold;
unsigned int touchValue = 0;
bool stationConnected = false;
unsigned long oscTimer = 0;
unsigned long linkTimer = 0;

struct Snapshot {
  unsigned int count = 0;
  unsigned int tap = 0;
  unsigned int doubleTap = 0;
  unsigned int tripleTap = 0;
  unsigned int distance = 0;
  unsigned int battery = 0;
  bool trigger = false;
  puara_gestures::Coord3D jab;
  puara_gestures::Coord3D shake;
} lastSent;

void applySettings() {
  udp.stop();
  udp.begin(puara.getVarNumber("localPORT"));
  osc.setDestination(0, puara.getVarText("oscIP1"), puara.getVarNumber("oscPORT1"));
  osc.setDestination(1, puara.getVarText("oscIP2"), puara.getVarNumber("oscPORT2"));
  touchThreshold = puara.getVarNumber("touchThreshold");
}

void onOscMessage(MicroOscMessage& message) {
  if (message.checkOscAddress("/state/info")) {
    osc.send("info", puara.dmi_name(), FIRMWARE_VERSION);
  }
}

void setup() {
  Serial.begin(115200);

  puara.set_version(FIRMWARE_VERSION);
  puara.start();
  esp_wifi_set_ps(WIFI_PS_NONE);

  osc.setNamespace(puara.dmi_name());
  applySettings();
  puara.set_settings_changed_handler(applySettings);

  ArduinoOTA.setHostname(puara.dmi_name().c_str());
  ArduinoOTA.begin();

  led.begin();
  ultrasonic.begin();
  jab.threshold(defaults::jabThreshold);
  Serial.printf("IMU: %s\n", imu.begin() ? "ready" : "not found");
  Serial.printf("%s ready\n", puara.dmi_name().c_str());
}

void loop() {
  ArduinoOTA.handle();
  readSensors();

  unsigned long now = millis();
  if (now - oscTimer >= rates::oscPeriodMs) {
    oscTimer = now - (now - oscTimer) % rates::oscPeriodMs;  // stay on the 10 ms grid
    sendContinuous();
  }
  sendDiscrete();
  osc.receive(onOscMessage);

  if (now - linkTimer >= rates::linkCheckPeriodMs) {
    linkTimer = now;
    wifi_ap_record_t accessPoint;
    stationConnected = esp_wifi_sta_get_ap_info(&accessPoint) == ESP_OK;
  }
  led.update(stationConnected ? StatusLed::Link::Station : StatusLed::Link::AccessPoint,
             battery.percentage < defaults::lowBatteryPercent);
}

void readSensors() {
  if (imu.update()) {
    shake.update();
    jab.update();
  }
  touchValue = touchRead(pins::touch);
  touch.update(touchValue < touchThreshold);
  ultrasonic.update();
  battery.update();
}

void sendContinuous() {
  const puara_gestures::Imu9Axis& r = imu.reading;
  osc.send("accl", r.accl.x, r.accl.y, r.accl.z);
  osc.send("gyro", r.gyro.x, r.gyro.y, r.gyro.z);
  osc.send("magn", r.magn.x, r.magn.y, r.magn.z);
  osc.send("quat", imu.quaternion.x, imu.quaternion.y, imu.quaternion.z, imu.quaternion.w);
  osc.send("ypr", imu.euler.yaw, imu.euler.pitch, imu.euler.roll);
  osc.send("touch", touchValue);
}

void sendDiscrete() {
  sendIfChanged(lastSent.count, touch.count, "count");
  sendIfChanged(lastSent.tap, touch.tap, "tap");
  sendIfChanged(lastSent.doubleTap, touch.doubleTap, "dtap");
  sendIfChanged(lastSent.tripleTap, touch.tripleTap, "ttap");
  sendIfChanged(lastSent.distance, ultrasonic.distance, "ult");
  sendIfChanged(lastSent.trigger, ultrasonic.trigger, "ultTrig");
  sendIfChanged(lastSent.jab, jab.current_value(), "jab");
  sendIfChanged(lastSent.shake, shake.current_value(), "shake");
  sendIfChanged(lastSent.battery, battery.percentage, "battery");
}

void sendIfChanged(unsigned int& previous, unsigned int current, const char* name) {
  if (previous == current) return;
  previous = current;
  osc.send(name, current);
}

void sendIfChanged(bool& previous, bool current, const char* name) {
  if (previous == current) return;
  previous = current;
  osc.send(name, current ? 1 : 0);
}

void sendIfChanged(puara_gestures::Coord3D& previous, puara_gestures::Coord3D current, const char* name) {
  if (previous.x == current.x && previous.y == current.y && previous.z == current.z) return;
  previous = current;
  osc.send(name, current.x, current.y, current.z);
}
