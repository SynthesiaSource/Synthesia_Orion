#ifndef __SYNTHESIA_ORION_H
#define __SYNTHESIA_ORION_H

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
#include <Arduino.h>

// Current draw per meter (32 pixels) at 100%, 50%, 25% brightness
// Rainbow Mode 200mA / 90mA / 45 mA
// Full White 500mA / 250mA / 125mA

// User defined option
#define NUMBER_OF_MODES          12
#define NUMBER_SPEED_SETTINGS    10
#define NUMBER_BRIGHTNESS_LEVELS  5

// Set numberPixels to the total number of LEDs in your strip
// The LED strips are 32 LEDs per meter and can be cut or extended in units of 2 LEDs at the cut lines
// The driver can handle up to 128 pixels. Battery life is proportional to the number of pixels used. 
// All mode battery endurance values are based on a 1 meter (32 pixel) length.
// Thus 2 meters (64) halves the endurance and 4 meters (128 pixels) cuts it to one quarter.
// Change this variable to match the number of pixels in your setup
// If numberPixels is less than the total LEDs connected, some LEDs will go unlit
// If numberPixels is greater than the total LEDs connected, you will get lower performance than if it exactly matches.
#define PIXEL_COUNT              32

#define PI 3.14159265
#define dist(a, b, c, d) sqrt(double((a - c) * (a - c) + (b - d) * (b - d)))

void setupOrion(void);
void updateOrion(void);

void stepMode(void);
void stepSpeed(void);
void stepBrightness(void);
void enable(boolean setBegun);
void disable(void);
boolean isEnabled(void);
boolean isDisabled(void);


// Available modes. Additional modes are available at www.synthesia.ca
void solidColor(void);
void rainbow(void);
void plasma(void);
void splitColorBuilder(void);
void smoothColors(void);
void fadeIn(uint32_t c, uint16_t wait);
void fadeOut(uint32_t c, uint16_t wait);
void sparkler(void);
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
void canada();
void canada2();

// Internal utility functions.
uint32_t Wheel(uint16_t WheelPos);
uint32_t dampenBrightness(uint32_t c, int brightness);

#endif

// End of file.

