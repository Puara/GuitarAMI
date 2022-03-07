
// GuitarAMI M5StickC - Main file
// Edu Meneses
// IDMIL - McGill University (2022)

// Info on the M5StickC: 
//    https://shop.m5stack.com/products/stick-c
//    https://github.com/m5stack/M5StickC


/*
OBS: There's an error with the ESP32 inet.h file. The file requires manual fixing. Check the issue at 
https://github.com/mathiasbredholt/libmapper-arduino/issues/3 
*/


//
// ╺┳┓┏━╸┏━┓┏━┓
//  ┃┃┣╸ ┣━ ┗━┓
// ╺┻┛┗━╸╹  ┗━┛
//

const unsigned int firmware_version = 220301;

// Turn everything related to MIDI off
#define DISABLE_MIDI


//////////////
// Includes //
//////////////

#include <Arduino.h>
#include <Update.h>       // For OTA over web server (manual .bin upload)

#include <deque>
#include <cmath>          // std::abs
#include <algorithm>      // std::min_element, std::max_element

#include "mdns.h"

#include <OSCMessage.h>   // https://github.com/CNMAT/OSC
#include <OSCBundle.h>

#include <M5StickC.h>

///////////////////////////////
// Firmware global variables //
///////////////////////////////

  struct global {
    char deviceName[25];
    char APpasswd[15];
    bool rebootFlag = false;
    unsigned long rebootTimer;
    unsigned int oscDelay = 10; // 10ms ~= 100Hz
    unsigned long messageTimer = 0;
    unsigned int lastCount = 1;
    unsigned int lastTap = 1;
    unsigned int lastDtap = 1;
    unsigned int lastTtap = 1;
    unsigned int lastTrig = 1;
    unsigned int lastDistance = 1;
    float lastFsr = 1;
    float lastShake[3] = {0.1, 0.1, 0.1};
    float lastJab[3] = {0.1, 0.1, 0.1};
    int ledValue = 0;
    uint8_t color = 0;
  } global;

double pi = 3.141592653589793238462643383279502884;

//////////////
// settings //
//////////////

  struct Settings {
    int id = 1;
    char author[20];
    char institution[20];
    char APpasswd[15];
    char lastConnectedNetwork[30];
    char lastStoredPsk[30];
    int32_t firmware;
    char oscIP[2][17] = { {'0','0','0','0'}, {'0','0','0','0'} };
    int32_t oscPORT[2] = {8000, 8000};
    int32_t localPORT = 8000;
    bool libmapper = false;
    bool osc = false;
    int mode = 0; // 0:STA, 1:Setup(STA+AP+WebServer)
  } settings;


////////////////////////////////////////
// Variable to hold M5StickC IMU info //
////////////////////////////////////////

  struct M5imu {
    float accX = 0.0F;
    float accY = 0.0F;
    float accZ = 0.0F;
    float gyroX = 0.0F; // DPS ?
    float gyroY = 0.0F;
    float gyroZ = 0.0F;
    float gyroXrad = 0.0F; // radians per second (if conversion is needed)
    float gyroYrad = 0.0F;
    float gyroZrad = 0.0F;
    float pitch = 0.0F;
    float roll  = 0.0F;
    float yaw   = 0.0F;
    float temp  = 0;
  } m5imu;

//////////////////////////////////////////////////
// Include WiFi and json module functions files //
//////////////////////////////////////////////////

  #include "module.h"

  DNSServer dnsServer;
  AsyncWebServer server(80);
  Module module;
  WiFiUDP oscEndpoint; // A UDP instance to let us send and receive packets over UDP  

  void sendOSC(char* ip,int32_t port, const char* messageNamespace, float data);
  void sendTrioOSC(char* ip,int32_t port, const char* messageNamespace, float data1, float data2, float data3);
  void sendContinuousOSC(char* ip, int32_t port);
  void sendInfo(OSCMessage &msg);
  void receiveOSC();

//////////////////////////////////////////
// Include MIDI libraries and functions //
//////////////////////////////////////////

  #ifndef DISABLE_MIDI

  #include "midi.h"

  Midi midi;

  struct MidiReady {
    unsigned int AccelX; // CC 021 (0b00010101) (for X-axis accelerometer)
    unsigned int AccelZ; // CC 023 (0b00010111) (for Z-axis accelerometer)
    unsigned int AccelY; // CC 022 (0b00010110) (for Y-axis accelerometer)
    unsigned int GyroX;  // CC 024 (0b00011000) (for X-axis gyroscope)
    unsigned int GyroY;  // CC 025 (0b00011001) (for Y-axis gyroscope)
    unsigned int GyroZ;  // CC 026 (0b00011010) (for Z-axis gyroscope)
    unsigned int Yaw;    // CC 027 (0b00011011) (for yaw)
    unsigned int Pitch;  // CC 028 (0b00011100) (for pitch)
    unsigned int Roll;   // CC 029 (0b00011101) (for roll)
    unsigned int ShakeX;
    unsigned int ShakeY;
    unsigned int ShakeZ;
    unsigned int JabX;
    unsigned int JabY;
    unsigned int JabZ;
    unsigned int ButtonA;
    unsigned int ButtonB;
  } midiReady;

  #endif

///////////////////
// Battery stuff //
///////////////////
  
  struct BatteryData {
    unsigned int percentage = 0;
    unsigned int lastPercentage = 0;
    float value;
    unsigned long timer = 0;
    int interval = 1000; // in ms (1/f)
    int queueAmount = 10; // # of values stored
    std::deque<int> filterArray; // store last values
  } battery;

  void readBattery();
  void batteryFilter();


//////////////////////////////////////////////
// Include High-level descriptors functions //
//////////////////////////////////////////////

  #include "instrument.h"

  Instrument instrument;


//////////////////////////
// Forward declarations //
//////////////////////////

  void printVariables();
  void parseJSON();
  void saveJSON();
  void initWebServer();
  void start_mdns_service();
  String indexProcessor(const String& var);
  String scanProcessor(const String& var);
  String factoryProcessor(const String& var);
  String updateProcessor(const String& var);

//
// ┏━┓┏━╸╺┳╸╻ ╻┏━┓
// ┗━┓┣╸  ┃ ┃ ┃┣━┛
// ┗━┛┗━╸ ╹ ┗━┛╹  
// 

void setup() {
  // put your setup code here, to run once:
    Serial.begin(115200);

  // Initialize the M5StickC
    M5.begin();
    M5.IMU.Init();  //Init IMU
    M5.Lcd.setTextColor(ORANGE);  // Set the font color to yellow
    M5.Lcd.setRotation(3);
    M5.Lcd.setTextSize(2);
    M5.Lcd.printf("GuitarAMI\n"
                  "M5StickC\n"
                  "module %03i\n"
                  "Booting...\n",settings.id);

  // Start FS and check Json file (config.json)
    module.mountFS();

  // Load Json file stored values
    parseJSON();
    settings.firmware = firmware_version;
    if (settings.mode < 0 || settings.mode > 1 ){
      settings.mode = 1;
      printf("\nMode error: changing to setup mode for correction\n");
      saveJSON();
    }

  printf( "\n"
        "GuitarAMI module - M5StickC\n"
        "Edu Meneses - IDMIL - CIRMMT - McGill University\n"
        "module ID: %03i\n"
        "Version %d\n"
        "\n"
        "Booting System...\n",settings.id,settings.firmware);

  // Print Json file stored values
    //module.printJSON();
    printVariables();

  // Define this module full name
    snprintf(global.deviceName,(sizeof(global.deviceName)-1),"GuitarAMI_m5_%03i",settings.id);

    module.startWifi(global.deviceName, settings.mode, settings.APpasswd, settings.lastConnectedNetwork, settings.lastStoredPsk);

  // Start listening for incoming OSC messages if WiFi is ON
    oscEndpoint.begin(settings.localPORT);
    printf("Starting UDP (listening to OSC messages)\n");
    printf("Local port: %d\n", settings.localPORT);

  // Start dns and web Server if in setup mode
    if (settings.mode == 1) {
      printf("Starting DNS server\n");
      dnsServer.start(53, "*", WiFi.softAPIP());
      start_mdns_service();
      initWebServer();
    }

  // // Setting Deep sleep wake button
  //   esp_sleep_enable_ext0_wakeup(GPIO_NUM_15,0); // 1 = High, 0 = Low
    
  printf("\n\nBoot complete\n\n"
          "This firmware accepts:\n"
          "- 's' to start setup mode\n"
          "- 'r' to reboot\n"
          //"- 'd' to enter deep sleep\n\n");
          "\n");
  
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0,0);
  M5.Lcd.setTextSize(2);
  M5.Lcd.printf("Done!\n\n");
  M5.Lcd.setTextSize(3);
  M5.Lcd.printf("Have\n"); M5.Lcd.printf("Fun!\n");
  delay(2000);
  M5.Lcd.fillScreen(BLACK);
  M5.Axp.SetLDO2(false); // close tft voltage output
  M5.Axp.SetLDO3(false); // close tft lcd voltage output

} // end Setup


//
// ╻  ┏━┓┏━┓┏━┓
// ┃  ┃ ┃┃ ┃┣━┛
// ┗━╸┗━┛┗━┛╹  
//

void loop() {

  // Check for serial messages. This firmware accepts:
  //     - 's' to start setup mode
  //     - 'r' to reboot
  //     - 'd' to enter deep sleep
  if (Serial.available() > 0) {
    int incomingByte = 0;
    incomingByte = Serial.read();
    switch(incomingByte) {
      case 114: // r
        printf("\nRebooting...\n");
        global.rebootFlag = true;
        break;
      case 115: // s
        printf("\nEntering setup mode\n");
        settings.mode = 1;
        saveJSON();
        global.rebootFlag = true;
        break;
      // case 100: // d
      //   printf("\nEntering deep sleep.\n\nGoodbye!\n");
      //   delay(1000);
      //   esp_deep_sleep_start();
      //   break;
      case 10:
        // ignore LF
        break;
      case 13:
        // ignore CR
        break;
      default:
        printf("\nI don't recognize this command\n"
          "This firmware accepts:\n"
            "'s' to start setup mode\n"
            "'r' to reboot\n"
            //"'d' to enter deep sleep\n\n");
            "\n");
    }
  }

  if (settings.mode == 1) {
    dnsServer.processNextRequest();
  }
  // read battery
    if (millis() - battery.interval > battery.timer) {
      battery.timer = millis();
      readBattery();
      batteryFilter();
    }

  // Read M5StickC sensors
    M5.update();
    M5.Imu.getAccelData(&m5imu.accX, &m5imu.accY, &m5imu.accZ);
    M5.Imu.getGyroData(&m5imu.gyroX,&m5imu.gyroY,&m5imu.gyroZ);
    M5.Imu.getAccelData(&m5imu.accX,&m5imu.accY,&m5imu.accZ);
    M5.Imu.getAhrsData(&m5imu.pitch,&m5imu.roll,&m5imu.yaw);
    M5.Imu.getTempData(&m5imu.temp);

  // Convert gyro from DPS to radians per second if needed
    //m5imu.gyroXrad = m5imu.gyroX * pi / 180;
    //m5imu.gyroYrad = m5imu.gyroY * pi / 180;
    //m5imu.gyroZrad = m5imu.gyroZ * pi / 180;

  // Get High-level descriptors (instrument data) - jab and shake for now
    instrument.updateInstrumentIMU(m5imu.gyroX, m5imu.gyroY, m5imu.gyroZ);

  // send data via OSC
    if (settings.osc) {
      if (settings.mode==0 || WiFi.status() == WL_CONNECTED) { // Send data via OSC ...
          // sending continuous data
            if (millis() - global.oscDelay > global.messageTimer) { 
              global.messageTimer = millis();
              sendContinuousOSC(settings.oscIP[0], settings.oscPORT[0]);
              sendContinuousOSC(settings.oscIP[1], settings.oscPORT[1]);
            }
          // send discrete (button/battery) data (only when it changes) or != 0
            if (m5.BtnA.wasPressed() == 1) {
              sendOSC(settings.oscIP[0], settings.oscPORT[0], "instrument/buttonA", 1);
              sendOSC(settings.oscIP[1], settings.oscPORT[1], "instrument/buttonA", 1);
              //global.lastCount = buttonA.getCount();
            } else if (m5.BtnA.wasReleased() == 1) {
              sendOSC(settings.oscIP[0], settings.oscPORT[0], "instrument/buttonA", 0);
              sendOSC(settings.oscIP[1], settings.oscPORT[1], "instrument/buttonA", 0);
            }
            if (m5.BtnB.wasPressed() == 1) {
              sendOSC(settings.oscIP[0], settings.oscPORT[0], "instrument/buttonB", 1);
              sendOSC(settings.oscIP[1], settings.oscPORT[1], "instrument/buttonB", 1);
              //global.lastCount = buttonA.getCount();
            } else if (m5.BtnB.wasReleased() == 1) {
              sendOSC(settings.oscIP[0], settings.oscPORT[0], "instrument/buttonB", 0);
              sendOSC(settings.oscIP[1], settings.oscPORT[1], "instrument/buttonB", 0);
            }
            if (global.lastJab[0] != instrument.getJabX() || global.lastJab[1] != instrument.getJabY() || global.lastJab[2] != instrument.getJabZ()) {
              sendTrioOSC(settings.oscIP[0], settings.oscPORT[0], "instrument/jabxyz", instrument.getJabX(), instrument.getJabY(), instrument.getJabZ());
              sendTrioOSC(settings.oscIP[1], settings.oscPORT[1], "instrument/jabxyz", instrument.getJabX(), instrument.getJabY(), instrument.getJabZ());
              global.lastJab[0] = instrument.getJabX(); global.lastJab[1] = instrument.getJabY(); global.lastJab[2] = instrument.getJabZ();
            }
            if (global.lastShake[0] != instrument.getShakeX() || global.lastShake[1] != instrument.getShakeY() || global.lastShake[2] != instrument.getShakeZ()) {
              sendTrioOSC(settings.oscIP[0], settings.oscPORT[0], "instrument/shakexyz", instrument.getShakeX(), instrument.getShakeY(), instrument.getShakeZ());
              sendTrioOSC(settings.oscIP[1], settings.oscPORT[1], "instrument/shakexyz", instrument.getShakeX(), instrument.getShakeY(), instrument.getShakeZ());
              global.lastShake[0] = instrument.getShakeX(); global.lastShake[1] = instrument.getShakeY(); global.lastShake[2] = instrument.getShakeZ();
            }
            if (battery.lastPercentage != battery.percentage) {
              sendOSC(settings.oscIP[0], settings.oscPORT[0], "battery", battery.percentage);
              sendOSC(settings.oscIP[1], settings.oscPORT[1], "battery", battery.percentage);
              battery.lastPercentage = battery.percentage;
            }
      }
    }

  // receiving OSC
      //receiveOSC();

  // Check if setup mode has been called
    if (m5.BtnB.pressedFor(4000)) {
      printf("\nLong button B press, entering setup mode\n");
      M5.Axp.SetLDO2(true);
      M5.Axp.SetLDO3(true);
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setCursor(0,0);
      M5.Lcd.setTextSize(2);
      M5.Lcd.printf("Entering\nsetup\nmode...");
      settings.mode = 1;
      saveJSON();
      global.rebootFlag = true;
    }

  // LCD indicator ON/OFF
    if (m5.BtnB.wasPressed()) {
      M5.Axp.SetLDO2(true);
      M5.Axp.SetLDO3(true);
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setCursor(0,0);
      M5.Lcd.setTextSize(1);
      M5.Lcd.printf("%s\n\n", global.deviceName);
      M5.Lcd.setTextSize(3);
      M5.Lcd.printf("Bat: %u%%\n", battery.percentage);
      M5.Lcd.setTextSize(1);
      M5.Lcd.printf("V: %f v\n\n", battery.value);
      if (!settings.mode) {
        M5.Lcd.printf("OSC | IP: ");
        M5.Lcd.println(WiFi.localIP().toString());
      } else {
        M5.Lcd.printf("Setup | IP: ");
        M5.Lcd.println(WiFi.localIP().toString());
        M5.Lcd.println("            192.168.4.1");
      }
    } else if (m5.BtnB.wasReleased()) {
      M5.Lcd.fillScreen(BLACK);
      M5.Axp.SetLDO2(false);
      M5.Axp.SetLDO3(false);
    }

  // Checking for timed reboot (called by setup mode) - reboots after 2 seconds
    if (global.rebootFlag && (millis() - 3000 > global.rebootTimer)) {
      ESP.restart();
    }

} // end loop

//
// ┏━┓╻ ╻┏┓╻┏━╸╺┳╸╻┏━┓┏┓╻┏━┓
// ┣━ ┃ ┃┃┗┫┃   ┃ ┃┃ ┃┃┗┫┗━┓
// ╹  ┗━┛╹ ╹┗━╸ ╹ ╹┗━┛╹ ╹┗━┛
//

void printVariables() {
  Serial.println("Printing loaded values (settings):");
  Serial.print("ID: "); Serial.println(settings.id);
  Serial.print("Designer: "); Serial.println(settings.author);
  Serial.print("Institution: "); Serial.println(settings.institution);
  Serial.print("AP password: "); Serial.println(settings.APpasswd);
  Serial.print("Saved SSID: "); Serial.println(settings.lastConnectedNetwork);
  Serial.print("Saved SSID password: "); Serial.println("********");
  Serial.print("Firmware version: "); Serial.println(settings.firmware);
  Serial.print("OSC IP #1: "); Serial.println(settings.oscIP[0]);
  Serial.print("OSC port #1: "); Serial.println(settings.oscPORT[0]);
  Serial.print("OSC IP #2: "); Serial.println(settings.oscIP[1]);
  Serial.print("OSC port #2: "); Serial.println(settings.oscPORT[1]);
  Serial.print("Local port: "); Serial.println(settings.localPORT);
  Serial.print("Libmapper mode: "); Serial.println(settings.libmapper);
  if (settings.mode == 2) {Serial.println("Setup mode enabled");} 
  else {Serial.print("Data transmission mode: "); Serial.println(settings.mode);}
  Serial.println();
}

void parseJSON() {    
  /* 
  Allocate a temporary JsonDocument
    Don't forget to change the capacity to match your requirements if using DynamicJsonDocument
    Use https://arduinojson.org/v6/assistant/ to compute the capacity.
  */

  StaticJsonDocument<512> doc;

  if (SPIFFS.exists("/config.json")) { // file exists, reading and loading
    
    printf("Reading config file...\n");
    File configFile = SPIFFS.open("/config.json", "r");
    
    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, configFile);

    if (error) {
      printf("Failed to read file!\n\n");
    } else {
      // Copy values from the JsonDocument to the Config
      settings.id = doc["id"];
      strlcpy(settings.author, doc["author"], sizeof(settings.author));
      strlcpy(settings.institution, doc["institution"], sizeof(settings.institution));
      strlcpy(settings.APpasswd, doc["APpasswd"], sizeof(settings.APpasswd));
      strlcpy(settings.lastConnectedNetwork, doc["lastConnectedNetwork"], sizeof(settings.lastConnectedNetwork));
      strlcpy(settings.lastStoredPsk, doc["lastStoredPsk"], sizeof(settings.lastStoredPsk));
      settings.firmware = doc["firmware"];
      strlcpy(settings.oscIP[0], doc["oscIP1"], sizeof(settings.oscIP[0]));
      strlcpy(settings.oscIP[1], doc["oscIP2"], sizeof(settings.oscIP[1]));
      settings.oscPORT[0] = doc["oscPORT1"];
      settings.oscPORT[1] = doc["oscPORT2"];
      settings.localPORT = doc["localPORT"];
      settings.libmapper = doc["libmapper"];
      settings.osc = doc["osc"];
      settings.mode = doc["mode"];
    
      configFile.close();
          
      printf("Module configuration file loaded.\n\n");
    }
  } else {
    printf("Failed to read config file!\n\n");
  }
}

void saveJSON() { // Serializing
  
  printf("Saving config to JSON file...\n");

  /*
  Allocate a temporary JsonDocument
    Don't forget to change the capacity to match your requirements if using DynamicJsonDocument
    Use https://arduinojson.org/v6/assistant/ to compute the capacity.
    
    const size_t capacity = JSON_OBJECT_SIZE(15);
    DynamicJsonDocument doc(capacity);
  */

  StaticJsonDocument<384> doc;
  
  // Copy values from Config to the JsonDocument
    doc["id"] = settings.id;
    doc["author"] = settings.author;
    doc["institution"] = settings.institution;
    doc["APpasswd"] = settings.APpasswd;
    doc["lastConnectedNetwork"] = settings.lastConnectedNetwork;
    doc["lastStoredPsk"] = settings.lastStoredPsk;
    doc["firmware"] = settings.firmware;
    doc["oscIP1"] = settings.oscIP[0];
    doc["oscPORT1"] = settings.oscPORT[0];
    doc["oscIP2"] = settings.oscIP[1];
    doc["oscPORT2"] = settings.oscPORT[1];
    doc["localPORT"] = settings.localPORT;
    doc["libmapper"] = settings.libmapper;
    doc["osc"] = settings.osc;
    doc["mode"] = settings.mode;
 
  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {printf("Failed to open config file for writing!\n\n");}
  
  // Serialize JSON to file
    if (serializeJson(doc, configFile) == 0)
      printf("Failed to write to file\n");
    else
      printf("JSON file successfully saved!\n\n");
  
  configFile.close();
} //end save

/////////////
// Battery //
/////////////

  // read battery level (based on https://www.youtube.com/watch?v=yZjpYmWVLh8&feature=youtu.be&t=88) 
  void readBattery() {
    battery.value = M5.Axp.GetVbatData() * 0.001;
    battery.percentage = static_cast<int>((battery.value - 3.0) * 100 / (3.8 - 3.0));
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


//////////
// Site //
//////////

String indexProcessor(const String& var) {
  if(var == "DEVICENAME")
    return global.deviceName;
  if(var == "STATUS") {
    if (WiFi.status() != WL_CONNECTED) {
      char str[40]; 
      snprintf(str, sizeof(str), "Currently not connected to any network");
      return str;
    } else {
    char str[120];
    snprintf (str, sizeof(str), "Currently connected on <strong style=\"color:Tomato;\">%s</strong> network (IP: %s)", settings.lastConnectedNetwork, WiFi.localIP().toString().c_str());
    return str;
    }
  }
  if(var == "CURRENTSSID")
    return WiFi.SSID();
  if(var == "CURRENTPSK")
    return settings.lastStoredPsk;
  if(var == "MODE0") {
    if(settings.mode == 0) {
      return "selected";
    } else {
      return "";
    }
  }
  if(var == "MODE1") {
    if(settings.mode == 1) {
      return "selected";
    } else {
      return "";
    }
  }
  if(var == "CURRENTIP")
    return WiFi.localIP().toString();
  if(var == "CURRENTAPIP")
    return WiFi.softAPIP().toString();
  if(var == "CURRENTSTAMAC")
      return WiFi.macAddress();
  if(var == "CURRENTAPMAC")
      return WiFi.softAPmacAddress();
  if(var == "CURRENTOSC1")
      return settings.oscIP[0];
  if(var == "CURRENTOSC2")
      return settings.oscIP[1]; 
  if(var == "CURRENTLM") {
    if(settings.libmapper) {
      return "checked";
    } else {
      return "";
    }
  }
  if(var == "CURRENTOSC") {
    if(settings.osc) {
      return "checked";
    } else {
      return "";
    }
  }
  if(var == "CURRENTPORT1") {
    char str[7]; 
    snprintf(str, sizeof(str), "%d", settings.oscPORT[0]);
    return str;
  }
  if(var == "CURRENTPORT2") {
    char str[7]; 
    snprintf(str, sizeof(str), "%d", settings.oscPORT[1]);
    return str;
  }
  if(var == "GUITARAMIID") {
      char str[4];
      snprintf (str, sizeof(str), "%03d", settings.id);
      return str;
  }
  if(var == "GUITARAMIAUTH")
      return settings.author;
  if(var == "GUITARAMIINST")
      return settings.institution;
  if(var == "GUITARAMIVER")
      return String(settings.firmware);
    
  return String();
}

String factoryProcessor(const String& var) {
  if(var == "DEVICENAME")
    return global.deviceName;
  if(var == "STATUS") {
    if (WiFi.status() != WL_CONNECTED) {
      char str[40]; 
      snprintf(str, sizeof(str), "Currently not connected to any network");
      return str;
    } else {
    char str[120];
    snprintf (str, sizeof(str), "Currently connected on <strong style=\"color:Tomato;\">%s</strong> network (IP: %s)", settings.lastConnectedNetwork, WiFi.localIP().toString().c_str());
    return str;
    }
  }
  if(var == "GUITARAMIID") {
      char str[4];
      snprintf (str, sizeof(str), "%03d", settings.id);
      return str;
  }
  if(var == "GUITARAMIVER")
      return String(settings.firmware);

  return String();
}

String scanProcessor(const String& var) {
  if(var == "SSIDS")
    return module.wifiScanResults;
    
  return String();
}

String updateProcessor(const String& var) {
  if(var == "UPDATESTATUSF")
    return module.wifiScanResults;
  if(var == "UPDATESTATUSS")
    return module.wifiScanResults;
    
  return String();
}

void initWebServer() {

  // Route for root page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(SPIFFS, "/index.html", String(), false, indexProcessor);
    });
    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
      printf("\nSettings received! (HTTP_post):\n");
      if(request->hasParam("reboot", true)) {
          request->send(SPIFFS, "/reboot.html");
          settings.mode = 0;
          global.rebootFlag = true;
          global.rebootTimer = millis();
      } else {
        if(request->hasParam("SSID", true)) {
            //Serial.println("SSID received");
            strcpy(settings.lastConnectedNetwork, request->getParam("SSID", true)->value().c_str());
            printf("SSID stored: %s\n", settings.lastConnectedNetwork);
        } else {
            printf("No SSID received\n");
        }
        if(request->hasParam("password", true)) {
            //Serial.println("SSID Password received");
            strcpy(settings.lastStoredPsk, request->getParam("password", true)->value().c_str());
            printf("SSID password stored: ********\n");
        } else {
            printf("No SSID password received\n");
        }
        if(request->hasParam("APpasswd", true) && request->hasParam("APpasswdValidate", true)) {
            if(request->getParam("APpasswd", true)->value() == request->getParam("APpasswdValidate", true)->value() && request->getParam("APpasswd", true)->value() != "") {
              strcpy(settings.APpasswd, request->getParam("APpasswd", true)->value().c_str());
              printf("AP password stored: %s\n", settings.APpasswd);
            } else {
              printf("AP password blank or not match retype. Keeping old password\n");
            }
        } else {
            printf("No AP password received\n");
        }
        if(request->hasParam("mode", true)) {
            settings.mode = atoi(request->getParam("mode", true)->value().c_str());
            printf("Mode stored: %d\n", settings.mode);
        } else {
            printf("No mode received");
        }
        if(request->hasParam("oscIP1", true)) {
            if(request->getParam("oscIP1", true)->value() == ""){
              strcpy(settings.oscIP[0],"0.0.0.0");
            } else {
              strcpy(settings.oscIP[0], request->getParam("oscIP1", true)->value().c_str());
            }
            printf("Primary IP received: %s\n", settings.oscIP[0]);
        } else {
            printf("No Primary IP received\n");
        }
        if(request->hasParam("oscPORT1", true)) {
            settings.oscPORT[0] = atoi(request->getParam("oscPORT1", true)->value().c_str());
            printf("Primary port received: %d\n", settings.oscPORT[0]);
        } else {
            printf("No Primary port received\n");
        }
        if(request->hasParam("oscIP2", true)) {
          if(request->getParam("oscIP2", true)->value() == ""){
              strcpy(settings.oscIP[1],"0.0.0.0");
            } else {
              strcpy(settings.oscIP[1], request->getParam("oscIP2", true)->value().c_str());
            }
        } else {
            printf("No Secondary IP received\n");
        }
        if(request->hasParam("oscPORT2", true)) {
            settings.oscPORT[1] = atoi(request->getParam("oscPORT2", true)->value().c_str());
            printf("Secondary port received: %d\n", settings.oscPORT[0]);
        } else {
            printf("No Secondary port received\n");
        }
        if(request->hasParam("libmapper", true)) {
            printf("Libmapper TRUE received\n");
            settings.libmapper = true;
        } else {
            printf("Libmapper FALSE received\n");
            settings.libmapper = false;
        }
        if(request->hasParam("osc", true)) {
            printf("OSC TRUE received\n");
            settings.osc = true;
        } else {
            printf("OSC FALSE received\n");
            settings.osc = false;
        }
        request->send(SPIFFS, "/index.html", String(), false, indexProcessor);
        //request->send(200);
      }
      saveJSON();
    });

  // Route for scan page
    server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(SPIFFS, "/scan.html", String(), false, scanProcessor);
    });

  // Route for factory page
    server.on("/factory", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(SPIFFS, "/factory.html", String(), false, factoryProcessor);
    });
    server.on("/factory", HTTP_POST, [](AsyncWebServerRequest *request) {
        printf("\nFactory Settings received! (HTTP_post):\n");
        if(request->hasParam("reboot", true)) {
            request->send(SPIFFS, "/reboot.html");
            settings.mode = 0;
            global.rebootFlag = true;
            global.rebootTimer = millis();
        } else {
          if(request->hasParam("ID", true)) {
              settings.id = atoi(request->getParam("ID", true)->value().c_str());
              printf("ID (factory) received: %d\n", settings.id);
          } else {
              printf("No ID (factory) received\n");
          }
          if(request->hasParam("firmware", true)) {
              settings.firmware = atoi(request->getParam("firmware", true)->value().c_str());
              printf("Firmware # (factory) received: %d\n", settings.firmware);
          } else {
              printf("No Firmware # (factory) received\n");
          }
          request->send(SPIFFS, "/factory.html", String(), false, factoryProcessor);
          //request->send(200);
        }
        saveJSON();
      });

  // Route for update page
    server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(SPIFFS, "/update.html");
    });
    /*handling uploading firmware file */
    server.on("/updateF", HTTP_POST, [](AsyncWebServerRequest *request){
      global.rebootFlag = !Update.hasError();
      AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", global.rebootFlag?"Update complete, rebooting":"Fail to send file and/or update");
      response->addHeader("Connection", "close");
      request->send(response);
    },[](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
      if(!index){
        Serial.printf("Firmware Update Start: %s\n", filename.c_str());
        //Update.runAsync(true);
        if(!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)){
          Update.printError(Serial);
        }
      }
      if(!Update.hasError()){
        if(Update.write(data, len) != len){
          Update.printError(Serial);
        }
      }
      if(final){
        if(Update.end(true)){
          Serial.printf("Firmware Update Success: %uB\n", index+len);
        } else {
          Update.printError(Serial);
        }
      }
    });     
    /*handling uploading SPIFFS file */
    server.on("/updateS", HTTP_POST, [](AsyncWebServerRequest *request){
      global.rebootFlag = !Update.hasError();
      AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", global.rebootFlag?"Update complete, rebooting":"Fail to send file and/or update");
      response->addHeader("Connection", "close");
      request->send(response);
    },[](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
      if(!index){
        Serial.printf("SPIFFS Update Start: %s\n", filename.c_str());
        //Update.runAsync(true);
        if(!Update.begin(UPDATE_SIZE_UNKNOWN, U_SPIFFS)){
          Update.printError(Serial);
        }
      }
      if(!Update.hasError()){
        if(Update.write(data, len) != len){
          Update.printError(Serial);
        }
      }
      if(final){
        if(Update.end(true)){
          Serial.printf("SPIFFS Update Success: %uB\n", index+len);
        } else {
          Update.printError(Serial);
        }
      }
    });  

  // Route to load style.css file
    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(SPIFFS, "/style.css", "text/css");
    });

  server.begin();
  
  Serial.println("HTTP server started");
}

void start_mdns_service() {
    //initialize mDNS service
    esp_err_t err = mdns_init();
    if (err) {
        printf("MDNS Init failed: %d\n", err);
        return;
    }

    //set hostname
    mdns_hostname_set(global.deviceName);
    //set default instance
    mdns_instance_name_set(global.deviceName);
}

/////////
// OSC //
/////////

 void sendContinuousOSC(char* ip,int32_t port) {

  IPAddress oscIP;
  IPAddress emptyIP(0,0,0,0);
  
  if (oscIP.fromString(ip) && oscIP != emptyIP) {
    char namespaceBuffer[40];

    static OSCBundle continuous;

        snprintf(namespaceBuffer,(sizeof(namespaceBuffer)-1),"/%s/raw/accl",global.deviceName);
        OSCMessage msgAccl(namespaceBuffer);
          msgAccl.add(m5imu.accX);
          msgAccl.add(m5imu.accY);
          msgAccl.add(m5imu.accZ);
          continuous.add(msgAccl);
          msgAccl.empty();
      
        snprintf(namespaceBuffer,(sizeof(namespaceBuffer)-1),"/%s/raw/gyro",global.deviceName);
        OSCMessage msgGyro(namespaceBuffer);
          msgGyro.add(m5imu.gyroX);
          msgGyro.add(m5imu.gyroY);
          msgGyro.add(m5imu.gyroZ);
          continuous.add(msgGyro);
          msgGyro.empty();

        snprintf(namespaceBuffer,(sizeof(namespaceBuffer)-1),"/%s/instrument/ypr",global.deviceName);
        OSCMessage msgEuler(namespaceBuffer);
          msgEuler.add(m5imu.yaw);
          msgEuler.add(m5imu.pitch);
          msgEuler.add(m5imu.roll);
          continuous.add(msgEuler);
          msgEuler.empty(); 

        oscEndpoint.beginPacket(oscIP,port);
        continuous.send(oscEndpoint);
        oscEndpoint.endPacket();
        continuous.empty(); 
  }
}

void sendOSC(char* ip,int32_t port, const char* messageNamespace, float data) {
    IPAddress oscIP;
    IPAddress emptyIP(0,0,0,0);
    if (oscIP.fromString(ip) && oscIP != emptyIP) {
      char namespaceBuffer[40];
      snprintf(namespaceBuffer,(sizeof(namespaceBuffer)-1),"/%s/%s",global.deviceName, messageNamespace);
      OSCMessage msg(namespaceBuffer);
      msg.add(data);
      oscEndpoint.beginPacket(oscIP, port);
      msg.send(oscEndpoint);
      oscEndpoint.endPacket();
      msg.empty();
  }
}

void sendTrioOSC(char* ip,int32_t port, const char* messageNamespace, float data1, float data2, float data3) {
    IPAddress oscIP;
    IPAddress emptyIP(0,0,0,0);
    if (oscIP.fromString(ip) && oscIP != emptyIP) {
      char namespaceBuffer[40];
      snprintf(namespaceBuffer,(sizeof(namespaceBuffer)-1),"/%s/%s",global.deviceName, messageNamespace);
      OSCMessage msg(namespaceBuffer);
      msg.add(data1);
      msg.add(data2);
      msg.add(data3);
      oscEndpoint.beginPacket(oscIP, port);
      msg.send(oscEndpoint);
      oscEndpoint.endPacket();
      msg.empty();
  }
}

void sendInfo(OSCMessage &msg) {
  // Send back instrument's current config
  char namespaceBuffer[30];
  snprintf(namespaceBuffer,(sizeof(namespaceBuffer)-1),"%s/info",global.deviceName);
  IPAddress oscIP;  
  if (oscIP.fromString(settings.oscIP[0])) {
    OSCMessage msgInfo(namespaceBuffer);
    msgInfo.add(settings.id);
    msgInfo.add(settings.firmware);
    oscEndpoint.beginPacket(oscIP, settings.oscPORT[0]);
    msgInfo.send(oscEndpoint);
    oscEndpoint.endPacket();
    msgInfo.empty();
  }
  if (oscIP.fromString(settings.oscIP[1])) {
    OSCMessage msgInfo(namespaceBuffer);
    msgInfo.add(settings.id);
    msgInfo.add(settings.firmware);
    oscEndpoint.beginPacket(oscIP, settings.oscPORT[1]);
    msgInfo.send(oscEndpoint);
    oscEndpoint.endPacket();
    msgInfo.empty();
  }
}

void receiveOSC() {

  OSCErrorCode error;
  OSCMessage msgReceive;
  int size = oscEndpoint.parsePacket();

  if (size > 0) {
    Serial.println("\nOSC message received");
    while (size--) {
      msgReceive.fill(oscEndpoint.read());
    }
    if (!msgReceive.hasError()) {
      Serial.println("Routing OSC message...");
      msgReceive.dispatch("/state/info", sendInfo); // send back instrument's current config
      //msgReceive.dispatch("/state/setup", openPortalOSC); // open portal
    } else {
      error = msgReceive.getError();
      Serial.print("\nOSC receive error: "); Serial.println(error);
    }
  }
}