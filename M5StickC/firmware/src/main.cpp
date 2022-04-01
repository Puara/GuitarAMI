
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

const unsigned int firmware_version = 220322;

// Turn everything related to MIDI off
//#define DISABLE_MIDI


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
    unsigned int midiDelay = 10;
    unsigned int lcdDelay = 3000;
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
    int mode = 0; // 0:STA, 1:Setup(STA+AP+WebServer), 2:MIDI
  } settings;


////////////////////////////////////////
// Variable to hold M5StickC IMU info //
////////////////////////////////////////

  struct M5imu {
    float accX = 0.0F; // between -1 and 1
    float accY = 0.0F;
    float accZ = 0.0F;
    float gyroX = 0.0F; // DPS ? - between -2000 and 1984.31 (2000?)
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

  //#include "midi.h"
  #include <BLEMidi.h>

  //Midi midi;

  struct MidiReady {
    unsigned int AccelX;  // CC 21  (0b00010101) (for X-axis accelerometer)
    unsigned int AccelY;  // CC 22  (0b00010110) (for Y-axis accelerometer)
    unsigned int AccelZ;  // CC 23  (0b00010111) (for Z-axis accelerometer)
    unsigned int GyroX;   // CC 24  (0b00011000) (for X-axis gyroscope)
    unsigned int GyroY;   // CC 25  (0b00011001) (for Y-axis gyroscope)
    unsigned int GyroZ;   // CC 26  (0b00011010) (for Z-axis gyroscope)
    unsigned int Yaw;     // CC 27  (0b00011011) (for yaw)
    unsigned int Pitch;   // CC 28  (0b00011100) (for pitch)
    unsigned int Roll;    // CC 29  (0b00011101) (for roll)
    unsigned int ShakeX;  // CC 85  (0b01010101) (for X-axis ShakeX)
    unsigned int ShakeY;  // CC 86  (0b01010110) (for Y-axis ShakeY)
    unsigned int ShakeZ;  // CC 87  (0b01010111) (for Z-axis ShakeZ)
    unsigned int JabX;    // CC 102 (0b01100110) (for X-axis JabX) 
    unsigned int JabY;    // CC 103 (0b01100111) (for Y-axis JabY) 
    unsigned int JabZ;    // CC 104 (0b01101000) (for Z-axis JabZ)
    unsigned int ButtonA; // CC 30  (0b00011110) (for button A)
    unsigned int ButtonB; // CC 31  (0b00011111) (for button B)
    unsigned int battery; // CC 105  (0b00011111) (for battery)
  } midiReady;

  #endif

///////////////////
// Battery stuff //
///////////////////
  
  struct BatteryData {
    int percentage = 0;
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
  int mapMIDI(float x, float in_min, float in_max);
  int mapMIDI(float x, float in_min, float in_max, float out_min, float out_max);

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
    if (settings.mode < 0 || settings.mode > 2){
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
        "Booting System...\n\n",settings.id,settings.firmware);

  // Print Json file stored values
    //module.printJSON();
    printVariables();

  // Define this module full name
    snprintf(global.deviceName,(sizeof(global.deviceName)-1),"GuitarAMI_m5_%03i",settings.id);

  // Start Wi-Fi stuff
    if (settings.mode == 0 || settings.mode == 1) {
      module.startWifi(global.deviceName, settings.mode, settings.APpasswd, settings.lastConnectedNetwork, settings.lastStoredPsk);
      // Start listening for incoming OSC messages if WiFi is ON
      oscEndpoint.begin(settings.localPORT);
      printf("Starting UDP (listening to OSC messages)\n");
      printf("Local port: %d\n", settings.localPORT);
    }

  // Start dns and web Server if in setup mode
    if (settings.mode == 1) {
      printf("Starting DNS server\n");
      dnsServer.start(53, "*", WiFi.softAPIP());
      start_mdns_service();
      initWebServer();
    }

  // Initializing MIDI if in MIDI mode (settings.mode = 2)

    #ifndef DISABLE_MIDI
      if (settings.mode == 2) {
        Serial.println("    Initializing BLE MIDI...  \n");
        //midi.setDeviceName(global.deviceName);
        BLEMidiServer.begin(global.deviceName);
        // if (midi.initMIDI()) {
        //   Serial.print("MIDI mode: Connect to "); Serial.print(global.deviceName); Serial.println(" to start sending BLE MIDI");
        //   Serial.print("    (channel "); Serial.print(midi.getChannel()); Serial.println(")");
        // } else {
        //   Serial.println("BLE MIDI initialization failed!");
        // }
      }
    #endif

  // Setting Deep sleep wake button
  //     esp_sleep_enable_ext0_wakeup(GPIO_NUM_15,0); // 1 = High, 0 = Low
    
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

  // prepare MIDI data if needed
    #ifndef DISABLE_MIDI
      if (settings.mode == 2) {
        midiReady.AccelX = mapMIDI(m5imu.accX, -1, 1);
        midiReady.AccelZ = mapMIDI(m5imu.accY, -1, 1);
        midiReady.AccelY = mapMIDI(m5imu.accZ, -1, 1);
        midiReady.GyroX = mapMIDI(m5imu.gyroX, -2000, 2000);
        midiReady.GyroY = mapMIDI(m5imu.gyroY, -2000, 2000);
        midiReady.GyroZ = mapMIDI(m5imu.gyroZ, -2000, 2000);
        midiReady.Yaw  = mapMIDI(m5imu.yaw, -180, 180);
        midiReady.Pitch = mapMIDI(m5imu.pitch, -180, 180);
        midiReady.Roll  = mapMIDI(m5imu.roll, -180, 180);
        midiReady.ShakeX = mapMIDI(instrument.getShakeX(), 0, 50);
        midiReady.ShakeY = mapMIDI(instrument.getShakeY(), 0, 50);
        midiReady.ShakeZ = mapMIDI(instrument.getShakeZ(), 0, 50);
        midiReady.JabX = mapMIDI(instrument.getJabX(), 0, 3000);
        midiReady.JabY = mapMIDI(instrument.getJabY(), 0, 3000);
        midiReady.JabZ = mapMIDI(instrument.getJabZ(), 0, 3000);
        if (m5.BtnA.wasPressed() == 1) {
              midiReady.ButtonA = 127;
        } else if (m5.BtnA.wasReleased() == 1) {
              midiReady.ButtonA = 0;
        }
        if (m5.BtnB.wasPressed() == 1) {
          midiReady.ButtonB = 1;
        } else if (m5.BtnB.wasReleased() == 1) {
          midiReady.ButtonB = 0;
        }
        midiReady.battery = battery.percentage;
      }
    #endif

  // send data via OSC
    if (settings.osc && WiFi.status() == WL_CONNECTED) {
      if (settings.mode==0 || settings.mode==1) { // Send data via OSC ...
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
              sendOSC(settings.oscIP[0], settings.oscPORT[0], "vbattery", battery.value);
              sendOSC(settings.oscIP[1], settings.oscPORT[1], "vbattery", battery.value);
            }
      }
    }

  // receiving OSC
      //receiveOSC();

  #ifndef DISABLE_MIDI
    // if (midi.status()) {
    //   // sending continuous data
    //   if (millis() - global.midiDelay > global.messageTimer) {
    //     global.messageTimer = millis();
    //       midi.CCbundle ( 21, midiReady.AccelX,
    //                       22, midiReady.AccelY,
    //                       23, midiReady.AccelZ);
    //       midi.CCbundle ( 24, midiReady.GyroX, 
    //                       25, midiReady.GyroY, 
    //                       26, midiReady.GyroZ);
    //       midi.CCbundle ( 27, midiReady.Yaw,  
    //                       28, midiReady.Pitch,
    //                       29, midiReady.Roll);
    //   }
    //   // send discrete data (only when it changes)
    //   if (m5.BtnA.wasPressed() == 1 || m5.BtnA.wasReleased()) {
    //     midi.CC(30, midiReady.ButtonA);
    //   }
    //   if (m5.BtnB.wasPressed() == 1 || m5.BtnB.wasReleased() == 1) {
    //     midi.CC(31, midiReady.ButtonB);
    //   }
    //   if (global.lastJab[0] != instrument.getJabX() || global.lastJab[1] != instrument.getJabY() || global.lastJab[2] != instrument.getJabZ()) {
    //     midi.CCbundle ( 102, midiReady.JabX,
    //                     103, midiReady.JabY,
    //                     104, midiReady.JabZ);
    //   }
    //   if (global.lastShake[0] != instrument.getShakeX() || global.lastShake[1] != instrument.getShakeY() || global.lastShake[2] != instrument.getShakeZ()) {
    //     midi.CCbundle ( 85, midiReady.ShakeX,
    //                     86, midiReady.ShakeY,
    //                     87, midiReady.ShakeZ);
    //   }
    //   if (battery.lastPercentage != battery.percentage) {
    //     midi.CC(105, midiReady.battery);
    //   }
    // }
    if (BLEMidiServer.isConnected()) {
      // sending continuous data
      if (millis() - global.midiDelay > global.messageTimer) {
        global.messageTimer = millis();
          BLEMidiServer.controlChange(0, 21, midiReady.AccelX);
          BLEMidiServer.controlChange(0, 22, midiReady.AccelY);
          BLEMidiServer.controlChange(0, 23, midiReady.AccelZ);
          BLEMidiServer.controlChange(0, 24, midiReady.GyroX);
          BLEMidiServer.controlChange(0, 25, midiReady.GyroY);
          BLEMidiServer.controlChange(0, 26, midiReady.GyroZ);
          BLEMidiServer.controlChange(0, 27, midiReady.Yaw);
          BLEMidiServer.controlChange(0, 28, midiReady.Pitch);
          BLEMidiServer.controlChange(0, 29, midiReady.Roll);
      }
      // send discrete data (only when it changes)
      if (m5.BtnA.wasPressed() == 1 || m5.BtnA.wasReleased()) {
        BLEMidiServer.controlChange(0, 30, midiReady.ButtonA);
      }
      if (m5.BtnB.wasPressed() == 1 || m5.BtnB.wasReleased() == 1) {
        BLEMidiServer.controlChange(0, 31, midiReady.ButtonB);
      }
      if (global.lastJab[0] != instrument.getJabX() || global.lastJab[1] != instrument.getJabY() || global.lastJab[2] != instrument.getJabZ()) {
        BLEMidiServer.controlChange(0, 102, midiReady.JabX);
        BLEMidiServer.controlChange(0, 103, midiReady.JabY);
        BLEMidiServer.controlChange(0, 104, midiReady.JabZ);
      }
      if (global.lastShake[0] != instrument.getShakeX() || global.lastShake[1] != instrument.getShakeY() || global.lastShake[2] != instrument.getShakeZ()) {
        BLEMidiServer.controlChange(0, 85, midiReady.ShakeX);
        BLEMidiServer.controlChange(0, 86, midiReady.ShakeY);
        BLEMidiServer.controlChange(0, 87, midiReady.ShakeZ);
      }
      if (battery.lastPercentage != battery.percentage) {
        BLEMidiServer.controlChange(0, 105, midiReady.battery);
      }
    }
    #endif

  // Show LCD instructions
    if (m5.BtnB.wasPressed()) {
      M5.Axp.SetLDO2(true);
      M5.Axp.SetLDO3(true);
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setCursor(0,0);
      M5.Lcd.setTextSize(1);
      M5.Lcd.printf("- Release for info\n\n"
                    "- Hold for setup mode\n\n"
                    "- Hold Power button to\n"
                    "  turn off");
    }

  // Check if setup mode has been called
    if (m5.BtnB.pressedFor(8000)) {
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
    if (m5.BtnB.wasReleased()) {
      M5.Axp.SetLDO2(true);
      M5.Axp.SetLDO3(true);
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setCursor(0,0);
      M5.Lcd.setTextSize(1);
      M5.Lcd.printf("%s\n\n", global.deviceName);
      M5.Lcd.setTextSize(3);
      M5.Lcd.printf("Bat:%u%%\n", battery.percentage);
      M5.Lcd.setTextSize(1);
      M5.Lcd.printf("V: %f v\n\n", battery.value);

      switch(settings.mode) {
        case 0: 
          M5.Lcd.printf("OSC | IP: ");
          M5.Lcd.println(WiFi.localIP().toString());
          break;
        case 1: 
          M5.Lcd.printf("Setup | IP: ");
          M5.Lcd.println(WiFi.localIP().toString());
          M5.Lcd.println("            192.168.4.1");
          break;
        case 2:
          M5.Lcd.printf("MIDI mode");
          break;
        default:
          printf("\nError: wrong mode\n");
          M5.Lcd.setTextSize(3);
          M5.Lcd.printf("mode error");
      }
    }
    
    // turn LCD off after inactivity
      if (m5.BtnB.releasedFor(global.lcdDelay)) {
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

// Functions to change range for sending MIDI messages
  int mapMIDI(float x, float in_min, float in_max, float out_min, float out_max) {
    float result = (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    if (result > out_max) {
      result = out_max;
    };
    if (result < out_min) {
      result = out_min;
    };
    return static_cast<int>(result);
  };
  int mapMIDI(float x, float in_min, float in_max) {
    int result = (x - in_min) * 127 / (in_max - in_min);
    if (result > 127) {
      result = 127;
    };
    if (result < 0) {
      result = 0;
    };
    return static_cast<int>(result);
  };

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
  if (settings.mode == 1) {Serial.println("Setup mode enabled");} 
  else {Serial.print("Data transmission mode: ");}
  if (settings.mode == 0) {Serial.println("STA mode");}
  else if (settings.mode == 2) {Serial.println("MIDI mode");}
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
    battery.lastPercentage = battery.percentage;
    battery.value = M5.Axp.GetVbatData() * 0.001;
    battery.percentage = static_cast<int>((battery.value - 2.8) * 100 / (3.8 - 2.8));
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
  if(var == "MODE2") {
    if(settings.mode == 2) {
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