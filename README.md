# Thermal Camera 

This project was inspired by the [Thermal Camera with Display](https://learn.adafruit.com/thermal-camera-with-display/overview)  project from Adafruit.

Although the project is very similar to Adafruit’s on the hardware side, the Arduino code (C language) was extensively modified to add the following features:

- Battery status/charge.
- Min./Max. temperature values represented by light blue areas (min.) and dark red areas (max.) on the LCD screen.
- Temperature range adjustment by tapping on the screen’s lower right corner. There are 3 temperature ranges to choose from:
	- 28C to 30C
	- 20C to 80C
	- 50C to 100C

When switching between ranges, the min. and max. temperature values are shown on screen. The number of ranges and temperature thresholds per range are easily configurable in the code. Just revise the line defining the `tempRange` array at the top of the sketch:

`int tempRange[] = {20, 28, 20, 80, 50, 100};`

This repository also includes files of a 3D printed case I designed to hold all components in a relatively small package.

More information can be found at www.movingelectrons.net.

