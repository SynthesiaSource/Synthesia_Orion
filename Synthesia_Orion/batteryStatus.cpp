#include "batteryStatus.h"
#include "pins.h"

boolean __refreshBatteryStatus; // Interrupt semaphore for updating the battery status.

// initialize Timer1 for use in controlling battery status update
void setupBatteryStatusInterrupt(void) {
  // First disable global interrupts during setup.
  cli();
  
  // Clear the TCCR1A and TCCR1B registers.
  TCCR1A = TCCR1B = 0;
 
  // Set compare match register to desired timer count:
  OCR1A = 3624;
  // Turn on CTC mode:
  TCCR1B |= (1 << WGM12);
  // Set CS10 and CS12 bits for 1024 prescaler:
  TCCR1B |= (1 << CS10);
  TCCR1B |= (1 << CS12);
  // Enable timer compare interrupt:
  TIMSK1 |= (1 << OCIE1A);
  // Now that we are finished, renable global interrupts
  sei();
} // setupBatteryStatusInterrupt()


ISR(TIMER1_COMPA_vect) {
  // Set a semaphore so the main loop can update the battery status.
  // This keeps the interrupt small and fast.
  __refreshBatteryStatus = true;
} // ISR()


void updateBatteryStatus(boolean isUnitPowered) {

//    digitalWrite(PIN_LED_GREEN, HIGH);
//  digitalWrite(PIN_LED_RED  , HIGH);
//  digitalWrite(PIN_LED_BLUE , HIGH);

  if(! __refreshBatteryStatus)
    return;
    
  //Now that the battery status has been updated reset the interrupt semaphone.
  __refreshBatteryStatus = false;

  
  // @todo DOC
  float batteryVoltage = analogRead(PIN_V_SENSE)*0.005;

  // Turn all the LEDs off, this is the default state.
  digitalWrite(PIN_LED_GREEN, HIGH);
  digitalWrite(PIN_LED_RED  , HIGH);
  digitalWrite(PIN_LED_BLUE , HIGH);
  
  // USB power detection
  // When charging LED is purple. When fully charged LED is white.
  if(!(UDINT & B00000001))
  {
    if(batteryVoltage < 4.0)
    {
      //digitalWrite(PIN_LED_GREEN, LOW);
      digitalWrite(PIN_LED_RED  , LOW);
      digitalWrite(PIN_LED_BLUE , LOW); 
      return;
    } else {
        // Flashing white
      digitalWrite(PIN_LED_GREEN, LOW);
      digitalWrite(PIN_LED_RED  , LOW);
      digitalWrite(PIN_LED_BLUE , LOW); 
      return;
    }
  }
  
  // If the unit is powered off, then don't turn on any leds as this wastes battery power.
  if(!isUnitPowered)
    return;
    
  if(batteryVoltage < 3.0) {
    digitalWrite(PIN_LED_RED, LOW);
    return;
  }
  
  if(batteryVoltage < 3.5) {
    digitalWrite(PIN_LED_BLUE, LOW);
    return;
  }

  digitalWrite(PIN_LED_GREEN, LOW);
  
  
  // If fully charged or charging turn on all the LEDs (white) 
//  bit usbConnected = (USBSTA & = 0x01);
//USBSTA & 1
//  if(UDINT & B00000001)
//  {
//
//  } else {
//     digitalWrite(PIN_LED_GREEN, LOW);
//  digitalWrite(PIN_LED_RED  , LOW);
//  digitalWrite(PIN_LED_BLUE , LOW); 
//  }
} // updateBatteryStatus()

void forceStatusLightOff() {
    // Turn all the LEDs off, this is the default state.
  digitalWrite(PIN_LED_GREEN, HIGH);
  digitalWrite(PIN_LED_RED  , HIGH);
  digitalWrite(PIN_LED_BLUE , HIGH);
}
// End of file.

