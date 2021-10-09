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