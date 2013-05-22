#include <LPD8806.h>
#include "SPI.h"

/*
 This is the base source of the Synthesia Portable LED strip driver.
 The Synthesia driver is a Atmel 32U4 with a modified Arduino Leonardo bootloader.
 It is designed for high current operation for driving a large number of pixels (up to 128).
 128 pixels is the maximum due to voltage drop on long lengths of LPD8806 LED strip and available battery life.
 These modes are programmed for non-locking operation, they allow the program to return to the main loop often for mode switching.

 Written by Vlad Lavrovsky
 Release under the MIT Licence Copyright (C) 2013 Synthesia Corporation
 Based on code written by Adafruit Industries.  Distributed under the BSD license --
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
*/
  
volatile int animationStep = 0;          // Used for incrementing animations
volatile int stateCount = 0;             // System mode
volatile int globalSpeed = 0;            // System animation speed control
volatile boolean semaphoreFlagMode = false;  // State flag for the modes
volatile boolean semaphoreFlagSpeed = false;  // State flag for the speed

void switchModes();
void switchSpeed();

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
const int numberPixels = 32;
LPD8806 strip = LPD8806(numberPixels);
#define PI 3.14159265

void setup() {
  
  // globalSpeed controls the delays in the animations. Starts low. Range is 1-5. 
  // Each animation is responsible for calibrating its own speed relative to the globalSpeed.
  globalSpeed = 1;
  
  // Set the switch pins as input
  pinMode(INT1, INPUT); 
  pinMode(INT2, INPUT); 

  attachInterrupt(INT1, switchModes, RISING);
  attachInterrupt(INT1, switchSpeed, RISING);

  // Start up the LED strip
  strip.begin();

  // Update the strip
  strip.show();
}


// All animations are controlled by the delay variable that is passed in. Range of delay is 1-5;
void loop() {

  // Test for recent button press and increment mode select  
  if(semaphoreFlagMode == true)
  {
   delay(100);        // This delay ensures debounce on high speed animations. 50-100 milliseconds is standard.
   stateCount++;
   
   if(stateCount>10)  // If modes are added this number must be increased.
     stateCount = 0;

    semaphoreFlagMode = false; 
    animationStep = 0;  
  }
  
  // Global animation frame limit of 384 (for full color wheel range).
  // Large animationSteps slow down the driver.
  if(animationStep > 384)
  {
   animationStep = 0; 
  }
  
  // Go to the current mode
  switch(stateCount) 
    {
      case 0:
      {
        // Smooth rainbow animation.
        rainbowCycle(globalSpeed); 
        break;
      }
      case 1:
      {
        // Single color strobe through rainbow color range.
        rainbowStrobe(globalSpeed);
        break;
      }
      case 2:
      {
        // Interesting breathing rainbow animation
        rainbowBreathing(globalSpeed);
        break;
      }
      case 3:
      {
        // Pulsing color strobe effect. 
        long randNumber = random(0, 384);
        uint32_t c = Wheel(((randNumber * 384 / strip.numPixels()) + randNumber) % 384);
        pulseStrobe(c, globalSpeed);
        break;
      }
      case 4:
      {
        // Smooth color fade.
        long randNumber = random(0, 384);
        uint32_t c = Wheel(((randNumber * 384 / strip.numPixels()) + randNumber) % 384);
        smoothStrobe(c, globalSpeed);  
        break;
      }
      case 5:
      {
        // Single pixel random color pixel chase.
        long randNumber = random(0, 384);
        uint32_t c = Wheel(((randNumber * 384 / strip.numPixels()) + randNumber) % 384);
        colorChase(c, globalSpeed);
        break;
      }
      case 6:
      {
        // Random color wipe.
        long randNumber = random(0, 384);
        uint32_t c = Wheel(((randNumber * 384 / strip.numPixels()) + randNumber) % 384);
        colorWipe(c, globalSpeed);
        break;
      }
       case 7:
       {
        // Random color dither. This is a color to color dither (does not clear between colors).
        long randNumber = random(0, 384);
        uint32_t c = Wheel(((randNumber * 384 / strip.numPixels()) + randNumber) % 384);
        dither(c, globalSpeed);  
        break;
       }
      case 8:
      {
        long randNumber = random(0, 384);
        uint32_t c = Wheel(((randNumber * 384 / strip.numPixels()) + randNumber) % 384);
        scanner(c, globalSpeed);  
        break;
      }
      case 9:
      {
        // Sin wave effect. New color every cycle.
        long randNumber = random(0, 384);
        uint32_t c = Wheel(((randNumber * 384 / strip.numPixels()) + randNumber) % 384);
        wave(c, globalSpeed);  
        break;
      }
      case 10:
      {
        // Random color noise animation.
        randomSparkle(globalSpeed);
        break;
      }
      default: break;
    }

  delay(globalSpeed);
// Commented out due to changes in animation induced by the introduction of non-locking animations.
//  for (int i=0; i < strip.numPixels(); i++) {
//    strip.setPixelColor(i, 0);
//  }

}

void switchModes()
{
  semaphoreFlagMode = true;   
}


void switchSpeed()
{
  globalSpeed += 1;
  if(globalSpeed>10)
    globalSpeed = 1;
  semaphoreFlagSpeed = false;   
}


void solidColor()
{
    for (int i=0; i < strip.numPixels(); i++) {
     strip.setPixelColor(i, strip.Color(127, 127, 127));
    }
    strip.show();

}


void rainbowStrobe(uint16_t wait)
{
  // Depends on the function smoothStrobe
   uint16_t i, j;

    for (i=0; i < strip.numPixels(); i++) 
    {
      smoothStrobe(Wheel(((i * 384 / strip.numPixels()) + animationStep) % 384), wait);
    }
    animationStep++;
    delay(wait);
}


// Depends on the function smoothStrobe
// Wait increases the delay between refreshes. Steps controls the speed of the flash. The large steps like 5 are very fast. Small steps (1-2) are slow.
// Has a synthetic delay called steps which controls both the incrementing of the flash and the peak hold time.
void rainbowBreathing(uint16_t wait)
{
  uint16_t steps = 10-wait;
  
  // Prevent negative values.
  if(wait>99)
  {
     steps = 1; 
  }
  
  uint16_t i, j;
  uint16_t shiftRand = random(0, 383);
  for(int y=255; y>0; y-=steps)
    {  
    for (i=0; i < strip.numPixels(); i++) 
      {
      uint32_t c = Wheel(((i * 384 / strip.numPixels()) + shiftRand) % 384);
  
      byte  r, g, b, r2, g2, b2, r8, g8,b8;
      
      // Need to decompose color into its r, g, b elements
      g = (c >> 16) & 0x7f;
      r = (c >>  8) & 0x7f;
      b =  c        & 0x7f; 
     
      r8 = r << 1;
      g8 = g << 1;
      b8 = b << 1;   
      
      float rr = r8/255;
      float gr = g8/255;
      float br = b8/255;
    
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

      strip.setPixelColor(i, r2, g2, b2);
    }
  strip.show();   // write all the pixels out
  
    if(semaphoreFlagMode == true)
      break;
  } 
  
  delay(700/steps);
    
  for(int y=0; y<255; y+=steps)
    {  
    for (i=0; i < strip.numPixels(); i++) 
      {
      uint32_t c = Wheel(((i * 384 / strip.numPixels()) + shiftRand) % 384);
  
      byte  r, g, b, r2, g2, b2, r8, g8,b8;
      
      // Need to decompose color into its r, g, b elements
      g = (c >> 16) & 0x7f;
      r = (c >>  8) & 0x7f;
      b =  c        & 0x7f; 
     
      r8 = r << 1;
      g8 = g << 1;
      b8 = b << 1;   
      
      float rr = r8/255;
      float gr = g8/255;
      float br = b8/255;
    
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

      strip.setPixelColor(i, r2, g2, b2);
    }
  strip.show();   // write all the pixels out
  if(semaphoreFlagMode == true)
    break;
  } 
}
  

void smoothStrobe(uint32_t c, uint16_t wait)
{
  // Improvement -> Static duration of fade by scaling the wait by the highColorByte
  
  byte  r, g, b, r2, g2, b2, r8, g8,b8;
  
  // Need to decompose color into its r, g, b elements
  g = (c >> 16) & 0x7f;
  r = (c >>  8) & 0x7f;
  b =  c        & 0x7f; 
 
  r8 = r << 1;
  g8 = g << 1;
  b8 = b << 1;

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
  
  float rr = r8/255;
  float gr = g8/255;
  float br = b8/255;

  for(int y=highColorByte-26; y>0; y--)
  {
    for (int i=0; i < strip.numPixels(); i++) 
    {
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

      strip.setPixelColor(i, r2, g2, b2);
    }
    
    strip.show();   // write all the pixels out
    if(semaphoreFlagMode == true)
      break;
    delay(wait);
  }
  
  
  for(int y=0; y<highColorByte-26; y++)
  {
    for (int i=0; i < strip.numPixels(); i++) 
    {
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

      strip.setPixelColor(i, r2, g2, b2);
    }
    
    strip.show();   // write all the pixels out
    if(semaphoreFlagMode == true)
      break;
    delay(wait);
  }
}


void pulseStrobe(uint32_t c, uint16_t wait)
{
  for(int y=20; y>0; y--)
  {
    for (int i=0; i < strip.numPixels(); i++) 
    {
      if(y%2)
        {
        strip.setPixelColor(i, dampenBrightness(c, y));
        } else {
        strip.setPixelColor(i, dampenBrightness(c, 10)); 
        }
    }
    
    strip.show();   // write all the pixels out  
    if(semaphoreFlagMode == true)
      break;
    delay(wait/y);
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
 
  for (i=0; i < strip.numPixels(); i++) 
    {
    strip.setPixelColor(i, c);
    strip.show();
    if(semaphoreFlagMode == true)
      break;  
    delay(wait);
    }
  
  delay(wait*5);
}


// Chase a dot down the strip
// Random color for each chase
void colorChase(uint32_t c, uint16_t wait) {
  int i;

  for (i=0; i < strip.numPixels(); i++) 
  {
      strip.setPixelColor(i, c); // set one pixel
      strip.show();              // refresh strip display
      if(semaphoreFlagMode == true)
        break;
      delay(wait*10);               // hold image for a moment
      strip.setPixelColor(i, 0); // erase pixel (but don't refresh yet)
  }
  strip.show(); // for last erased pixel
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
    if(semaphoreFlagMode == true)
      break;
    delay(wait);
  }
  delay(wait);  
}


void randomSparkle(uint16_t wait) {

  long randNumber;
  
  // Determine highest bit needed to represent pixel index
  uint16_t i, j;

  for(int i=0; i<strip.numPixels(); i++) 
  {
    randNumber = random(0, strip.numPixels()-1);
    strip.setPixelColor(randNumber, Wheel(((i * 384 / strip.numPixels()) + animationStep) % 384));
    strip.show();
    if(semaphoreFlagMode == true)
      break;
  }
  delay(wait);
  animationStep++;

}


// "Larson scanner" = Cylon/KITT bouncing light effect
void scanner(uint32_t c, uint16_t wait) {

  byte  r, g, b;
  
  // Decompose color into its r, g, b elements
  g = (c >> 16) & 0x7f;
  r = (c >>  8) & 0x7f;
  b =  c        & 0x7f; 
  
  int i, j, pos, dir;

  pos = 0;
  dir = 1;

  for(i=0; i<((strip.numPixels()-1) * 2); i++) {
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
    if(semaphoreFlagMode == true)
      break;      
    delay(wait);
    // If we wanted to be sneaky we could erase just the tail end
    // pixel, but it's much easier just to erase the whole thing
    // and draw a new one next time.
    for(j=-2; j<= 2; j++) 
        strip.setPixelColor(pos+j, strip.Color(0,0,0));
    // Bounce off ends of strip
    pos += dir;
    if(pos < 0) {
      pos = 1;
      dir = -dir;
    } else if(pos >= strip.numPixels()) {
      pos = strip.numPixels() - 2;
      dir = -dir;
    }
  }
}

// Sine wave effect.
// Self calibrating for pixel run length.
void wave(uint32_t c, uint16_t wait) {
  float y;
  byte  r, g, b, r2, g2, b2;

  long randNumber = random(0, 128);
  c = Wheel(((randNumber * 384 / strip.numPixels()) + randNumber) % 384);

  // Need to decompose color into its r, g, b elements
  g = (c >> 16) & 0x7f;
  r = (c >>  8) & 0x7f;
  b =  c        & 0x7f; 

  for(int x=0; x<(strip.numPixels()*2); x++)
  {
    for(int i=0; i<strip.numPixels(); i++) {
      y = sin(PI * 1 * (float)(x + i) / (float)strip.numPixels());
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
      strip.setPixelColor(i, r2, g2, b2);
    }
    strip.show();
    if(semaphoreFlagMode == true)
      break;
    delay(wait);
  }
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
  return pgm_read_byte(&gammaTable[x]);
}


