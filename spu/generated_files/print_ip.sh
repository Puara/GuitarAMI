#!/bin/sh

# get IP variables:

ipl=$(/sbin/ip -o -4 addr list eth0 | awk '{print $4}' | cut -d/ -f1)
ipw=$(/sbin/ip -o -4 addr list wlan0 | awk '{print $4}' | cut -d/ -f1 | head -n 1)

iplp="eth0:$ipl"
ipwp="WiFi:$ipw"

sendosc 127.0.0.1 20000 /lcd s "                   " i 4 i 1
sendosc 127.0.0.1 20000 /lcd s "$iplp" i 4 i 1
sleep 1
sendosc 127.0.0.1 20000 /lcd s "                   " i 4 i 1
sendosc 127.0.0.1 20000 /lcd s "$iplp" i 4 i 1
sleep 1
sendosc 127.0.0.1 20000 /lcd s "                   " i 4 i 1
sendosc 127.0.0.1 20000 /lcd s "$iplp" i 4 i 1
sleep 1
sendosc 127.0.0.1 20000 /lcd s "                   " i 4 i 1
sendosc 127.0.0.1 20000 /lcd s "$iplp" i 4 i 1
sleep 1
sendosc 127.0.0.1 20000 /lcd s "                   " i 4 i 1
sendosc 127.0.0.1 20000 /lcd s "$iplp" i 4 i 1
sleep 1
sendosc 127.0.0.1 20000 /lcd s "                   " i 4 i 1
sendosc 127.0.0.1 20000 /lcd s "$ipwp" i 4 i 1
sleep 1
sendosc 127.0.0.1 20000 /lcd s "                   " i 4 i 1
sendosc 127.0.0.1 20000 /lcd s "$ipwp" i 4 i 1
sleep 1
sendosc 127.0.0.1 20000 /lcd s "                   " i 4 i 1
sendosc 127.0.0.1 20000 /lcd s "$ipwp" i 4 i 1
sleep 1
sendosc 127.0.0.1 20000 /lcd s "                   " i 4 i 1
sendosc 127.0.0.1 20000 /lcd s "$ipwp" i 4 i 1
sleep 1
sendosc 127.0.0.1 20000 /lcd s "                   " i 4 i 1
sendosc 127.0.0.1 20000 /lcd s "$ipwp" i 4 i 1

sleep 5