#ifndef MISC_H
#define MISC_H

#include <Arduino.h>

void initializeHardwarePins();
void initializeServo();
void initializeSensorArray();
void initializeSensorTiming();
void performInitialDiagnostics();

void displayStartupMessage();
void displaySystemReady();
void performSensorPreheating();
void displayPreheatingAnimation(unsigned long startTime);

#endif
