# Builing Instructions - GuitarAMI Sound Processing Unit

Using Rpi 4B + Pisound

## Prepare PatchboxOS

- Download the [PatchboxOS](https://blokas.io/patchbox-os/#patchbox-os-download)
- Flash the image using [ApplePiBaker](https://www.tweaking4all.com/hardware/raspberry-pi/applepi-baker-v2/), [balenaEtcher](https://www.balena.io/etcher/), or diskutil (dd)
- Boot the Rpi and folow the [first run tutorial](https://blokas.io/patchbox-os/docs/first-run-options/)
  - ssh to th SPU: `ssh patch@ip_address` (The default user name is *patch* and its password is *blokaslabs*)
    - update PatchBox
    - set new password: `mappings`
    - choose default soundcard: pisound, 48000, 128, 2
    - select boot: *console autologin*
    - connect to network: *no*
    - choose module: *modep*

## Finish PatchboxOS config

- Enter PiSound configuration: `sudo pisound-config`
  - Update Pisound
  - Change Pisound HotSpot settings:
    - ssid: `SPU00X` (use SPU's ID)
    - wpa_passphrase: `mappings` (default SPU password)
  - Install Additional software: *SuperCollider* and *Pure Data*

  - If not yet, run `patchbox` to *MODEP* module (usage instructions [here](https://blokas.io/modep/docs/running-modep/))

- Enter Raspi-Config: `sudo raspi-config`
  - Update:
    - `Update this tool to the latest version`
  - System options:
    - Hostname: `SPU00X` (use SPU's ID)

Create a place for the patches and docs: `mkdir ~/Patches ~/Documents ~/sources`

Reboot

- To access MODEP, use [http://spu00X.local/](http://spu00X.local/) (replace SPU number) or the SPU's IP address
- Button scripts: `/usr/local/pisound/scripts/pisound-btn`. More info at [https://blokas.io/pisound/docs/the-button/](https://blokas.io/pisound/docs/the-button/)
- The Button settings are stored at `/usr/local/patchbox-modules/modep/pisound-btn.conf`
- Modep API: `/usr/modep/scripts/modep-ctrl.py`

## Compile and install SuperCollider

```bash
sudo apt install -y libsamplerate0-dev libsndfile1-dev libasound2-dev libavahi-client-dev libreadline-dev libfftw3-dev libudev-dev libncurses5-dev cmake git
cd ~/sources
git clone --recurse-submodules https://github.com/supercollider/supercollider.git
cd supercollider
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DSUPERNOVA=OFF -DSC_ED=OFF -DSC_EL=OFF -DSC_VIM=ON -DNATIVE=ON -DSC_IDE=OFF -DNO_X11=ON -DSC_QT=OFF ..
sudo cmake --build . --config Release --target install
sudo ldconfig
```

## Compile and install sc3-plugins

```bash
cd ~/sources
git clone --recursive https://github.com/supercollider/sc3-plugins.git
cd sc3-plugins
mkdir build && cd build
cmake -DSC_PATH=../../supercollider -DCMAKE_BUILD_TYPE=Release -DSUPERNOVA=ON ..
cmake --build . --config Release
sudo cmake --build . --config Release --target install
```

## Copy patches

Open another terminal and copy the folder containig the SC files needed using scp:

```bash
scp <PATH_CONTAINING_FILES>/* patch@<SPU_IP>:~/Patches/
scp <PATH_CONTAINING_FILES>/* patch@<SPU_IP>:~/Documents/
```

## Install Samba server for uploading files

OBS: Choose *No* when asked **Modify smb.conf to use WINS settings from DHCP?**

```bash
sudo apt install -y samba
sudo systemctl stop smbd.service
sudo mv /etc/samba/smb.conf /etc/samba/smb.conf.orig
sudo smbpasswd -a patch
sudo smbpasswd -e patch
```

Create Samba configuration file. **Don't forget to replace the SPU number!**

```bash
cat <<- "EOF" | sudo tee /etc/samba/smb.conf
[global]
        server string = SPU00X
        server role = standalone server
        interfaces = lo eth0 wlan0
        bind interfaces only = yes
        smb ports = 445
        log file = /var/log/samba/smb.log
        max log size = 10000
        map to guest = bad user

[SPU00X-Docs]
        path = /home/patch/Documents
        read only = no
        valid users = patch

[SPU00X-Patches]
        path = /home/patch/Patches
        read only = no
        valid users = patch
EOF
```

Start Samba service again: `sudo systemctl start smbd.service`

## Set SC to start on boot

Obs: this service is started as a user service

```bash
cat <<- "EOF" | tee ~/.config/systemd/user/supercollider.service
[Unit]
Description=Run default SC code
ConditionPathExists=/home/patch/Patches/default.scd
After=multi-user.target jack.service

[Service]
Type=idle
ExecStart=sclang -D /home/patch/Patches/default.scd

[Install]
WantedBy=default.target
EOF
```

```bash
sudo chmod 644 ~/.config/systemd/user/supercollider.service
systemctl --user enable supercollider.service
```

## Set PD to start on boot

Obs: this service is started as a user service

```bash
cat <<- "EOF" | tee ~/.config/systemd/user/puredata.service
[Unit]
Description=Run default PD code
ConditionPathExists=/home/patch/Patches/default.pd
After=multi-user.target jack.service

[Service]
Type=idle
ExecStart=pd -rt -jack ~/Documents/default.pd

[Install]
WantedBy=default.target
EOF
```

```bash
sudo chmod 644 ~/.config/systemd/user/puredata.service
systemctl --user enable puredata.service
```

## Set LCD

```bash
mkdir ~/sources/lcd
sudo apt install python3-rpi.gpio python3-pip -y
sudo pip3 install osc4py3
```

```bash
cat <<- "EOF" | tee ~/sources/lcd/lcd.py

# The wiring for the LCD is as follows (SPU with PiSound):
#
# LCD pin    Rpi4 BOARD pin  function
# 1          GND             GND
# 2          5V              5V
# 3          5V, GND         Contrast (0-5V), use a potentiometer here: middle leg to LCD pin 3, sides to Rpi pin 2 or 4 (5V) and 14(GND)
# 4          26 (GPIO 7)     RS (Register Select)
# 5          GND             R/W (Read Write) - GROUND THIS PIN
# 6           3 (GPIO 2)     Enable or Strobe
# 7          -               Data Bit 0 - NOT USED
# 8          -               Data Bit 1 - NOT USED
# 9          -               Data Bit 2 - NOT USED
# 10         -               Data Bit 3 - NOT USED
# 11          5 (GPIO  0)    Data Bit 4
# 12          8 (GPIO 14)    Data Bit 5
# 13         16 (GPIO 25)    Data Bit 6
# 14          7 (GPIO  4)    Data Bit 7
# 15         3V3             LCD Backlight 3V3
# 16         GND (optional)  LCD Backlight R GND
# 17         GND (optional)  LCD Backlight G GND
# 18         GND (optional)  LCD Backlight B GND
#
# OBS: on Rpi4:
#    3V3 = pin 1 = pin 17
#    5V  = pin 2 = pin 4
#    GND = pin 6 = pin 9 = pin 14 = pin 20 = pin 25 = pin 30 = pin 34 = pin 39


#import
import RPi.GPIO as GPIO
import time
from time import sleep
from datetime import datetime
from time import strftime
import socket
import os

# Import needed modules from osc4py3
from osc4py3.as_eventloop import *
from osc4py3 import oscmethod as osm

import types
import sys

# Define GPIO to LCD mapping
LCD_RS = 26
LCD_E  = 3
LCD_D4 = 5
LCD_D5 = 8
LCD_D6 = 16
LCD_D7 = 7
# LCD_VEE = 15 # Contrast

# Define some device constants
LCD_WIDTH = 20 # Maximum characters per line
LCD_CHR = True
LCD_CMD = False

LCD_LINE_1 = 0x80 # LCD RAM address for the 1st line
LCD_LINE_2 = 0xC0 # LCD RAM address for the 2nd line
LCD_LINE_3 = 0x94 # LCD RAM address for the 3rd line
LCD_LINE_4 = 0xD4 # LCD RAM address for the 4th line

# To navigate through columns: LCD_LINE + steps (0 to 15)
LCD_POS = [0, LCD_LINE_1, LCD_LINE_2, LCD_LINE_3, LCD_LINE_4]
LCD_TAM = 20

# Timing constants
E_PULSE = 0.0005
E_DELAY = 0.0005

# Start the OSC "server"
osc_startup()

first_message = False

# Make server channels to receive packets
osc_udp_server("0.0.0.0", 20000, "lcdserver")

def lcd_commands(text, line, column):
    global first_message
    if first_message:
        lcd_byte(0x01,LCD_CMD)
        first_message = False
    LCD_TAM = len(text)
    if text == 'clear':
        lcd_byte(0x01,LCD_CMD)
    else:
        if 1 <= line <= 4 and 1 <= column <= LCD_WIDTH:
            pos = LCD_POS[int(line)] + column - 1
            lcd_string(text,int(pos))

# Associate Python functions with message address patterns
osc_method("/lcd", lcd_commands)

def main():
    # Main program block

    GPIO.setwarnings(True)
    GPIO.setmode(GPIO.BOARD)     # Use BOARD GPIO numbers (printed on the Pi)
    GPIO.setup(LCD_E, GPIO.OUT)  # E
    GPIO.setup(LCD_RS, GPIO.OUT) # RS
    GPIO.setup(LCD_D4, GPIO.OUT) # DB4
    GPIO.setup(LCD_D5, GPIO.OUT) # DB5
    GPIO.setup(LCD_D6, GPIO.OUT) # DB6
    GPIO.setup(LCD_D7, GPIO.OUT) # DB7
    # GPIO.setup(LCD_VEE, GPIO.OUT) # Contrast for LCD text

    # Set PWM values for LCD contrast
    # my_pwm=GPIO.PWM(15,100)
    # my_pwm.start(10)
    # my_pwm.ChangeFrequency(1000)
    # my_pwm.ChangeDutyCycle(30)

    # Initialise display
    lcd_init()

    lcd_byte(0x01,LCD_CMD)
    lcd_string("GuitarAMI", LCD_LINE_1)
    lcd_string("Booting...",LCD_LINE_3)

    #time.sleep(10.0) # 1 second delay
    lcd_byte(0x01,LCD_CMD)

    lcd_string("GuitarAMI", LCD_LINE_1)
    lcd_string("Boot Complete", LCD_LINE_3)
    lcd_string("Have Fun!", LCD_LINE_4)
 
    while True:
        osc_process()

def lcd_string(message,pos):
    # Send string to display
    lcd_byte(pos, LCD_CMD)
    for i in range(len(message)):
        lcd_byte(ord(message[i]),LCD_CHR)

def lcd_init():
    # Initialise display
    lcd_byte(0x33,LCD_CMD) # 110011 Initialise
    lcd_byte(0x32,LCD_CMD) # 110010 Initialise
    lcd_byte(0x06,LCD_CMD) # 000110 Cursor move direction
    lcd_byte(0x0C,LCD_CMD) # 001100 Display On,Cursor Off, Blink Off
    lcd_byte(0x28,LCD_CMD) # 101000 Data length, number of lines, font size
    lcd_byte(0x01,LCD_CMD) # 000001 Clear display
    time.sleep(E_DELAY)

def lcd_byte(bits, mode):
    # Send byte to data pins
    # bits = data
    # mode = True  for character
    #        False for command

    GPIO.output(LCD_RS, mode) # RS

    # High bits
    GPIO.output(LCD_D4, False)
    GPIO.output(LCD_D5, False)
    GPIO.output(LCD_D6, False)
    GPIO.output(LCD_D7, False)
    if bits&0x10==0x10:
        GPIO.output(LCD_D4, True)
    if bits&0x20==0x20:
        GPIO.output(LCD_D5, True)
    if bits&0x40==0x40:
        GPIO.output(LCD_D6, True)
    if bits&0x80==0x80:
        GPIO.output(LCD_D7, True)

    # Toggle 'Enable' pin
    lcd_toggle_enable()

    # Low bits
    GPIO.output(LCD_D4, False)
    GPIO.output(LCD_D5, False)
    GPIO.output(LCD_D6, False)
    GPIO.output(LCD_D7, False)
    if bits&0x01==0x01:
        GPIO.output(LCD_D4, True)
    if bits&0x02==0x02:
        GPIO.output(LCD_D5, True)
    if bits&0x04==0x04:
        GPIO.output(LCD_D6, True)
    if bits&0x08==0x08:
        GPIO.output(LCD_D7, True)

    # Toggle 'Enable' pin
    lcd_toggle_enable()

def lcd_toggle_enable():
    # Toggle enable
    time.sleep(E_DELAY)
    GPIO.output(LCD_E, True)
    time.sleep(E_PULSE)
    GPIO.output(LCD_E, False)
    time.sleep(E_DELAY)

if __name__ == '__main__':

    try:
        main()
    except KeyboardInterrupt:
        pass
    finally:
        osc_terminate()
        lcd_byte(0x01, LCD_CMD)
        lcd_string("Goodbye!",LCD_LINE_1)
        # my_pwm=GPIO.PWM(15,100)
        # my_pwm.stop()
        GPIO.cleanup()
        sys.exit(0)
EOF
```

Set to start at boot (as service)

```bash
cat <<- "EOF" | sudo tee /lib/systemd/system/lcd.service

[Unit]
 Description=OSC to LCD
 After=multi-user.target

 [Service]
 Type=idle
 ExecStart=/usr/bin/python3 /home/patch/sources/lcd/lcd.py

 [Install]
 WantedBy=multi-user.target
EOF
```

Set permissions and enable service:

```bash
sudo chmod 644 /lib/systemd/system/lcd.service
sudo systemctl daemon-reload
sudo systemctl enable lcd.service
```

## Set LCD status bar

```bash
cat <<- "EOF" | tee ~/sources/lcd/status.py

# Python code to update the LCD with SPU info
# Edu Meneses, 2021, IDMIL, CIRMMT, McGill University

# Import needed modules from osc4py3
from osc4py3.as_eventloop import *
from osc4py3 import oscbuildparse
import subprocess
import threading
import sys

# Start the system.
osc_startup()

# Make client channels to send packets.
osc_udp_client("0.0.0.0", 20000, "lcd")

def updateStatus():
    # services returns 0 when active
    WiFiStatus = subprocess.call(['systemctl','is-active','--quiet','hostapd.service'])
    SCStatus = subprocess.call(['systemctl','--user','is-active','--quiet','supercollider.service'])
    PDStatus = subprocess.call(['systemctl','--user','is-active','--quiet','puredata.service'])
    pedalStatus = subprocess.getoutput("python3 /usr/modep/scripts/modep-ctrl.py current")
    if pedalStatus == '':
        pedalStatus = 'UNTITLED  '
    else:
        pedalStatus = pedalStatus.split('/var/modep/pedalboards/')[1].lstrip().split('.pedalboard')[0]
        pedalStatus = "{:<10}".format(pedalStatus[:10])
    if SCStatus == 0:
        msgSC = oscbuildparse.OSCMessage("/lcd", ",sii", ["SC|", 4, 1])
    else:
        msgSC = oscbuildparse.OSCMessage("/lcd", ",sii", ["  |", 4, 1])
    if PDStatus == 0:
        msgPD = oscbuildparse.OSCMessage("/lcd", ",sii", ["PD|", 4, 4])
    else:
        msgPD = oscbuildparse.OSCMessage("/lcd", ",sii", ["  |", 4, 4])
    if WiFiStatus == 0:
        msgWifi = oscbuildparse.OSCMessage("/lcd", ",sii", [" AP|", 4, 7])
    else:
        msgWifi = oscbuildparse.OSCMessage("/lcd", ",sii", ["STA|", 4, 7])
    msgPedal = oscbuildparse.OSCMessage("/lcd", ",sii", [pedalStatus, 4, 11])
    bun = oscbuildparse.OSCBundle(oscbuildparse.OSC_IMMEDIATELY,[msgSC, msgPD, msgWifi, msgPedal])
    osc_send(bun, "lcd")
    threading.Timer(5, updateStatus).start() # scheduling event every 5 seconds

updateStatus()

def main():
    while True:
        osc_process()

if __name__ == '__main__':

    try:
        main()
    except KeyboardInterrupt:
        pass
    finally:
        osc_terminate()
        sys.exit(0)
EOF
```

Set to start at boot (as service)

```bash
cat <<- "EOF" | sudo tee ~/.config/systemd/user/lcdStatus.service

[Unit]
 Description=SPU status print on LCD
 After=multi-user.target

 [Service]
 Type=idle
 ExecStart=/usr/bin/python3 /home/patch/sources/lcd/status.py

 [Install]
 WantedBy=default.target
EOF
```

Set permissions and enable service:

```bash
sudo chmod 644 ~/.config/systemd/user/lcdStatus.service
systemctl --user enable lcdStatus.service
```

## Set buttons to work

[https://raspberrypihq.com/use-a-push-button-with-raspberry-pi-gpio/](https://raspberrypihq.com/use-a-push-button-with-raspberry-pi-gpio/)

```bash
sudo apt install python3-rpi.gpio python3-gpiozero -y
sudo pip3 install osc4py3
```

Create python application:

```bash
cat <<- "EOF" | tee ~/sources/lcd/buttonOSC.py

# Python code to send button info (OSC) (SPU)
# Author: Edu Meneses (IDMIL, 2020)

# The wiring for the LCD is as follows (SPU002):
#
# button    button pin        Rpi4 BOARD pin
# Button 1
#           "left"            3V3
#           "right"           13 (GPIO 27)
#           resistor(right)   GND
# Button 2
#           "left"            3V3
#           "right"           29 (GPIO 5)
#           resistor(right)   GND
# Button 3
#           "left"            3V3
#           "right"           15 (GPIO 22)
#           resistor(right)   GND
#
# OBS: on Rpi4:
#    3V3 = pin 1 = pin 17
#    5V  = pin 2 = pin 4
#    GND = pin 6 = pin 9 = pin 14 = pin 20 = pin 25 = pin 30 = pin 34 = pin 39


from gpiozero import Button
from signal import pause

import sys
import argparse
import time

# Import needed modules from osc4py3
from osc4py3.as_eventloop import *
from osc4py3 import oscbuildparse

parser = argparse.ArgumentParser(description='Python code to send button info using OSC (SPU).')

parser.add_argument('-i', '--ip', dest='ip', type=str, default="0.0.0.0",
                    help='IP to send OSC messages. Default: 0.0.0.0')
parser.add_argument('-p', '--port', dest='port', type=int, default=8000,
                    help='port to send OSC messages. Default: 8000')
parser.add_argument('-n', '--namespace', dest='namespace', type=str, default="/guitarami_spu",
                    help='Namespace for the OSC messages. Default: /guitarami_spu')

argumentos = parser.parse_args()

buttonPin = [0,27,5,22] # use BCM (GPIO) numbers!
debounce_time = 0.05 # bouncetime in seconds

# Start the system.
osc_startup()

# Make client channels to send packets.
osc_udp_client(argumentos.ip, argumentos.port, "oscclient")

# Define button callback function for button 1(momentary):
def button_callback(arg):
    time.sleep(debounce_time)    # Wait a while for the pin to settle
    msg = oscbuildparse.OSCMessage(f"{argumentos.namespace}/button{buttonPin.index(arg.pin.number)}", ",i", [arg.value])
    osc_send(msg, "oscclient")

button1 = Button(buttonPin[1], pull_up=True)
button2 = Button(buttonPin[2], pull_up=True)
button3 = Button(buttonPin[3], pull_up=True)

button1.when_pressed = button_callback
button1.when_released = button_callback
button2.when_pressed = button_callback
button2.when_released = button_callback
button3.when_pressed = button_callback
button3.when_released = button_callback

def main():
    while True:
        osc_process()

if __name__ == '__main__':

    try:
        main()
    except KeyboardInterrupt:
        pass
    finally:
        osc_terminate()
        sys.exit(0)
EOF
```

Set the code to load at boot:

```bash
cat <<- "EOF" | sudo tee /lib/systemd/system/buttonOSC.service
[Unit]
Description=Button to OSC Python3 code
After=multi-user.target

[Service]
Type=idle
ExecStart=/usr/bin/python3 /home/patch/sources/lcd/buttonOSC.py

[Install]
WantedBy=multi-user.target
EOF
```

```bash
sudo chmod 644 /lib/systemd/system/buttonOSC.service
sudo systemctl daemon-reload
sudo systemctl enable buttonOSC.service
```

## Install sendosc

```bash
cd ~/sources
sudo apt-get install liboscpack-dev cmake
git clone https://github.com/yoggy/sendosc.git
cd sendosc
cmake .
make
sudo make install
```

Add shutdown warning: `sudo nano /usr/local/pisound/scripts/pisound-btn/shutdown.sh`

- `sendosc 127.0.0.1 20000 /lcd s "clear" i 1 i 1`
- `sendosc 127.0.0.1 20000 /lcd s "           GuitarAMI" i 1 i 1`
- `sendosc 127.0.0.1 20000 /lcd s "Shutdown            " i 2 i 1`

Usage example: `sendosc 127.0.0.1 20000 /lcd s "test message" i 1 i 1` or `sendosc 127.0.0.1 20000 /lcd s "clear" i 1 i 1`

## Jack available commands

To get a list on the computer type **jack** and hit *Tab*

|command          |command              |command                     |command              |command                 |
|-----------------|---------------------|----------------------------|---------------------|------------------------|
| jack_alias      | jack_bufsize        | jack_capture               | jack_capture_gui    | jack_connect           |
| jackdbus        | jack_disconnect     | jack-dl                    | jack-dssi-host      | jack_evmon             |
| jack_load       | jack_lsp            | jack_metro                 | jack_midi_dump      | jack_midi_latency_test |
| jack_net_master | jack_net_slave      | jack_netsource             | jack-osc            | jack-play              |
| jack_samplerate | jack-scope          | jack_server_control        | jack_session_notify | jack_showtime          |
| jack_thru       | jack_transport      | jack-transport             | jack-udp            | jack_unload            |
| jack_control    | jack_cpu            | jack_cpu_load              | jackd               | jack_wait              |
| jack_freewheel  | jack_iodelay        | jack-keyboard              | jack_latent_client  |
| jack_midiseq    | jack_midisine       | jack_monitor_client        | jack_multiple_metro |
| jack-plumbing   | jack-rack |jack_rec | jack-record                | jack_test           |
| jack_simdtests  | jack_simple_client  | jack_simple_session_client | jack_zombie         |

## Places to replace names in files

- guitarami_module_00X
  - All SC and PD patches at `~/Patches`
    - For SC code: `sed -i 's/GuitarAMI_module_001/GuitarAMI_module_00X/g' ~/Patches/*.scd`
- SPU00X
  - SSID hotspot settings: `sudo pisound-config`
  - Samba config: `sudo nano /etc/samba/smb.conf`
  - SPU Hostname: `sudo raspi-config`

Reboot

## Code to load GuitarAMI sets

```bash
cat <<- "EOF" | sudo tee /usr/local/pisound/scripts/pisound-btn/guitarami_set.sh
#!/bin/sh

# Use PiSound button to change the GuitarAMI sets (modep pedalboard + default SC patch + default PD patch)

sendosc 127.0.0.1 20000 /lcd s "clear" i 1 i 1
sendosc 127.0.0.1 20000 /lcd s "           GuitarAMI" i 1 i 1
sendosc 127.0.0.1 20000 /lcd s "Loading preset $1   " i 2 i 1

# replace spaces for underscores in file names

# for file in /home/patch/Patches/$1*/*' '*
# do
#  mv -- "$file" "${file// /_}"
# done

# set default SC file
if [ -e /home/patch/Patches/$1*/main.scd ]; then
    cp -f /home/patch/Patches/$1*/main.scd /home/patch/Patches/default.scd
elif [ -e /home/patch/Patches/default.scd ]; then
    rm /home/patch/Patches/default.scd
fi

# set default PD file
if [ -e /home/patch/Patches/$1*/main.pd ]; then
    cp -f /home/patch/Patches/$1*/main.pd /home/patch/Patches/default.pd
elif [ -e /home/patch/Patches/default.pd ]; then
    rm /home/patch/Patches/default.pd
fi

# set current MODEP pedalboard
if [ -e /home/patch/Patches/$1*/*.pedalboard ]; then
    pedalname=$(basename `find /home/patch/Patches/$1*/ -type d -name "*.pedalboard" -print -quit` .pedalboard)
    sudo cp -f -R /home/patch/Patches/$1*/$pedalname.pedalboard /var/modep/pedalboards/
    sudo chown -R modep:modep /var/modep/pedalboards/$pedalname.pedalboard
fi

# set permissions and restart services
sudo systemctl restart modep-mod-ui
# sendosc 127.0.0.1 20000 /lcd s "Loading             " i 2 i 1
# sendosc 127.0.0.1 20000 /lcd s $pedalname i 3 i 1

sleep 5

systemctl --user restart supercollider.service
systemctl --user restart puredata.service
python3 /usr/modep/scripts/modep-ctrl.py load-board "/var/modep/pedalboards/$pedalname.pedalboard"

EOF
```

```bash
sudo chmod +x /usr/local/pisound/scripts/pisound-btn/guitarami_set.sh
```

## Script to print IP addresses on the LCD

```bash
cat <<- "EOF" | sudo tee /usr/local/pisound/scripts/pisound-btn/print_ip.sh
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
EOF
```

```bash
sudo chmod +x /usr/local/pisound/scripts/pisound-btn/print_ip.sh
```

## Configure PiSound button behaviour

```bash
cat <<- "EOF" | sudo tee /usr/local/patchbox-modules/modep/pisound-btn.conf
DOWN              /usr/local/pisound/scripts/pisound-btn/system/down.sh
UP                /usr/local/pisound/scripts/pisound-btn/system/up.sh

CLICK_1           /usr/local/pisound/scripts/pisound-btn/guitarami_set.sh
CLICK_2           /usr/local/pisound/scripts/pisound-btn/guitarami_set.sh
CLICK_3           /usr/local/pisound/scripts/pisound-btn/guitarami_set.sh
CLICK_OTHER       /usr/local/pisound/scripts/pisound-btn/guitarami_set.sh

CLICK_COUNT_LIMIT 8

HOLD_1S           /usr/local/pisound/scripts/pisound-btn/print_ip.sh
HOLD_3S           /usr/local/pisound/scripts/pisound-btn/toggle_wifi_hotspot.sh
HOLD_5S           /usr/local/pisound/scripts/pisound-btn/shutdown.sh
HOLD_OTHER        /usr/local/pisound/scripts/pisound-btn/do_nothing.sh
EOF
```

```bash
sudo cp /usr/local/patchbox-modules/modep/pisound-btn.conf /etc/pisound.conf
```

## Update tool (bash)

```bash
cat <<- "EOF" | sudo tee ~/sources/lcd/updatetool.sh
#!/bin/bash

if [ -e /home/patch/Patches/guitarami.update ]; then
    sudo chmod +x /home/patch/Patches/guitarami.update
    /home/patch/Patches/guitarami.update
fi

EOF
```

Set the code to load at boot:

```bash
cat <<- "EOF" | sudo tee /lib/systemd/system/updatetool.service
[Unit]
Description=GuitarAMI update tool
After=multi-user.target

[Service]
Type=idle
ExecStart=/home/patch/sources/lcd/updatetool.sh

[Install]
WantedBy=multi-user.target
EOF
```

```bash
sudo chmod +x ~/sources/lcd/updatetool.sh
sudo chmod 644 /lib/systemd/system/updatetool.service
sudo systemctl daemon-reload
sudo systemctl enable updatetool.service
```

## update file model

```bash
cat <<- "EOF" | tee /home/patch/Patches/guitarami.update
#!/bin/sh

# model script to update the GuitarAMI.
# This file will be deleted after the upgrade is complete
# Edu Meneses, 2021, IDMIL, CIRMMT, McGill University

# Do your stuff:

sed -i.bak 's/, bounce_time=0.005//' ~/sources/lcd/buttonOSC.py

# remove this script
rm $0
EOF
```

## Install Libmapper/Webmapper/LMJack

```bash
cat <<- "EOF" | tee ~/sources/install_libmapper.sh
#!/bin/bash

# exit when any command fails
set -e

# keep track of the last executed command
trap 'last_command=$current_command; current_command=$BASH_COMMAND' DEBUG
# echo an error message before exiting
trap 'echo "\"${last_command}\" command filed with exit code $?."' EXIT

echo
echo
echo 'GuitarAMI base instalation bash script'
echo 'Libmapper/Webmapper/LMJack'
echo
echo 'Do not run with SUDO!!'
echo
echo

read -p "Do you want to continue? " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]
then
echo
echo
echo '<--------------------INSTALLING DEPENDENCIES------------------->'
sudo apt install git python-dev python3-dev python-pip python3-pip autoconf libtool automake libasound2-dev ant libfftw3-3 python-numpy python3-numpy -y
sudo apt install -y default-jdk
echo
echo
echo '<----------------------DOWNLOADING liblo----------------------->'
cd ~/sources
wget https://sourceforge.net/projects/liblo/files/liblo/0.31/liblo-0.31.tar.gz
echo
echo
echo '<------------------------UNPACKING liblo----------------------->'
tar -xvf liblo-0.31.tar.gz
echo
echo
echo '<-----------------------INSTALLING liblo----------------------->'
cd liblo-0.31
chmod a+rwx configure
./configure
make check
sudo make install
echo
echo
echo '<--------------------DOWNLOADING libmapper-------------------->'
cd ~/sources
git clone https://www.github.com/libmapper/libmapper
cd libmapper
echo
echo
echo '<-----------------------INSTALLING SWIG----------------------->'
sudo apt install 2to3 -y
sudo apt install swig -y
echo
echo
echo '<---------------------INSTALLING libmapper-------------------->'
./autogen.sh PYTHON=python3
make
sudo make install
export PKG_CONFIG_PATH=~/sources/libmapper
export LD_LIBRARY_PATH=/usr/local/lib/
sudo ldconfig
cd ~
echo
echo
echo '<---------------------INSTALLING NETIFACES-------------------->'
pip install netifaces
pip3 install netifaces
echo
echo  
echo '<--------------------DOWNLOADING WEBMAPPER-------------------->'
cd ~/sources
git clone https://www.github.com/libmapper/webmapper
echo
echo  
echo '<---------------------INSTALLING WEBMAPPER-------------------->'
cd webmapper
cp ~/sources/libmapper/swig/mapper.py ./
cp ~/sources/libmapper/swig/_mapper*.so ./
export LD_LIBRARY_PATH=/usr/local/lib
echo  >> ~/.profile
echo 'export LD_LIBRARY_PATH=/usr/local/lib' >> ~/.profile
echo
echo  
echo 'Installation complete'

trap - EXIT
exit 0

fi

EOF
```

```bash
cd ~/sources
sudo chmod +x ./install_libmapper.sh
./install_libmapper.sh
```

If needed, add mapper to PYTHONPATH:

- get *SITEDIR* or python 2.X using `python -m site --user-site`, and 3.X using `python3 -m site --user-site`
- create them if they don't exist: `mkdir -p "$SITEDIR"`
- create new .pth file with our path: `echo "$HOME/sources/libmapper/swig" > "$SITEDIR/mapper.pth"`
- run `python -m mapper` and `python3 -m mapper`

Some modifications on Webmapper config to make it work fine:

- `nano -l ~/sources/webmapper/webmapper.py`
  - Add after line 44: `g.set_interface('eth0')` (change to wlan0 after setup is done)
  - in get_interfaces (~line 338):

      ```python
      elif ( 'eth0' in networkInterfaces['available'] ):
      networkInterfaces['active'] = 'eth0'
      ```
