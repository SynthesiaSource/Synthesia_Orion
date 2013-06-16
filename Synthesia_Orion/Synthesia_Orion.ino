#include "PinChangeInt.h"
//#include <stdio.h>
#include "LPD8806.h"
#include "SPI.h"


/*
 This is the base source of the Synthesia Portable LED strip driver.
 The Synthesia driver is a Atmel 32U4 with a modified Arduino Leonardo bootloader.
 It is designed for high current operation for driving a large number of pixels (up to 128).
 128 pixels is the maximum due to voltage drop on long lengths of LPD8806 LED strip and available battery life.
 These modes are programmed for non-locking operation, they allow the program to return to the main loop often for mode switching.

 Written by Vlad Lavrovsky
 This project is supported by Synthesia Corporation
 www.synthesia.ca
 
 Release under the GPL Licence 
 
 Copyright (C) 2013 Vlad Lavrovsky

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 Inspired by code developed by Adafruit Industries. www.adafruit.com Distributed under the BSD license
 see license.txt for more information.  This paragraph must be included
 in any redistribution.

 How to program modes:
 Modes are all loops which set each individual pixel of the LED strips. The pixels can be set withot changing their state (strip.setPixelColor()). They change color only upon the strip being refreshed (strip.show()).
 There is limited RAM available so keep frame buffers small.

 Key methods:
 strip.numPixels()            Returns the total number of pixels in the strip. Alternatively, use numberPixels.
 strip.Color(r, g, b)         Returns a uint32_t variable for the specified r,g,b combination
 strip.setPixelColor(i, c)    Sets the pixel at position i to the color c (a uint32_t). 
 strip.show()                 Refreshes the pixels. All LEDs are updated. To maximize performance, limit this call.
 delay(x)                     Delay the program for x number of milliseconds. Used to calibrate speed of modes.
 globalSpeed                  This is a universal speed used in the delay(x) calls within the animations.
 animationStep                A variable constrained to the range 0-384. Use this to animate your modes. Each mode must control its use of animationStep
 frameStep                    Tracks the frame position 0-PIXEL_COUNT. Uses to retain frame position between frame draws.
*/

// Current draw per meter (32 pixels) at 100%, 50%, 25% brightness
// Rainbow Mode 200mA / 90mA / 45 mA
// Full White 500mA / 250mA / 125mA

#define OFF   0x0
#define ON    0x1

// User defined option
#define NUMBER_SPEED_SETTINGS 10
#define NUMBER_BRIGHTNESS_LEVELS 5
#define PIXEL_COUNT 32

// Pins for the RGB status led.  LOW is on, HIGH is off.
#define PIN_LED_RED   2 // Red
#define PIN_LED_GREEN 0 // Green
#define PIN_LED_BLUE  1 // Blue

#define PIN_BUTTON_MODE  8  //
#define PIN_BUTTON_POWER 9  //
#define PIN_BUTTON_SPEED 10 //

#define PIN_STRIP_ENABLE 3 // This pin allows power to flow to the LED strip.
#define SCK_ENABLE 9      // SPI clock enable
#define MOSI_ENABLE 10    // SPI data enable

#define PIN_V_SENSE_ENABLE A1 //
#define PIN_V_SENSE         5 //

#define PIN_USB_CHARGE_ENABLE 17
#define PIN_USB_CHARGE_HIGH  11

#define PI 3.14159265
#define dist(a, b, c, d) sqrt(double((a - c) * (a - c) + (b - d) * (b - d)))

// Gamma correction code and values developed by Jason Clark https://github.com/elmerfud
// Gamma correction compensates for our eyes' nonlinear perception of
// intensity.  It's the LAST step before a pixel value is stored, and
// allows intermediate rendering/processing to occur in linear space.
// The table contains 256 elements (8 bit input), though the outputs are
// only 7 bits (0 to 127).  This is normal and intentional by design: it
// allows all the rendering code to operate in the more familiar unsigned
// 8-bit colorspace (used in a lot of existing graphics code), and better
// preserves accuracy where repeated color blending operations occur.
// Only the final end product is converted to 7 bits, the native format
// for the LPD8806 LED driver.  Gamma correction and 7-bit decimation
// thus occur in a single operation.
PROGMEM prog_uchar gammaTable[]  = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,
    2,  2,  2,  2,  2,  3,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,
    4,  4,  4,  4,  5,  5,  5,  5,  5,  6,  6,  6,  6,  6,  7,  7,
    7,  7,  7,  8,  8,  8,  8,  9,  9,  9,  9, 10, 10, 10, 10, 11,
   11, 11, 12, 12, 12, 13, 13, 13, 13, 14, 14, 14, 15, 15, 16, 16,
   16, 17, 17, 17, 18, 18, 18, 19, 19, 20, 20, 21, 21, 21, 22, 22,
   23, 23, 24, 24, 24, 25, 25, 26, 26, 27, 27, 28, 28, 29, 29, 30,
   30, 31, 32, 32, 33, 33, 34, 34, 35, 35, 36, 37, 37, 38, 38, 39,
   40, 40, 41, 41, 42, 43, 43, 44, 45, 45, 46, 47, 47, 48, 49, 50,
   50, 51, 52, 52, 53, 54, 55, 55, 56, 57, 58, 58, 59, 60, 61, 62,
   62, 63, 64, 65, 66, 67, 67, 68, 69, 70, 71, 72, 73, 74, 74, 75,
   76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91,
   92, 93, 94, 95, 96, 97, 98, 99,100,101,102,104,105,106,107,108,
  109,110,111,113,114,115,116,117,118,120,121,122,123,125,126,127
};


// This function (which actually gets 'inlined' anywhere it's called)
// exists so that gammaTable can reside out of the way down here in the
// utility code...didn't want that huge table distracting or intimidating
// folks before even getting into the real substance of the program, and
// the compiler permits forward references to functions but not data.
inline byte gamma(byte x) {
  return pgm_read_byte(&gammaTable[(x*2)+2]);
}

byte stripBufferA[PIXEL_COUNT];
byte stripBufferB[PIXEL_COUNT];

uint32_t pixelBuffer[PIXEL_COUNT];

volatile boolean poweredOn = false;  
volatile int animationStep = 0;          // Used for incrementing animations (0-384)
volatile int frameStep = 0;              // Used to increment frame counts.
volatile int stateCount = 0;             // System mode
volatile int globalSpeed = 0;            // System animation speed control
volatile int globalBrightness = 1;       // System brightness control

// This is used to calibrate the speed range for different modes
// Slow modes require a low frameDelayTimer (1-5). Fast modes require a high frameDelayTimer (5+).
int frameDelayTimer = 5;
long previousMillis = 0;

// Variables used by modes

// Cylon mode
int pos = 0;
int dir = 0;

// Breathing rainbow mode
int shifter = 0;

// Used to store a current color for modes which cycle through colors.
uint32_t currentColor;

uint8_t latest_interrupted_pin;
uint8_t interrupt_count[20]={0}; // 20 possible arduino pins

void quicfunc() {
  latest_interrupted_pin=PCintPort::arduinoPin;
  interrupt_count[latest_interrupted_pin]++;
};

// You can assign any number of functions to any number of pins.
// How cool is that?
void pinPowerFunc() {

  if(poweredOn)
    {
    digitalWrite(PIN_STRIP_ENABLE, HIGH);
    poweredOn = false;
    } else {
    digitalWrite(PIN_STRIP_ENABLE, LOW);   
    poweredOn = true;   
    }
}

void pinModeFunc() 
{
//  semaphoreMode = true;
   stateCount++;
   
   if(stateCount>12)  // If modes are added this number must be increased.
     stateCount = 0;
     
//  globalSpeed = 1;
  frameStep = 0;
  animationStep = 0;
}

void pinSpeedFunc() 
{
//  semaphoreSpeed = true;
   globalSpeed+=1;
   
   if(globalSpeed > NUMBER_SPEED_SETTINGS)  // If modes are added this number must be increased.
     globalSpeed = 0;

}

void pinBrightnessFunc() 
{
//  semaphoreSpeed = true;
   // globalBrightness == 1 is the highest brightness
   globalBrightness+=1;
   
   if(globalBrightness > NUMBER_BRIGHTNESS_LEVELS)  // If modes are added this number must be increased.
     globalBrightness = 1;

  // The range of brightnesses is always the same. Only the number of steps is variable.
  // Linear conversion function
  //  new_value = ( (old_value - old_min) / (old_max - old_min) ) * (new_max - new_min) + new_min
  
//  float scaledBrightness = ((float) globalBrightness / ((float)NUMBER_BRIGHTNESS_LEVELS-1)) * (4.0-1.0) + 1.0;
//  brightnessFraction = 1.25-0.25*(float)globalBrightness;

}

// Available modes. Additional modes are available at www.synthesia.ca
void solidColor();
void rainbowCycle(uint16_t wait);                 // Standard rainbow mode. Medium drain mode.
void rainbowStrobe(uint16_t wait);                // Steps through the rainbow, pulsing all the way. Medium drain mode.
void rainbowBreathing(uint16_t wait);             // Slow pulsating rainbow. Medium drain mode.
void pulseStrobe(uint32_t c, uint16_t wait);      // Fast flashy strobe. Medium drain mode.
void smoothStrobe(uint32_t c, uint16_t wait);     // Pulses a color on and off. Medium drain mode.
void colorChase(uint32_t c, uint16_t wait);       // Single pixel random color chase. Low drain mode.
void colorWipe(uint32_t c, uint16_t wait);        // Random color fill. Medium drain mode.
void dither(uint32_t c, uint16_t wait);           // Random multi-color dither. Medium drain mode.
void scanner(uint32_t c, uint16_t wait);          // Bounced a 5 pixel wide color band across the strip.
void wave(uint32_t c, uint16_t wait);             // Sine wave color ranges from full white to c. Random colors. High drain mode.
void randomSparkle(uint16_t wait);                // Sparkles with random colors at random points. Medium drain mode.

byte gamma(byte x);
uint32_t Wheel(uint16_t WheelPos);
uint32_t dampenBrightness(uint32_t c, int brightness);

// Set numberPixels to the total number of LEDs in your strip
// The LED strips are 32 LEDs per meter and can be cut or extended in units of 2 LEDs at the cut lines
// The driver can handle up to 128 pixels. Battery life is proportional to the number of pixels used. 
// All mode battery endurance values are based on a 1 meter (32 pixel) length.
// Thus 2 meters (64) halves the endurance and 4 meters (128 pixels) cuts it to one quarter.

// Change this variable to match the number of pixels in your setup
// If numberPixels is less than the total LEDs connected, some LEDs will go unlit
// If numberPixels is greater than the total LEDs connected, you will get lower performance than if it exactly matches.
const int numberPixels = PIXEL_COUNT;

LPD8806 strip = LPD8806(PIXEL_COUNT);
#define PI 3.14159265

void setup() {
  
    // Turn off all the LED's, strip power and voltage sensing.
  digitalWrite(PIN_LED_RED  , HIGH);
  digitalWrite(PIN_LED_GREEN, HIGH);
  digitalWrite(PIN_LED_BLUE , HIGH);
  digitalWrite(PIN_STRIP_ENABLE  , HIGH);
  digitalWrite(PIN_V_SENSE_ENABLE, HIGH);

  // Set all pin directions.
  pinMode(PIN_LED_RED  , OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_BLUE , OUTPUT);
  pinMode(PIN_BUTTON_MODE , INPUT);
  pinMode(PIN_BUTTON_POWER, INPUT);
  pinMode(PIN_BUTTON_SPEED, INPUT);
  pinMode(PIN_STRIP_ENABLE, OUTPUT);
  pinMode(PIN_V_SENSE_ENABLE, OUTPUT);
  pinMode(PIN_USB_CHARGE_HIGH, OUTPUT);
  pinMode(PIN_USB_CHARGE_ENABLE, OUTPUT);
  
  // Enable the internal pullups on the switch pins.
  digitalWrite(PIN_BUTTON_MODE , HIGH);
  digitalWrite(PIN_BUTTON_POWER, HIGH);
  digitalWrite(PIN_BUTTON_SPEED, HIGH);
  digitalWrite(PIN_STRIP_ENABLE, HIGH);
  
  // Make the ADC use the nternal 2.56v reference.
  analogReference(INTERNAL);

  // Default to 450mA charge current and charging enabled.
  digitalWrite(PIN_USB_CHARGE_HIGH, HIGH);
  digitalWrite(PIN_USB_CHARGE_ENABLE, HIGH);

  // Attach button interupts
  PCintPort::attachInterrupt(PIN_BUTTON_SPEED, &pinSpeedFunc, FALLING);  // add more attachInterrupt code as required
  PCintPort::attachInterrupt(PIN_BUTTON_MODE, &pinModeFunc, FALLING);  // add more attachInterrupt code as required
  PCintPort::attachInterrupt(PIN_BUTTON_POWER, &pinPowerFunc, FALLING);
  //PCintPort::attachInterrupt(PIN_BUTTON_BRIGHTNESS &pinBrightnessFunc, FALLING);  // add more attachInterrupt code as required

//  attachInterrupt(PIN_BUTTON_MODE, switchModes, CHANGE);
//  attachInterrupt(PIN_BUTTON_SPEED, switchSpeed, CHANGE);
//  attachInterrupt(PIN_BUTTON_POWER, switchPower, CHANGE);

    // initialize Timer1 for use in controlling battery status update
    cli();          // disable global interrupts
    TCCR1A = 0;     // set entire TCCR1A register to 0
    TCCR1B = 0;     // same for TCCR1B
 
    // set compare match register to desired timer count:
    OCR1A = 3624;
    // turn on CTC mode:
    TCCR1B |= (1 << WGM12);
    // Set CS10 and CS12 bits for 1024 prescaler:
    TCCR1B |= (1 << CS10);
    TCCR1B |= (1 << CS12);
    // enable timer compare interrupt:
    TIMSK1 |= (1 << OCIE1A);
    // enable global interrupts:
    sei();

  for(int u = 0; u<PIXEL_COUNT; u++)
    pixelBuffer[u] = 0;
  
  // globalSpeed controls the delays in the animations. Starts low. Range is 1-5. 
  // Each animation is responsible for calibrating its own speed relative to the globalSpeed.
  globalSpeed = 0;

  // Boot up the strip with power off
  // Does not work without a battery if boots on LOW (on)
  digitalWrite(PIN_STRIP_ENABLE, HIGH);   
  poweredOn = false;   
  
    // Start up the LED strip
  strip.begin();

  // Update the strip
  strip.show();


}

ISR(TIMER1_COMPA_vect)
{
  batteryStatus();
}

// All animations are controlled by a delay method. Range of delay is 0-5;
// All animations must be totally non-locking. That is, draw only one frame at a time.
void loop() {

  // These calls might be what is slowing down the driver.
  digitalWrite(PIN_LED_BLUE , digitalRead(PIN_BUTTON_SPEED));
  digitalWrite(PIN_LED_RED  , digitalRead(PIN_BUTTON_MODE));
  digitalWrite(PIN_LED_GREEN, digitalRead(PIN_BUTTON_POWER));
  
  // Test for recent button press and increment mode select  
  
    
  boolean pushFrame = true; 
  unsigned long currentMillis = millis();

    if(currentMillis - previousMillis > frameDelayTimer*globalSpeed)
      {      
      previousMillis = currentMillis;
      pushFrame = true;
      } else {
      pushFrame = false;
      }

    if(pushFrame)
      {
      // Go to the current mode
      switch(stateCount) 
        {
          case 0:
          {
            // Smooth rainbow animation.
            rainbow(); 
            frameDelayTimer = 10;
            break;
          }
          case 1:
          {
            rainbowBreathing(10);
            frameDelayTimer = 1;
            break;
          }
          case 2:
          {
            plasma();
            frameDelayTimer = 20;    
            break;
          }
          case 3:
          {
            splitColorBuilder();
            frameDelayTimer = 5;
            break;
          }          
          case 4:
          {
            smoothColors();
            frameDelayTimer = 5;
            break;
          }          
          case 5:
          {
            // Single pixel random color pixel chase.
            if(animationStep == 0)
            {
              currentColor =Wheel(random(0, 384));
            }
            colorChase(currentColor, globalSpeed);
            frameDelayTimer = 5;    
            break;
          }          
          case 6:
          {
            // Random color wipe.
            if(frameStep == 0)
            {
              currentColor =Wheel(random(0, 384));
            }
            colorWipe(currentColor, globalSpeed);
            frameDelayTimer = 5;    
            break;
          }          
          case 7:
          {
            // Random color dither. This is a color to color dither (does not clear between colors).
            long randNumber = random(0, 384);
            uint32_t c = Wheel(((randNumber * 384 / strip.numPixels()) + randNumber) % 384);
            dither(c, globalSpeed);  
            frameDelayTimer = 8;    
            break;
          }          
          case 8:
          {
            if(animationStep == 0)
            {
              currentColor =Wheel(random(0, 384));
            }
            scanner(currentColor, globalSpeed);  
            frameDelayTimer = 5;    
            break;
          }
          case 9:
          {
            // Sin wave effect. New color every cycle.
            if(animationStep == 0)
            {
              currentColor =Wheel(random(0, 384));
            }
            wave(currentColor, globalSpeed);  
            frameDelayTimer = 5;    
            break;
          }
          case 10:
          {
            // Random color noise animation.
            randomSparkle(globalSpeed);
            frameDelayTimer = 3;    
            break;
          }
          case 11:
          {
            // Color fade-in fade-out effect
            if(animationStep==0)
              currentColor =Wheel(random(0, 384));
            if(animationStep<192)
            {
              fadeIn(currentColor, 10); 
            } else {
              fadeOut(currentColor, 10);
            }

            frameDelayTimer = 1;
            break;
          }
          case 12:
          {
            sparkler();
            frameDelayTimer = 10;
            break;
          }
          default: break;
        }
        
        // Increment animation and frame positions
        animationStep++;
        frameStep++;
      }  
  
  // Global animation frame limit of 384 (for full color wheel range).
  // Large animationSteps slow down the driver.
  if(animationStep>384)
    animationStep = 0;
    
  // Ensure that only as many pixels are drawn as there are in the strip.  
  if(frameStep > PIXEL_COUNT)
    frameStep = 0;
    

}


void solidColor()
{
    for (int i=0; i < strip.numPixels(); i++) {
     strip.setPixelColor(i, strip.Color(127, 127, 127));
    }
    strip.show();

}

void plasma() {
  
    double time = animationStep;
            
    for(int y = 0; y < PIXEL_COUNT; y++)
    {
        double value = sin(dist(frameStep + time, y, 64.0, 64.0) / 4.0)
                   + sin(dist(frameStep, y, 32.0, 32.0) / 4.0);
                   + sin(dist(frameStep, y + time / 7, 95.0, 32) / 3.5);
                   + sin(dist(frameStep, y, 95.0, 50.0) / 4.0);
  
      int color = int((4 + value)*384)%384;
      strip.setPixelColor(y, Wheel(color)); 
    }    
  strip.show();
}

void sparkler() {
  
  stripBufferA[random(0,PIXEL_COUNT)] = random(0, 255);

  for(int x = 0; x < PIXEL_COUNT; x++) 
    {
      byte newPoint = (stripBufferA[x] + stripBufferA[x+1]) / 2 - 15;
      stripBufferB[x] = newPoint;
      if(newPoint>50)
         strip.setPixelColor(x, Wheel(((newPoint/5)+animationStep)%384));      
      if(newPoint<50)
         strip.setPixelColor(x, strip.Color(0,0,0)); 
    }
   
   strip.show();
    
    for(int x = 0; x < PIXEL_COUNT; x++) 
    {
      stripBufferA[x] = stripBufferB[x];
   }
  
}


void rainbowBreathing(uint16_t wait)
{
  uint16_t i, j;
  if(animationStep<192)
  {
  float modifier = 0.25+0.004*(float)animationStep;

    for (i=0; i < strip.numPixels(); i++) 
      {
        
      uint32_t c =  Wheel(((i * 384 / PIXEL_COUNT)) % 384);
 
      byte  r, g, b;
      
      // Need to decompose color into its r, g, b elements
      g = (c >> 16) & 0x7f;
      r = (c >>  8) & 0x7f;
      b =  c        & 0x7f; 
     
      strip.setPixelColor(i, r*modifier, g*modifier, b*modifier);
//      strip.setPixelColor(i, gamma(r*modifier), gamma(g*modifier), gamma(b*modifier));
    }
  strip.show();   // write all the pixels out
  } else {
  float modifier = 1.75-0.004*(float)animationStep;

    for (i=0; i < strip.numPixels(); i++) 
      {
        
      uint32_t c =  Wheel(((i * 384 / PIXEL_COUNT)+shifter) % 384);
 
      byte  r, g, b;
      
      // Need to decompose color into its r, g, b elements
      g = (c >> 16) & 0x7f;
      r = (c >>  8) & 0x7f;
      b =  c        & 0x7f; 
     
      strip.setPixelColor(i, r*modifier, g*modifier, b*modifier);
//      strip.setPixelColor(i, gamma(r*modifier), gamma(g*modifier), gamma(b*modifier));
    }
  strip.show();   // write all the pixels out   
  }
} 
  
  
void rainbow() {
  uint16_t i, j;
  int pixelCount = strip.numPixels();

      for (i=0; i < strip.numPixels(); i++) 
      {
        //strip.setPixelColor(i, Wheel(((i * 384 / pixelCount) + animationStep) % 384));
        setPixelAtBrightness(i, Wheel(((i * 384 / pixelCount) + animationStep) % 384));
      }
      strip.show();   // write all the pixels out
}

void splitColorBuilder() {
  uint16_t i, j;
  int pixelCount = strip.numPixels();
//  pixelBuffer[0] = Wheel((sin(animationStep)+1)*384);
//  pixelBuffer[1] = Wheel((sin(animationStep)+1)*384);
  
  uint32_t c = Wheel(animationStep);
  float y = (sin(PI * 1 * (float)(animationStep) / (float)strip.numPixels()/4))+1;
  byte  r, g, b, r2, g2, b2;

  // Need to decompose color into its r, g, b elements
  g = (c >> 16) & 0x7f;
  r = (c >>  8) & 0x7f;
  b =  c        & 0x7f; 
  
  r2 = 127 - (byte)((float)(127 - r) * y);
  g2 = 127 - (byte)((float)(127 - g) * y);
  b2 = 127 - (byte)((float)(127 - b) * y);
  
  pixelBuffer[0] = strip.Color(r2, g2, b2);

  for (i=0; i < strip.numPixels()/2; i++) 
  {
    setPixelAtBrightness(PIXEL_COUNT/2-i, pixelBuffer[i]);
    setPixelAtBrightness(PIXEL_COUNT/2+i, pixelBuffer[i]); 
  }
  
  strip.show();   // write all the pixels out
  
  for(int k = PIXEL_COUNT-1; k>0; k--)
  {
   pixelBuffer[k] = pixelBuffer[k-1]; 
  }


}

//void splitColorBuilder() {
//  uint16_t i, j;
//  int pixelCount = strip.numPixels();
////  pixelBuffer[0] = Wheel((sin(animationStep)+1)*384);
////  pixelBuffer[1] = Wheel((sin(animationStep)+1)*384);
//  
//  uint32_t c = Wheel(animationStep);
//  float y = (sin(PI * 1 * (float)(animationStep) / (float)strip.numPixels()/4))+1;
//  byte  r, g, b, r2, g2, b2;
//
//  // Need to decompose color into its r, g, b elements
//  g = (c >> 16) & 0x7f;
//  r = (c >>  8) & 0x7f;
//  b =  c        & 0x7f; 
//  
//  r2 = 127 - (byte)((float)(127 - r) * y);
//  g2 = 127 - (byte)((float)(127 - g) * y);
//  b2 = 127 - (byte)((float)(127 - b) * y);
//  
//  pixelBuffer[0] = strip.Color(r2, g2, b2);
//
//  for (i=0; i < strip.numPixels()/2; i++) 
//  {
//    setPixelAtBrightness(PIXEL_COUNT/2-i, pixelBuffer[i]);
//    setPixelAtBrightness(PIXEL_COUNT/2+i, pixelBuffer[i]); 
//  }
//  
//  strip.show();   // write all the pixels out
//  
//  for(int k = PIXEL_COUNT-1; k>0; k--)
//  {
//   pixelBuffer[k] = pixelBuffer[k-1]; 
//  }
//
//
//}

void smoothColors() {
  uint16_t i, j;
  int pixelCount = strip.numPixels();
  uint32_t c = Wheel(((i * 384 / pixelCount) + animationStep) % 384);
  pixelBuffer[0] = c;
  
      for (i=0; i < strip.numPixels(); i++) 
      {
        //strip.setPixelColor(i, Wheel(((i * 384 / pixelCount) + animationStep) % 384));
        setPixelAtBrightness(i, c);
        
      }
      strip.show();   // write all the pixels out
}

void fadeOut(uint32_t c, uint16_t wait)
{  

  byte  r, g, b, r2, g2, b2, r8, g8,b8;
  
  // Need to decompose color into its r, g, b elements
  g = (c >> 16) & 0x7f;
  r = (c >>  8) & 0x7f;
  b =  c        & 0x7f; 
 
  r8 = r;
  g8 = g;
  b8 = b;

  byte highColorByte = 0;
  if(g8>highColorByte)
  {highColorByte= g8; }  
  if(r8>highColorByte)
  {highColorByte= r8; }
  if(b8>highColorByte)
  {highColorByte= b8; }  

  byte lowColorByte = 255;
  if(g8 < lowColorByte)
  {lowColorByte= g8; }  
  if(r8 < lowColorByte)
  {lowColorByte= r8; }
  if(b8 < lowColorByte)
  {lowColorByte= b8; }   
  
  int y = (float)highColorByte-(float)highColorByte*(float)(2-0.005*animationStep);

//  int y = (float)highColorByte-(float)highColorByte*(2.0-(0.005*(float)animationStep));

      if(g8>y)
      {
        g2 =  gamma(g8-y);
      } else {
        g2 = gamma(0); 
      }
      if(r8>y)
      {
        r2 =  gamma(r8-y);
      } else {
        r2 = gamma(0); 
      }
      if(b8>y)
      {
        b2 =  gamma(b8-y);
      } else {
        b2 = gamma(0); 
      }
      
    for (int i=0; i < strip.numPixels(); i++) 
    {
      strip.setPixelColor(i, r2, g2, b2);
    }
    
    strip.show();   // write all the pixels out
}

void fadeIn(uint32_t c, uint16_t wait)
{
  byte  r, g, b, r2, g2, b2, r8, g8,b8;
  
  // Need to decompose color into its r, g, b elements
  g = (c >> 16) & 0x7f;
  r = (c >>  8) & 0x7f;
  b =  c        & 0x7f; 
 
  r8 = r;
  g8 = g;
  b8 = b;

  byte highColorByte = 0;
  if(g8>highColorByte)
  {highColorByte= g8; }  
  if(r8>highColorByte)
  {highColorByte= r8; }
  if(b8>highColorByte)
  {highColorByte= b8; }  

  byte lowColorByte = 255;
  if(g8 < lowColorByte)
  {lowColorByte= g8; }  
  if(r8 < lowColorByte)
  {lowColorByte= r8; }
  if(b8 < lowColorByte)
  {lowColorByte= b8; }   

  int y = (float)highColorByte - (float)highColorByte*((float)animationStep/192);

      if(g8>y)
      {
        g2 =  gamma(g8-y);
      } else {
        g2 = gamma(0); 
      }
      if(r8>y)
      {
        r2 =  gamma(r8-y);
      } else {
        r2 = gamma(0); 
      }
      if(b8>y)
      {
        b2 =  gamma(b8-y);
      } else {
        b2 = gamma(0); 
      }
    for (int i=0; i < strip.numPixels(); i++) 
    {
      strip.setPixelColor(i, r2, g2, b2);
    }
    
    strip.show();   // write all the pixels out
}


void pulseStrobe(uint32_t c, uint16_t wait)
{
    for (int i=0; i < strip.numPixels(); i++) 
    {
      if(animationStep%2)
        {
        strip.setPixelColor(i, dampenBrightness(c, animationStep));
        } else {
        strip.setPixelColor(i, dampenBrightness(c, 10)); 
        }
    
    strip.show();   // write all the pixels out  
    }
}


// Cycle through the color wheel, equally spaced around the belt
void rainbowCycle(uint16_t wait) {
  uint16_t i, j;
  for (i=0; i < strip.numPixels(); i++) 
  {
    strip.setPixelColor(i, Wheel(((i * 384 / strip.numPixels()) + animationStep) % 384));
  }
  strip.show();   // write all the pixels out
  delay(wait);
  animationStep++;
}

// fill the dots one after the other with said color
// good for testing purposes
void colorWipe(uint32_t c, uint16_t wait) {
  int i;
 
  for (i=0; i < frameStep; i++) 
    {
    setPixelAtBrightness(i, currentColor);
    }
   
    strip.show(); 
}


// Chase a dot down the strip
// Random color for each chase
void colorChase(uint32_t c, uint16_t wait) {
  int i;

    setPixelAtBrightness(frameStep-1, 0); // Erase pixel, but don't refresh!
    setPixelAtBrightness(frameStep, c); // Set new pixel 'on'
    strip.show(); // Refresh LED states
}


// An "ordered dither" fills every pixel in a sequence that looks
// sparkly and almost random, but actually follows a specific order.
void dither(uint32_t c, uint16_t wait) {
 
  // Determine highest bit needed to represent pixel index
  int hiBit = 0;
  int n = strip.numPixels() - 1;
  for(int bit=1; bit < 0x8000; bit <<= 1) {
    if(n & bit) hiBit = bit;
  }

  int bit, reverse;
  for(int i=0; i<(hiBit << 1); i++) {
    // Reverse the bits in i to create ordered dither:
    reverse = 0;
    for(bit=1; bit <= hiBit; bit <<= 1) {
      reverse <<= 1;
      if(i & bit) reverse |= 1;
    }
    strip.setPixelColor(reverse, c);
    strip.show();
    delay(wait);
  }
  delay(wait);  
}


void randomSparkle(uint16_t wait) {

  long randNumber;
  
  // Determine highest bit needed to represent pixel index
  uint16_t i, j;

  randNumber = random(0, strip.numPixels()-1);
  strip.setPixelColor(randNumber, Wheel(((frameStep * 384 / strip.numPixels()) + animationStep) % 384));
  strip.show();


}


// "Larson scanner" = Cylon/KITT bouncing light effect
// Draws two sets of frames per call to double the resolution range
void scanner(uint32_t c, uint16_t wait) {

  byte  r, g, b;
  
  // Decompose color into its r, g, b elements
  g = (c >> 16) & 0x7f;
  r = (c >>  8) & 0x7f;
  b =  c        & 0x7f; 
  
  int i, j, pos, dir;

  if(frameStep == 0)
  {
    pos = 0;
    dir = 2;
  }
  
  pos = frameStep*2;
  
   if(pos >= strip.numPixels()) 
   {
     pos = 32-((frameStep-16)*2);
    }
  
 // pos = frameStep;
    // Draw 5 pixels centered on pos.  setPixelColor() will clip
    // any pixels off the ends of the strip, no worries there.
    // we'll make the colors dimmer at the edges for a nice pulse
    // look
    strip.setPixelColor(pos - 2, strip.Color(r/4, g/4, b/4));
    strip.setPixelColor(pos - 1, strip.Color(r/2, g/2, b/2));
    strip.setPixelColor(pos, strip.Color(r, g, b));
    strip.setPixelColor(pos + 1, strip.Color(r/2, g/2, b/2));
    strip.setPixelColor(pos + 2, strip.Color(r/4, g/4, b/4));

    strip.show();
    // If we wanted to be sneaky we could erase just the tail end
    // pixel, but it's much easier just to erase the whole thing
    // and draw a new one next time.
    for(j=-2; j<= 2; j++) 
        strip.setPixelColor(pos+j, strip.Color(0,0,0));
        
    strip.setPixelColor(pos - 1, strip.Color(r/4, g/4, b/4));
    strip.setPixelColor(pos - 0, strip.Color(r/2, g/2, b/2));
    strip.setPixelColor(pos+1, strip.Color(r, g, b));
    strip.setPixelColor(pos + 2, strip.Color(r/2, g/2, b/2));
    strip.setPixelColor(pos + 3, strip.Color(r/4, g/4, b/4));

    strip.show();
    // If we wanted to be sneaky we could erase just the tail end
    // pixel, but it's much easier just to erase the whole thing
    // and draw a new one next time.
    for(j=-1; j<= 3; j++) 
        strip.setPixelColor(pos+j, strip.Color(0,0,0));
}

// Sine wave effect.
// Self calibrating for pixel run length.
void wave(uint32_t c, uint16_t wait) {
  float y;
  byte  r, g, b, r2, g2, b2;

  // Need to decompose color into its r, g, b elements
  g = (c >> 16) & 0x7f;
  r = (c >>  8) & 0x7f;
  b =  c        & 0x7f; 

    for(int i=0; i<strip.numPixels(); i++) 
    {
      y = sin(PI * 1 * (float)(animationStep + i) / (float)strip.numPixels());
      if(y >= 0.0) {
        // Peaks of sine wave are white
        y  = 1.0 - y; // Translate Y to 0.0 (top) to 1.0 (center)
        r2 = 127 - (byte)((float)(127 - r) * y);
        g2 = 127 - (byte)((float)(127 - g) * y);
        b2 = 127 - (byte)((float)(127 - b) * y);
      } else {
        // Troughs of sine wave are black
        y += 1.0; // Translate Y to 0.0 (bottom) to 1.0 (center)
        r2 = (byte)((float)r * y);
        g2 = (byte)((float)g * y);
        b2 = (byte)((float)b * y);
      }
      setPixelAtBrightness(i, strip.Color(r2, g2, b2));
//      strip.setPixelColor(i, r2, g2, b2);
    }
    strip.show();
}


//Input a value 0 to 384 to get a color value.
//The colours are a transition r - g - b - back to r

uint32_t Wheel(uint16_t WheelPos)
{
  byte r, g, b;
  switch(WheelPos / 128)
  {
    case 0:
      r = 127 - WheelPos % 128; // red down
      g = WheelPos % 128;       // green up
      b = 0;                    // blue off
      break;
    case 1:
      g = 127 - WheelPos % 128; // green down
      b = WheelPos % 128;       // blue up
      r = 0;                    // red off
      break;
    case 2:
      b = 127 - WheelPos % 128; // blue down
      r = WheelPos % 128;       // red up
      g = 0;                    // green off
      break;
  }
  return(strip.Color(r,g,b));
}


uint32_t dampenBrightness(uint32_t c, int brightness) {

 byte  r, g, b;
  
  g = ((c >> 16) & 0x7f)/brightness;
  r = ((c >>  8) & 0x7f)/brightness;
  b = (c        & 0x7f)/brightness; 

  return(strip.Color(r,g,b));

}

void batteryStatus() {
    // Drive the voltage sense divider low, allowing us to read.
  digitalWrite(PIN_V_SENSE_ENABLE, LOW);
  float batteryVoltage = (analogRead(PIN_V_SENSE)*0.005);
  // Now drive the sense divider high.  This saves power when we're not reading it.
  digitalWrite(PIN_V_SENSE_ENABLE, HIGH);
  
  if(poweredOn)
  {
    if(batteryVoltage < 3.0)
    {
      digitalWrite(PIN_LED_RED , LOW);
    } else 
    {
      if(batteryVoltage < 3.5)
      {
        digitalWrite(PIN_LED_BLUE , LOW);
      } else 
        {
          digitalWrite(PIN_LED_GREEN , LOW);
        }
    }
  
  // Fully charged / Charging display 
  if(batteryVoltage >= 4.0)
    {
      digitalWrite(PIN_LED_GREEN , LOW);
      digitalWrite(PIN_LED_RED, LOW);
      digitalWrite(PIN_LED_BLUE , LOW);
    }
  }
  
}

// Set pixel at globalBrightness
void setPixelAtBrightness(int i, uint32_t c)
{
   int  r, g, b;
  
//  g = ((c >> 16) & 0x7f)*(brightnessFraction);
//  r = ((c >>  8) & 0x7f)*(brightnessFraction);
//  b = (c        & 0x7f)*(brightnessFraction); 

  // Extract RGB channels
  g = ((c >> 16) & 0x7f);
  r = ((c >>  8) & 0x7f);
  b = (c        & 0x7f); 
  
   // Fast bitshift calculation
   // Using floating point calulations in this constantly called method
   // results in really low frame rates
   switch(globalBrightness) 
  {
    case 1:
    {
      break;
    }
    case 2:
    {
      g = (g << 1)+g;
      r = (r << 1)+r;
      b = (b << 1)+b;
      
      g >>= 2;
      r >>= 2;
      b >>= 2;   
      break;
    }
    case 3:
    { 
      g >>= 1;
      r >>= 1;
      b >>= 1;
      break;
    }
    case 4:
    {
      g >>= 2;
      r >>= 2;
      b >>= 2;   
      break;
    }
    case 5:
    {
      g >>= 3;
      r >>= 3;
      b >>= 3;   
      break;
    }
     default: break;
  }
  
  strip.setPixelColor(i, r, g, b); 
}




