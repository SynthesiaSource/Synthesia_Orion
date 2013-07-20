#ifndef __SYNTHESIA_BATTERY_STATUS_H
#define __SYNTHESIA_BATTERY_STATUS_H

#include <Arduino.h>

void setupBatteryStatusInterrupt(void);
void updateBatteryStatus(boolean isUnitPowered);
void forceStatusLightOff();

#endif

// End of file.

