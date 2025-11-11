Displaying a character table on a 1602 or 2004 LCD.

The LCD is connected via I2C. An Attiny44A microcontroller is used. 

The buttons, when pressed, set the display initialization mode. 

Pin. 2 - LCD 20x4, pin. 3 - Russian font (if available), pin. 4 - additional font (if available). 

The refresh period is 3 seconds, set in EEPROM cell #00. After the first cycle, pressing the button on pin 3 stops scrolling.

![Schematic](https://github.com/vad7/LCD1602-test/blob/master/LCDTester3.png)


Firmware: https://github.com/vad7/LCD1602-test/blob/master/Debug/LCDTest.hex

FUSES: BODLEVEL = 1V8, RSTDISBL
