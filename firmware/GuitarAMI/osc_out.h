#pragma once

#include <IPAddress.h>
#include <MicroOscUdp.h>
#include <WiFiUdp.h>

#include <string>

// Sends every message to up to two destinations under the "/<device name>/" namespace.
class OscOut {
public:
  explicit OscOut(WiFiUDP& udp);

  void setNamespace(const std::string& deviceName);
  void setDestination(int index, const std::string& ip, uint16_t port);
  void receive(MicroOsc::MicroOscCallback callback);

  void send(const char* name, int value);
  void send(const char* name, unsigned int value);
  void send(const char* name, double value);
  void send(const char* name, double x, double y, double z);
  void send(const char* name, double x, double y, double z, double w);
  void send(const char* name, const std::string& text, int value);

private:
  static constexpr int destinationCount = 2;

  struct Destination {
    IPAddress ip;
    uint16_t port = 0;
    bool valid = false;
  };

  const char* address(const char* name);
  template <typename Send>
  void toEachDestination(Send send);

  MicroOscUdp<1024> osc;
  Destination destinations[destinationCount];
  std::string prefix;
  char addressBuffer[64];
};
