#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <LiquidCrystal.h>
#include <Servo.h>
#include <MQ135.h>

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

extern MQ135 mq135_sensor;

//---------------------------
// Timing & sampling
//---------------------------
extern const int SAMPLES_PER_READING;
extern float ppmReadings[];
extern int readingIndex;
extern unsigned long lastSampleTime;
extern const unsigned long WARNING_DISPLAY_TIME; 
extern const unsigned long RECALIBRATION_INTERVAL;
extern const float SENSOR_VOLTAGE_THRESHOLD;

extern int adc;
extern int d0; 
extern float sensor_voltage;

//---------------------------
// Flags & states
//---------------------------
extern bool isPreheated;
extern bool isWarningActive;
extern bool recalibrationDue;
extern bool skipPreheating;
extern unsigned long lastCalibrationTime;
extern unsigned long warningStartTime;

// Buzzer control variables
extern unsigned long buzzerTimer;
extern bool buzzerState;      // false=OFF, true=ON
extern bool buzzerActive;

//---------------------------
// Thresholds
//---------------------------
extern const int PPM_THRESHOLD;

#endif
