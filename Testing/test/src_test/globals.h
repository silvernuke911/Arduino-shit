#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <LiquidCrystal.h>
#include <Servo.h>

//---------------------------
// Hardware Pins
//---------------------------
extern int CO2_analog_pin;
extern int CO2_digital_pin;
extern int LED_output;
extern int Buzzer_output;

extern Servo DoorServo;
extern const int servoPin;

extern LiquidCrystal lcd;

//---------------------------
// Sensor calibration
//---------------------------
extern float R0;
extern const float RL;

//---------------------------
// Timing & sampling
//---------------------------
extern const int SAMPLES_PER_READING;
extern float ppmReadings[];
extern int readingIndex;
extern unsigned long lastSampleTime;

//---------------------------
// Flags & states
//---------------------------
extern bool isPreheated;
extern bool isWarningActive;
extern bool recalibrationDue;
extern bool skipPreheating;
extern unsigned long lastCalibrationTime;

//---------------------------
// Thresholds
//---------------------------
extern const int PPM_THRESHOLD;

#endif
