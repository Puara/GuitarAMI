# How to build: GuitarAMI guitar module

## BOM

| Qty | Description                                            | Online Reference                                                                            |
| --- |:-------------------------------------------------------|:------------------------------------------------------------------------------------------- |
| 1   | Rocker Switch                                          | https://www.digikey.ca/en/products/detail/bulgin/H8600VBACN/9598449                         |
| 1   | Small Suction Cups without Hooks                       | https://www.uline.ca/Product/Detail/S-17647/Store-Supplies/Suction-Cups-without-Hooks-Small |
| 1   | Perma-Proto Small Mint Tin Size Breadboard PCB         | https://www.adafruit.com/product/1214                                                       |
| 1   | Ultrasonic Distance Sensor - 3V compatible - RCWL-1601 | https://www.adafruit.com/product/4007                                                       |
| 1   | TinyPICO - ESP32 Development Board                     | https://www.adafruit.com/product/4335                                                       |
| 1   | 3.7V 2000mAh LiPo Rechargeable Battery                 | https://www.adafruit.com/product/2011                                                       |
| 1   | LSM9DS1 nine-axis Sensor Module 9-axis IMU             | https://www.sparkfun.com/products/13284                                                     |

## Instructions

Start with the Perma-Proto Breadboard and solder all required connections according to the picture.

The IMU uses I2C, and it will be connected on rows 11-14 (breadboard) pins.
The TinyPico I2C (SDA and SCL) connections will be soldered on rows 3 and 2 of the breadboard (opposite side).

The ultrasonic sensor trigger and echo will be connected on rows 9 and 8 on the breadboard.
The TinyPico pins for that will be connected on rows 4 and 5 on the breadboard (same side).

The remaining jumper wires are for VCC and ground.

> :warning: **The TinyPico cannot reliably provide 5V when powered from battery**: Make sure the chosen ultrasonic sensor operates with 3.3V!

![Start](./images_building/01_start.jpg "Start")

Proceed by soldering the IMU:

![IMU](./images_building/02_imu.jpg "IMU")

Then the ultrasonic sensor. Note that the sensor is soldered "upside down".

![Ultrasonic](./images_building/03_ult.jpg "Ultrasonic")

Start preparing the TinyPico: set the header pins according to the picture below:

![TinyPico part 1](./images_building/04_tiny1.jpg "Tiny Pico part 1")

Solder the pins on the TinyPico...

![TinyPico part 2](./images_building/05_tiny2.jpg "Tiny Pico part 2")

... and the TinyPico to the breadboard according to the picture below.
Note that the USB connector must be aligned with the end of the breadboard.
That means some overlapping with the IMU is required.

![TinyPico part 3](./images_building/06_tiny3.jpg "Tiny Pico part 3")

Start preparing the touch sensor: a piece of copper tape and some wiring:

![Touch part 1](./images_building/07_touch1.jpg "Touch part 1")

Tape and solder according to the picture:

![Touch part 2](./images_building/08_touch2.jpg "Touch part 2")

Protect the copper tape to prevent sensor acquisition problems:

![Touch part 3](./images_building/09_touch3.jpg "Touch part 3")

Lastly, prepare the rocker switch:

![Rocker Switch](./images_building/10_switch.jpg "Rocker Switch")

Add the 3D printed enclosure and assemble everything together according to the pictures below.
The battery caps protect the breadboard and battery, and it is recommended to use the bottom cap.
The top cap can be used to prevent the battery to shake inside the module, if needed.

![Putting everything together part 1](./images_building/11_assembly.jpg "Putting everything together part 1")

The top cap can be used to prevent the battery to shake inside the module, if needed.

![Putting everything together part 2](./images_building/12_assembly2.jpg "Putting everything together part 2")

Done! Have fun.

![Done!](./images_building/13_done.jpg "Done!")
