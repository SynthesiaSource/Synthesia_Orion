#ifndef __SYNTHESIA_PINS_H
#define __SYNTHESIA_PINS_H

#include <Arduino.h>

// Pins for the RGB status led.  LOW is on, HIGH is off.
#define PIN_LED_RED       8 // Red
#define PIN_LED_GREEN     9 // Green
#define PIN_LED_BLUE      5 // Blue

#define PIN_BUTTON_MODE   10
#define PIN_BUTTON_SPEED  17
#define PIN_BUTTON_LEVEL  2
#define PIN_BUTTON_POWER  3

// This pin allows power to flow to the LED strip.
#define PIN_STRIP_ENABLE 13

#define PIN_V_SENSE       5
#define PIN_CHARGE_HIGH  11

void setupPins(void);

#endif

// End of file.

