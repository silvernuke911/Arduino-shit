#ifndef CALIB_H
#define CALIB_H

#include <Arduino.h>

void calibrateInitWaiting();
void calibrateSensor();
void checkRecalibration();
void performRegularRecalibration();
void quickRecalibrationCheck();

#endif
    