#include "globals.h"

//============================================================================
// HARDWARE PIN CONFIGURATION
//============================================================================

// Air Quality Sensor pins
int CO2_analog_pin = A0;  // MQ135 Analog Pin on pin A0
int CO2_digital_pin = 4;  // MQ135 Digital Pin on pin D4
int LED_output = 13;      // Warning LED input on pin D13
int Buzzer_output = 11;   // Buzzer input on pin D11

// Servo pin
Servo DoorServo;
const int servoPin = 5;   // Servo input on pin D5

// LCD pin connections (1602A)
const int rs = 2;  // RS pin on D2
const int en = 3;  // Enable pin on D3
const int d4 = 6;  // Data pins D4-D7
const int d5 = 7;  // on Uno pin D6-D9
const int d6 = 8;  // Half-byte transfer per cycle
const int d7 = 9;  // 

LiquidCrystal lcd(rs, en, d4, d5, d6, d7); // LCD Object

//============================================================================
// SENSOR CALIBRATION
//============================================================================

// Data from MQ135 Data sheet: 
// https://www.elprocus.com/mq135-air-quality-sensor/

float R0 = 76.63;
const float RL = 20.0;     // Load resistance: 20 kOhms
float originalR0 = 0;
int adc = analogRead(CO2_analog_pin);
int d0  = digitalRead(CO2_digital_pin);
float sensor_voltage = adc * (5.0 / 1023.0);

//============================================================================
// Timing & sampling
//============================================================================
const int SAMPLES_PER_READING = 50;
float ppmReadings[SAMPLES_PER_READING] = {0};
int readingIndex = 0;
unsigned long lastSampleTime = 0;
const unsigned long WARNING_DISPLAY_TIME = 3000;
const unsigned long RECALIBRATION_INTERVAL = 300000;  // 5 mins = 300,000 ms
const float SENSOR_VOLTAGE_THRESHOLD = 1.0; // look at test/co2_testing.ipynb

//============================================================================
// Flags & states
//============================================================================
bool isPreheated      = false;
bool isWarningActive  = false;
bool recalibrationDue = false;
bool skipPreheating   = false;
unsigned long lastCalibrationTime = 0;

//============================================================================
// Thresholds
//============================================================================
const int PPM_THRESHOLD = 2000;
