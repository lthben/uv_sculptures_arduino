/*
  Author: Benjamin Low (Lthben@gmail.com)
  Date: Oct 2019
  Description: UV Sculptures 1 - Ann 

      Arduino Mega 2560 with DFPlayer Mini MP3 player module. LED flickering observed with using the 3.3V Teensy.

      Each sculpture has one button and one slider each.
      The button is playback mode for the light readings. The slider is toggle mode for the different readings. 
      Both have bands of neon flex leds to show the UV levels in the form of light. 
      There are sounds associated with all light animations, including a pulsing idle animation. 
      
      SCULPTURE1 (curved light strips) is Ann, SCULPTURE2 (straight light strips) is Soh and Suang Suang
      SCULPTURE 1 has 10 led strips. SCULPTURE 2 has 9.
      SCULPTURE 1 uses a slider. 2 uses a knob  
*/

#include <Arduino.h>
#include <FastLED.h>
#include <Bounce2.h>
#include <elapsedMillis.h>
#include "DFRobotDFPlayerMini.h"

//-------------------- PIN ASSIGNMENT --------------------//
// Mapping for max of 10 data pins. For Mega, use D2 to D13 which has PWM

const int LEDPIN0 = 2, LEDPIN1 = 3, LEDPIN2 = 4, LEDPIN3 = 5, LEDPIN4 = 6, LEDPIN5 = 7, LEDPIN6 = 8, LEDPIN7 = 9, LEDPIN8 = 10, LEDPIN9 = 11;

const int buttonPin = 0;
const int sliderPin = A0;

//-------------------- USER DEFINED SETTINGS --------------------//

/* CHECK IF CHECK_CONSOLE() IS COMMENTED OUT!!!!!!!!!*/

#define __SCULPTURE1__  //Ann
// #define __SCULPTURE2__  //Soh & Suang

const int MAXBRIGHTLVL = 160;

//band 1 is inner most centre where the idle fade animation starts from
//number of pixels (every 10cm) for each data pin controlled led strip
//number system is NX_BX_SX where NX is SCULPTURE ID number, BX is band number and SX is the individual strip number within the band
const int p1_1_x = 3, p1_2_x = 6, p1_3_x = 10, p1_4_1 = 14, p1_4_2 = 14, p1_5_1 = 18, p1_5_2 = 18, p1_6_1 = 22, p1_6_2 = 22, p1_6_3 = 23; //SCULPTURE1. 6 bands. 10 strips.
const int p2_1_1 = 9, p2_1_2 = 9, p2_1_3 = 9, p2_2_x = 20, p2_3_x = 22, p2_4_x = 11, p2_5_x = 11, p2_6_x = 11, p2_7_x = 11;//Sculpture 2. 7 bands. 9 strips.

const int NUMDATA1 = 5, NUMDATA2 = 8; //number of data points for each sculpture
const float ann_readings[NUMDATA1] = {2.94, 4.27, 4.09, 0.42, 8.0}; //actual values
// const float ann_readings[NUMDATA1] = {1.0, 8.0, 1.0, 8.0, 1.0}; //test values

const float sohsuang_readings[NUMDATA2] = {0.13, 0.06, 1.6, 4.38, 7.0, 1.51, 3.8, 0.04}; //actual values
// const float sohsuang_readings[NUMDATA2] = {1.0, 7.0, 1.0, 7.0, 1.0, 7.0, 1.0, 7.0}; //test values

const int BAND_DELAY = 500;   //ms delay between each band lightup
const int SLIDER_WAIT = 3000; //ms idle for slider movement before IDLE_MODE kicks in

CHSV cyellow(64, 255, 255); //sculpture 1 idle mode colour
CHSV cpink(224, 255, 255); //sculpture 2 idle mode colour
CHSV corange(32, 255, 255);
CHSV cred(0, 255, 255);

//-------------------- Audio --------------------//
DFRobotDFPlayerMini myDFPlayer;

//-------------------- Buttons and Sliders --------------------//
Bounce myButton = Bounce(buttonPin, 15); // 15 = 15 ms debounce time

unsigned int sliderVal, prevSliderVal, currSliderVal;
unsigned int sliderPosIndex, prevSliderPosIndex, currSliderPosIndex; //0 - 4 for Ann, 0 - 7 for Soh & Suang

bool isButtonPressed, isSliderToggled;

//-------------------- Light --------------------//

#define LED_TYPE WS2812 //Not UCS1903
#define COLOR_ORDER GRB //Yes! GRB!

#if defined(__SCULPTURE1__)
CHSV myIdleColor = cyellow;
const int SCULPTURE_ID = 1;
const int idleTrackNum = 4, activeTrackNum = 1;
CRGB leds0[p1_1_x], leds1[p1_2_x], leds2[p1_3_x], leds3[p1_4_1], leds4[p1_4_2], leds5[p1_5_1], leds6[p1_5_2], leds7[p1_6_1], leds8[p1_6_2], leds9[p1_6_3];
const int n1 = p1_1_x, n2 = p1_2_x, n3 = p1_3_x, n4 = p1_4_1, n5 = p1_4_2, n6 = p1_5_1, n7 = p1_5_2, n8 = p1_6_1, n9 = p1_6_2, n10 = p1_6_3; //for common code for both sculptures
float vol = 10; //hard to access the vol knob on the amp, so adjust in software

#elif defined(__SCULPTURE2__)
CHSV myIdleColor = cpink;
const int SCULPTURE_ID = 2;
const int idleTrackNum = 2, activeTrackNum = 3;
CRGB leds0[p2_1_1], leds1[p2_1_2], leds2[p2_1_3], leds3[p2_2_x], leds4[p2_3_x], leds5[p2_4_x], leds6[p2_5_x], leds7[p2_6_x], leds8[p2_7_x], leds9[0]; //need to define "leds9[]" else cannot compile
const int n1 = p2_1_1, n2 = p2_1_2, n3 = p2_1_3, n4 = p2_2_x, n5 = p2_3_x, n6 = p2_4_x, n7 = p2_5_x, n8 = p2_6_x, n9 = p2_7_x, n10 = 0; //for common code for both sculptures
float vol = 10; //master volume gain 0.0 - 1.0

#else
#error "invalid sculpture ID"
#endif

#define UPDATES_PER_SECOND 100 //speed of light animation

int brightness1, brightness2, brightness3, brightness4, brightness5, brightness6, brightness7; //band 1 to 7 brightness
const int IDLE_MODE = 1, BUTTON_MODE = 2, SLIDER_MODE = 3;
unsigned int playMode = IDLE_MODE; 
bool hasplayModeChanged = true; //for audio track changes
int activeLedState = 0;            //to track led animaton states, e.g. 0 - idle mode, start fade to black 1 - show brightness according to reading, 2 - has completed animations, fade to black and idle
bool isMaxBrightness = false;      //to track idle animation direction
int maxBrightLvl = MAXBRIGHTLVL; //variable max brightness
elapsedMillis bandms;              //multiple use time ellapsed counter
unsigned int band_delay = BAND_DELAY; //speed of fade animation
int readings1[NUMDATA1], readings2[NUMDATA2]; //brightness values translated from UV readings
unsigned int readingsCounter;                 //keeps track of indexing the readings array
unsigned int prevBrightVal, currBrightVal;    //for comparing prev and current values for dimming and brightening
CHSV activeColor1 = cyellow; CHSV activeColor2 = corange; CHSV activeColor3 = cred;

#include "myfunctions.h"

/*--------------------------------------------------------------------------------
Setup
--------------------------------------------------------------------------------*/
void setup()
{
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(sliderPin, INPUT);

  Serial.begin(9600);
  Serial3.begin(9600);

  Serial.println();
  Serial.println(F("DFRobot DFPlayer Mini Demo"));
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));
  
  if (!myDFPlayer.begin(Serial3)) {  //Use Serial3 to communicate with mp3.
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card or USB drive!"));
    while(true){
      delay(0); // Code to compatible with ESP8266 watch dog.
    }
  }
  myDFPlayer.volume(vol);
  Serial.println(F("DFPlayer Mini online."));

  delay(2000); //power up safety delay

  if (SCULPTURE_ID == 1)
  {
    FastLED.addLeds<LED_TYPE, LEDPIN0, COLOR_ORDER>(leds0, p1_1_x);
    FastLED.addLeds<LED_TYPE, LEDPIN1, COLOR_ORDER>(leds1, p1_2_x);
    FastLED.addLeds<LED_TYPE, LEDPIN2, COLOR_ORDER>(leds2, p1_3_x);
    FastLED.addLeds<LED_TYPE, LEDPIN3, COLOR_ORDER>(leds3, p1_4_1);
    FastLED.addLeds<LED_TYPE, LEDPIN4, COLOR_ORDER>(leds4, p1_4_2);
    FastLED.addLeds<LED_TYPE, LEDPIN5, COLOR_ORDER>(leds5, p1_5_1);
    FastLED.addLeds<LED_TYPE, LEDPIN6, COLOR_ORDER>(leds6, p1_5_2);
    FastLED.addLeds<LED_TYPE, LEDPIN7, COLOR_ORDER>(leds7, p1_6_1);
    FastLED.addLeds<LED_TYPE, LEDPIN8, COLOR_ORDER>(leds8, p1_6_2);
    FastLED.addLeds<LED_TYPE, LEDPIN9, COLOR_ORDER>(leds9, p1_6_3);
  }
  else if (SCULPTURE_ID == 2)   
  {
    FastLED.addLeds<LED_TYPE, LEDPIN0, COLOR_ORDER>(leds0, p2_1_1);
    FastLED.addLeds<LED_TYPE, LEDPIN1, COLOR_ORDER>(leds1, p2_1_2);
    FastLED.addLeds<LED_TYPE, LEDPIN2, COLOR_ORDER>(leds2, p2_1_3);
    FastLED.addLeds<LED_TYPE, LEDPIN3, COLOR_ORDER>(leds3, p2_2_x);
    FastLED.addLeds<LED_TYPE, LEDPIN4, COLOR_ORDER>(leds4, p2_3_x);
    FastLED.addLeds<LED_TYPE, LEDPIN5, COLOR_ORDER>(leds5, p2_4_x);
    FastLED.addLeds<LED_TYPE, LEDPIN6, COLOR_ORDER>(leds6, p2_5_x);
    FastLED.addLeds<LED_TYPE, LEDPIN7, COLOR_ORDER>(leds7, p2_6_x);
    FastLED.addLeds<LED_TYPE, LEDPIN8, COLOR_ORDER>(leds8, p2_7_x);
    FastLED.addLeds<LED_TYPE, LEDPIN9, COLOR_ORDER>(leds9, 0); //need to define, else cannot compile
  }

  FastLED.setBrightness(255);

  delay(10);

  register_readings(); //translate the ann or sohsuang readings into a readings[] brightness value array.  
}

/*--------------------------------------------------------------------------------
  Loop
--------------------------------------------------------------------------------*/
void loop()
{
  // read_console(); //listen to buttons and sliders

  check_playMode();

  if (playMode == IDLE_MODE)
  {
    if (SCULPTURE_ID == 1) sixband_fade_animation();
    else sevenband_fade_animation();
  }
  else if (playMode == BUTTON_MODE)
  {
    playback_readings(); //play brightness sequence according to readings[] array
  }
  else if (playMode == SLIDER_MODE)
  {
    toggle_readings(); //toggle according to the slider
  }

  FastLED.show();
  FastLED.delay(1000 / UPDATES_PER_SECOND);

  play_audio(); 
}
