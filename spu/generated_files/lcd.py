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
#from time import sleep
#from signal import pause
#from datetime import datetime
#from time import strftime
from threading import Event
import socket
import os
import types
import sys

# Import needed modules from osc4py3
from osc4py3.as_eventloop import *
from osc4py3 import oscmethod as osm

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

    # time.sleep(10.0) # 1 second delay
    lcd_byte(0x01,LCD_CMD)

    lcd_string("GuitarAMI", LCD_LINE_1)
    lcd_string("Boot Complete", LCD_LINE_3)
    lcd_string("Have Fun!", LCD_LINE_4)

    while True:
        forever = Event(); forever.wait()

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