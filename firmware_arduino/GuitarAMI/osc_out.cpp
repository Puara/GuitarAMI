#include "osc_out.h"

OscOut::OscOut(WiFiUDP& udp) : osc(udp) {}

void OscOut::setNamespace(const std::string& deviceName) {
  prefix = "/" + deviceName + "/";
}

void OscOut::setDestination(int index, const std::string& ip, uint16_t port) {
  Destination& destination = destinations[index];
  destination.port = port;
  destination.valid = destination.ip.fromString(ip.c_str()) && destination.ip != IPAddress(0, 0, 0, 0) && port != 0;
}

void OscOut::receive(MicroOsc::MicroOscCallback callback) {
  osc.onOscMessageReceived(callback);
}

const char* OscOut::address(const char* name) {
  snprintf(addressBuffer, sizeof(addressBuffer), "%s%s", prefix.c_str(), name);
  return addressBuffer;
}

template <typename Send>
void OscOut::toEachDestination(Send send) {
  for (const Destination& destination : destinations) {
    if (!destination.valid) continue;
    osc.setDestination(destination.ip, destination.port);
    send();
  }
}

void OscOut::send(const char* name, int value) {
  const char* path = address(name);
  toEachDestination([&] { osc.sendInt(path, value); });
}

void OscOut::send(const char* name, unsigned int value) {
  send(name, static_cast<int>(value));
}

void OscOut::send(const char* name, double value) {
  const char* path = address(name);
  toEachDestination([&] { osc.sendFloat(path, value); });
}

void OscOut::send(const char* name, double x, double y, double z) {
  const char* path = address(name);
  toEachDestination([&] { osc.sendMessage(path, "fff", x, y, z); });
}

void OscOut::send(const char* name, double x, double y, double z, double w) {
  const char* path = address(name);
  toEachDestination([&] { osc.sendMessage(path, "ffff", x, y, z, w); });
}

void OscOut::send(const char* name, const std::string& text, int value) {
  const char* path = address(name);
  toEachDestination([&] { osc.sendMessage(path, "si", text.c_str(), value); });
}
