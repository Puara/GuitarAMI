# Builing Instructions - GuitarAMI Sound Processing Unit

Using Rpi 4B + Fe-Pi + PatchboxOS

## Prepare PatchboxOS

- Download the [PatchboxOS](https://blokas.io/patchbox-os/#patchbox-os-download)
- Flash the image using [ApplePiBaker](https://www.tweaking4all.com/hardware/raspberry-pi/applepi-baker-v2/), [balenaEtcher](https://www.balena.io/etcher/), or diskutil (dd)
- Boot the Rpi and folow the [first run tutorial](https://blokas.io/patchbox-os/docs/first-run-options/)
  - ssh to th SPU: `ssh patch@ip_address` (The default user name is *patch* and its password is *blokaslabs*)
    - update PatchBox
    - set new password: `mappings`
    - choose default soundcard: Built-in BCM audio jack (bcm2835_alsa), 44100, 512, 3
    - select boot: *console autologin*
    - connect to network: *no*
    - choose module: *modep*

## Finish PatchboxOS config

- Enter PiSound configuration: `sudo pisound-config`
  - Update Pisound
  - Change Pisound button settings:
    - All buttons should DO NOTHING
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

/usr/modep/scripts/modep-ctrl.py

## Enable Fe-Pi Audio interface

[https://fe-pi.com/p/support-and-setup](https://fe-pi.com/p/support-and-setup). Link down in 2021/02/25

```bash
sudo sed -i -e 's/Enable audio (loads snd_bcm2835)/Disable audio (loads snd_bcm2835)/ ; s/dtparam=audio=on/dtparam=audio=off\n\n# Enable Fe-Pi\ndtoverlay=fe-pi-audio/' /boot/config.txt
```

Reboot

Enter patchBox config: `patchbox` and change soundcard to Fe-Pi (Jack -> config ->): Fe, 48000, 128, 2

To change sound settings (terminal) use `alsamixer`, to save the current settings use `sudo alsactl store 0`

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

# The wiring for the LCD is as follows (SPU with Fe-Pi):
#
# LCD pin    Rpi4 BOARD pin  function
# 1          GND             GND
# 2          5V              5V
# 3          5V, GND         Contrast (0-5V), use a potentiometer here: middle leg to LCD pin 3, sides to Rpi pin 2 or 4 (5V) and 14(GND)
# 4          26 (GPIO 7)     RS (Register Select)
# 5          GND             R/W (Read Write) - GROUND THIS PIN
# 6          23 (GPIO 11)    Enable or Strobe
# 7          -               Data Bit 0 - NOT USED
# 8          -               Data Bit 1 - NOT USED
# 9          -               Data Bit 2 - NOT USED
# 10         -               Data Bit 3 - NOT USED
# 11         22 (GPIO 23)    Data Bit 4
# 12         18 (GPIO 24)    Data Bit 5
# 13         16 (GPIO 25)    Data Bit 6
# 14         11 (GPIO 17)    Data Bit 7
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
LCD_E  = 23
LCD_D4 = 22
LCD_D5 = 18
LCD_D6 = 16
LCD_D7 = 11
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
 ExecStart=/usr/bin/python3 /home/pi/sources/lcd/lcd.py

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
 ExecStart=/usr/bin/python3 /home/pi/sources/lcd/status.py

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
debounce_time = 0.005 # bouncetime in seconds

# Start the system.
osc_startup()

# Make client channels to send packets.
osc_udp_client(argumentos.ip, argumentos.port, "oscclient")

# Define button callback function for button 1(momentary):
def button_callback(arg):
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
ExecStart=/usr/bin/python3 /home/pi/sources/lcd/buttonOSC.py

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
- SPU00X
  - SSID hotspot settings: `sudo pisound-config`
  - Samba congif: `/etc/samba/smb.conf`
  - SPU Hostname: `sudo raspi-config`

Reboot
