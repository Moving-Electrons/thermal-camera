/***************************************************************************
  This sketch is a revision of the one offered by Adafruit to work with 
  the AMG8833 GridEYE sensor. It uses bicubic interpolation (coded by Adafruit) to
  show thermal image. The following features have been added:

  - Battery charge indicator.
  - Tapping the lower right area of the screen changes the thermal sensor scale.

  The following comments have been left as reference:

  -------------------------------------------------------------------------
  
  This is a library for the AMG88xx GridEYE 8x8 IR camera

  This sketch makes an inetrpolated pixel thermal camera with the 
  GridEYE sensor and a 2.4" tft featherwing:
	 https://www.adafruit.com/product/3315

  Designed specifically to work with the Adafruit AMG8833 Featherwing
          https://www.adafruit.com/product/3622

  These sensors use I2C to communicate. The device's I2C address is 0x69

  Adafruit invests time and resources providing this open source code,
  please support Adafruit andopen-source hardware by purchasing products
  from Adafruit!

  Written by Dean Miller, James DeVito & ladyada for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ***************************************************************************/

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ILI9341.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_AMG88xx.h>
#include <Adafruit_STMPE610.h> //Touchscreen controller drivers

#ifdef ESP32
   #define STMPE_CS 32
   #define TFT_CS   15
   #define TFT_DC   33
   #define SD_CS    14
#endif

// The following definitions have been left in so the code is compatible with 
// different micro-controllers. Remove if the Huzzah32 is the one that will be used.

#ifdef ESP8266
   #define STMPE_CS 16
   #define TFT_CS   0
   #define TFT_DC   15
   #define SD_CS    2
#endif

#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega328P__)
   #define STMPE_CS 6
   #define TFT_CS   9
   #define TFT_DC   10
   #define SD_CS    5
#endif
#ifdef ARDUINO_SAMD_FEATHER_M0
   #define STMPE_CS 6
   #define TFT_CS   9
   #define TFT_DC   10
   #define SD_CS    5
#endif
#ifdef TEENSYDUINO
   #define TFT_DC   10
   #define TFT_CS   4
   #define STMPE_CS 3
   #define SD_CS    8
#endif
#ifdef ARDUINO_STM32_FEATHER
   #define TFT_DC   PB4
   #define TFT_CS   PA15
   #define STMPE_CS PC7
   #define SD_CS    PC5
#endif
#ifdef ARDUINO_FEATHER52
   #define STMPE_CS 30
   #define TFT_CS   31
   #define TFT_DC   11
   #define SD_CS    27
#endif
#ifdef __SAMD51__
   #define STMPE_CS 6
   #define TFT_CS   9
   #define TFT_DC   10
   #define SD_CS    5
   
  Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, MOSI, SCK);
#else
  Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

#endif


Adafruit_STMPE610 ts = Adafruit_STMPE610(STMPE_CS);

// Touchscreen settings:
// This is calibration data for the raw touch data to the screen coordinates.
// Original calibration values from Adafruit.
#define TS_MINX 3800
#define TS_MAXX 100
#define TS_MINY 100
#define TS_MAXY 3750

//Comment this out to remove the text overlay
//#define SHOW_TEMP_TEXT

// Temperature Ranges
// include integer pairs (low temp and high temp) in the array below.
// low temps will be blue and high temps will be red on screen.
int tempRange[] = {20, 28, 20, 80, 50, 100};

// Auxiliary integers for switching between ranges
int tempIndex = 0;
int arrSize = sizeof(tempRange) / sizeof(int);


int battVal = 0;
float battVolt = 0;


//the colors we will be using
const uint16_t camColors[] = {0x480F,
0x400F,0x400F,0x400F,0x4010,0x3810,0x3810,0x3810,0x3810,0x3010,0x3010,
0x3010,0x2810,0x2810,0x2810,0x2810,0x2010,0x2010,0x2010,0x1810,0x1810,
0x1811,0x1811,0x1011,0x1011,0x1011,0x0811,0x0811,0x0811,0x0011,0x0011,
0x0011,0x0011,0x0011,0x0031,0x0031,0x0051,0x0072,0x0072,0x0092,0x00B2,
0x00B2,0x00D2,0x00F2,0x00F2,0x0112,0x0132,0x0152,0x0152,0x0172,0x0192,
0x0192,0x01B2,0x01D2,0x01F3,0x01F3,0x0213,0x0233,0x0253,0x0253,0x0273,
0x0293,0x02B3,0x02D3,0x02D3,0x02F3,0x0313,0x0333,0x0333,0x0353,0x0373,
0x0394,0x03B4,0x03D4,0x03D4,0x03F4,0x0414,0x0434,0x0454,0x0474,0x0474,
0x0494,0x04B4,0x04D4,0x04F4,0x0514,0x0534,0x0534,0x0554,0x0554,0x0574,
0x0574,0x0573,0x0573,0x0573,0x0572,0x0572,0x0572,0x0571,0x0591,0x0591,
0x0590,0x0590,0x058F,0x058F,0x058F,0x058E,0x05AE,0x05AE,0x05AD,0x05AD,
0x05AD,0x05AC,0x05AC,0x05AB,0x05CB,0x05CB,0x05CA,0x05CA,0x05CA,0x05C9,
0x05C9,0x05C8,0x05E8,0x05E8,0x05E7,0x05E7,0x05E6,0x05E6,0x05E6,0x05E5,
0x05E5,0x0604,0x0604,0x0604,0x0603,0x0603,0x0602,0x0602,0x0601,0x0621,
0x0621,0x0620,0x0620,0x0620,0x0620,0x0E20,0x0E20,0x0E40,0x1640,0x1640,
0x1E40,0x1E40,0x2640,0x2640,0x2E40,0x2E60,0x3660,0x3660,0x3E60,0x3E60,
0x3E60,0x4660,0x4660,0x4E60,0x4E80,0x5680,0x5680,0x5E80,0x5E80,0x6680,
0x6680,0x6E80,0x6EA0,0x76A0,0x76A0,0x7EA0,0x7EA0,0x86A0,0x86A0,0x8EA0,
0x8EC0,0x96C0,0x96C0,0x9EC0,0x9EC0,0xA6C0,0xAEC0,0xAEC0,0xB6E0,0xB6E0,
0xBEE0,0xBEE0,0xC6E0,0xC6E0,0xCEE0,0xCEE0,0xD6E0,0xD700,0xDF00,0xDEE0,
0xDEC0,0xDEA0,0xDE80,0xDE80,0xE660,0xE640,0xE620,0xE600,0xE5E0,0xE5C0,
0xE5A0,0xE580,0xE560,0xE540,0xE520,0xE500,0xE4E0,0xE4C0,0xE4A0,0xE480,
0xE460,0xEC40,0xEC20,0xEC00,0xEBE0,0xEBC0,0xEBA0,0xEB80,0xEB60,0xEB40,
0xEB20,0xEB00,0xEAE0,0xEAC0,0xEAA0,0xEA80,0xEA60,0xEA40,0xF220,0xF200,
0xF1E0,0xF1C0,0xF1A0,0xF180,0xF160,0xF140,0xF100,0xF0E0,0xF0C0,0xF0A0,
0xF080,0xF060,0xF040,0xF020,0xF800,};

// AMG8833 Code from Adafruit:

Adafruit_AMG88xx amg;
unsigned long delayTime;

#define AMG_COLS 8
#define AMG_ROWS 8
float pixels[AMG_COLS * AMG_ROWS];

// Change values below to 16 for faster processing but less on-screen definition.
#define INTERPOLATED_COLS 24
#define INTERPOLATED_ROWS 24


float get_point(float *p, uint8_t rows, uint8_t cols, int8_t x, int8_t y);
void set_point(float *p, uint8_t rows, uint8_t cols, int8_t x, int8_t y, float f);
void get_adjacents_1d(float *src, float *dest, uint8_t rows, uint8_t cols, int8_t x, int8_t y);
void get_adjacents_2d(float *src, float *dest, uint8_t rows, uint8_t cols, int8_t x, int8_t y);
float cubicInterpolate(float p[], float x);
float bicubicInterpolate(float p[], float x, float y);
void interpolate_image(float *src, uint8_t src_rows, uint8_t src_cols, 
                       float *dest, uint8_t dest_rows, uint8_t dest_cols);

void setup() {

  delay(500);
  Serial.begin(115200);  
  Serial.println("\n\nAMG88xx Interpolated Thermal Camera!");

  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(ILI9341_BLACK);
    
  // Attempting to initiate AMG8833 sensor 
  if (!amg.begin()) {
    Serial.println("Could not find a valid AMG88xx sensor, check wiring!");
    while (1) { delay(1); }
  }

  // Attempting to start touchscreen controller
  if (!ts.begin()) {
    Serial.println("Couldn't start touchscreen controller");
    while (1);
  }

  // Drawing temperature values with first pair on the 
  // range array.
  drawRange(tempRange[0], tempRange[1]);
    
  Serial.println("-- Thermal Camera Test --");
  // IMPORTANT!: delay below allows the sensor to warm up.
  delay(250);
}

void loop() {

  // Reading battery voltage:
  battVal = analogRead(35);
  // Calculating actual voltage per http://cuddletech.com/?p=1030
  battVolt = ((float)battVal/4095)*2*3.3*1.1;
  Serial.print("Battery voltage: "); Serial.println(battVolt);
  drawBattery(battVolt);
   
  //reading all pixels
  amg.readPixels(pixels);

  // read touch screen point
  TS_Point p = ts.getPoint();

  // Scale from ~0->4000 to tft.width using the calibration #'s
  // Touch controller assumes 0,0 on left and top corner.Play with number below to adjust.
  p.x = map(p.x, TS_MINX, TS_MAXX, 0, tft.height());
  p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.width());

  Serial.print("X = "); Serial.print(p.x);
  Serial.print("\tY = "); Serial.print(p.y);
  Serial.print("\tPressure = "); Serial.println(p.z);

  if (ts.touched()) {
    
    // Detecting touch point in lower right corner
    // the lower the z value the harder/higher the pressure on screeen
    if (p.x > 70 && p.y < 80 && p.z < 55) {
  
      tempIndex = tempIndex + 2;
  
      if (tempIndex >= arrSize) tempIndex = 0;
      
      drawRange(tempRange[0+tempIndex], tempRange[1+tempIndex]);
   
    }
  }

  
  Serial.print("[");
  for(int i=1; i<=AMG88xx_PIXEL_ARRAY_SIZE; i++){
    Serial.print(pixels[i-1]);
    Serial.print(", ");
    if( i%8 == 0 ) Serial.println();
  }
  Serial.println("]");
  Serial.println();

  float dest_2d[INTERPOLATED_ROWS * INTERPOLATED_COLS];

  int32_t t = millis();
  interpolate_image(pixels, AMG_ROWS, AMG_COLS, dest_2d, INTERPOLATED_ROWS, INTERPOLATED_COLS);
  Serial.print("Interpolation took "); Serial.print(millis()-t); Serial.println(" ms");

  uint16_t boxsize = min(tft.width() / INTERPOLATED_COLS, tft.height() / INTERPOLATED_COLS);
  
  drawpixels(dest_2d, INTERPOLATED_ROWS, INTERPOLATED_COLS, boxsize, boxsize, false);

  //delay(50);
}

void drawpixels(float *p, uint8_t rows, uint8_t cols, uint8_t boxWidth, uint8_t boxHeight, boolean showVal) {
  int colorTemp;
  for (int y=0; y<rows; y++) {
    for (int x=0; x<cols; x++) {
      float val = get_point(p, rows, cols, x, y);
      if(val >= tempRange[1+tempIndex]) colorTemp = tempRange[1+tempIndex];
      else if(val <= tempRange[0+tempIndex]) colorTemp = tempRange[0+tempIndex];
      else colorTemp = val;
      
      uint8_t colorIndex = map(colorTemp, tempRange[0+tempIndex], tempRange[1+tempIndex], 0, 255);
      colorIndex = constrain(colorIndex, 0, 255);
      //draw the pixels!
      uint16_t color;
      color = val * 2;

   
      // left Margin thermal image: 15 pixels.
      tft.fillRect(15+boxWidth * x, boxHeight * y, boxWidth, boxHeight, camColors[colorIndex]);
        
      if (showVal) {
        tft.setCursor(boxWidth * y + boxWidth/2 - 12, 40 + boxHeight * x + boxHeight/2 - 4);
        tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(1);
        tft.print(val,1);
      }
    } 
  }
}


void drawBattery(float battVolt){

  int battPercent, battmVolt = 0;

  // Cleaning screen on battery percentage value
  tft.fillRect(260,5,60,45,ILI9341_BLACK);
  
  
  tft.setCursor(260,5);

  tft.setTextColor(ILI9341_WHITE); 
  tft.setTextSize(2);
  tft.print("Batt:");
  
  battVolt = battVolt*1000; //converting to milivolts before casting.
  battmVolt = (int)battVolt;
  // map only works with integers, then voltage values are entered in mili-Volts
  // instead of volts:
  battPercent = map(battmVolt, 3200, 4250, 0, 100);
  
  Serial.print("batt volt corrected: "); Serial.println(battmVolt);
  Serial.print("batt percent: "); Serial.println(battPercent);
  

  if(battPercent < 21){
    tft.setTextColor(ILI9341_RED);
  }
  else {
    tft.setTextColor(ILI9341_GREEN);
  }
  
  tft.setCursor(260,25);
  tft.print(battPercent); tft.print("%");
  
  
}

void drawRange(int lowLimit, int highLimit){

  // Cleaning screen on temperature values and setting text
  tft.fillRect(260,140,60,20,ILI9341_BLACK);
  tft.fillRect(260,190,60,20,ILI9341_BLACK);
  
  tft.setTextColor(ILI9341_WHITE); 
  tft.setTextSize(2);
  
  tft.setCursor(260,90);
  tft.print("Temp.");
  
  tft.setCursor(260,120);
  tft.print("Min:");
  tft.setCursor(260,140);
  tft.print(lowLimit); tft.print("C");

  tft.setCursor(260,170);
  tft.print("Max:");
  tft.setCursor(260,190);
  tft.print(highLimit); tft.print("C");
  
}

