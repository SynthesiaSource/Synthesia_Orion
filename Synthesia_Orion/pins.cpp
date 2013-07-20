#include "pins.h"

void setupPins(void) {
  // Turn off all the LED's and the strip power before enabling these pins as outputs.
  digitalWrite(PIN_LED_RED  , HIGH);
  digitalWrite(PIN_LED_GREEN, HIGH);
  digitalWrite(PIN_LED_BLUE , HIGH);
  digitalWrite(PIN_STRIP_ENABLE, HIGH);

  // Default to 450mA charge current.
  digitalWrite(PIN_CHARGE_HIGH, HIGH);

  // Set all pin directions with internal pullups enabled on the button pins.
  pinMode(PIN_LED_RED     , OUTPUT);
  pinMode(PIN_LED_GREEN   , OUTPUT);
  pinMode(PIN_LED_BLUE    , OUTPUT);
  pinMode(PIN_STRIP_ENABLE, OUTPUT);
  pinMode(PIN_BUTTON_MODE , INPUT_PULLUP);
  pinMode(PIN_BUTTON_SPEED, INPUT_PULLUP);
  pinMode(PIN_BUTTON_LEVEL, INPUT_PULLUP);
  pinMode(PIN_BUTTON_POWER, INPUT_PULLUP);
  pinMode(PIN_CHARGE_HIGH , OUTPUT);
} // setupPins()

// End of file.

