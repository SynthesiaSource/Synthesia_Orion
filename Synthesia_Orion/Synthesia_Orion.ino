#include <SPI.h>
#include <avr/sleep.h>
#include "PinChangeInt.h"
#include "pins.h"
#include "batteryStatus.h"
#include "orion.h"

boolean poweredOn = false;

//
void togglePower(void) 
{
  noInterrupts();
  poweredOn = !poweredOn;
  
  if(poweredOn)
  {
    updateBatteryStatus(poweredOn);
    updateOrion();
  }
  interrupts();
} // togglePower()


void setup() 
{
  setupPins();
  
  // Make the ADC use the nternal 2.56v reference.
  analogReference(INTERNAL);
  
  // The unit should be off on powerup.
  poweredOn = false; 
  
  // Attach button interrupts
  interrupts();
  PCintPort::attachInterrupt(PIN_BUTTON_MODE, &stepMode, FALLING);
  PCintPort::attachInterrupt(PIN_BUTTON_SPEED, &stepSpeed, FALLING);
  attachInterrupt(INT1, &stepBrightness, FALLING);
  attachInterrupt(INT0, &togglePower, FALLING);
// Does not work. ???
//  attachInterrupt(PIN_BUTTON_LEVEL, &stepBrightness, FALLING);
//  attachInterrupt(PIN_BUTTON_POWER, &togglePower, FALLING);
  
  setupBatteryStatusInterrupt();  
  setupOrion();
} // setup()


void loop() {

  updateBatteryStatus(poweredOn);

  // Start up the device
  if(poweredOn && isDisabled())
    enable(true);
  
  // Shut down the device
  if(!poweredOn && isEnabled())
  {
    disable();
    // Ensure that the status LED is off
    forceStatusLightOff();
    // Define sleep type
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
   // Attach the awake interrupt to the power button
   //attachInterrupt(INT0,sleepHandler, FALLING); 
   sleep_enable();
   sleep_mode();  //sleep now
   sleep_disable(); //fully awake now 
  }

  // Update the LEDs if the device is enabled
  if(poweredOn)
  {
    updateOrion();
  }

}

