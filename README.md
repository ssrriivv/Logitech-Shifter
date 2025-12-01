# PLEASE NOTE

This is a fork of n-e-y-s' G27 shifter and pedals project, but it is a massive departure as it's for a completely different architechture of controller and I only made it for the shifter.

# Logitech Shifter

ESP32 (using arduino bootloader) interface for Logitech shifter:

![on breadboard](screenshots/Breadboard.jpg)
![in altoids tin](screenshots/Altoids_Tin.jpg)

## Credits

This fork wouldn't have been possible without the great project from functionreturnfunction and n-e-y-s. I decided to fork the project to give them a shoutout for credits.

## Required Parts/Materials

* [SparkFun Thing Plus ESP32](https://www.sparkfun.com/sparkfun-thing-plus-esp32-wroom-micro-b.html)
* [DB9 Connectors](http://www.amazon.com/Female-Male-Solder-Adapter-Connectors/dp/B008MU0OR4/ref=sr_1_1?ie=UTF8&qid=1457291922&sr=8-1&keywords=db9+connectors) 1 male, 1 female
* Hookup wire in assorted colors (I used red, black, blue, green, purple, yellow, orange, and white)
* You can also rawdog it and not use a DB9 Connector if you just want to chop the wire, and solder it directly. I will later upload a breakout schematic but I assume you can also find it yourself.
## Firmware

Open the .ino file in the Arduino IDE, select the proper board type and COM port under "Tools" (you will need to install the [SparkFun board library](https://github.com/sparkfun/Arduino_Boards)). 

Make sure you select the sparkfun thing plus. If you used a different ESP32 board, use what's appropriate
## Calibration and Configration

Right now there is no way of calibrating it, it's calibrated to my shifter. Will add support later
