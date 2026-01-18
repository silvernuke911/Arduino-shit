/**
 * @file calib.cpp
 * @brief MQ-135 sensor calibration and recalibration routines.
 *
 * This module implements all calibration-related functionality for the
 * MQ-135 air quality sensor. It supports:
 *
 *  - Initial clean-air calibration at startup
 *  - Periodic recalibration based on elapsed time
 *  - Drift detection using Rs/R0 deviation
 *
 * Calibration assumes a clean-air baseline of approximately 400 ppm CO2
 * equivalent, as commonly used for MQ-135 sensors.
 *
 * Dependencies:
 *  - globals.h : shared system state (R0, timing flags, LCD, pins)
 *  - utils.h   : sensor math (Rs, PPM calculations)
 *
 * Hardware:
 *  - MQ-135 analog output on CO2_analog_pin
 *  - 16x2 character LCD
 *
 * Timing:
 *  - Initial calibration: ~7 seconds total
 *  - Regular recalibration: user-assisted, blocking
 *
 * Limitations:
 *  - Requires user to place device in clean air
 *  - Blocking delays are used intentionally for stability
 */


#include "calib.h"
#include "globals.h"
#include "utils.h"
#include <Arduino.h>
#include <math.h>

//======================================================================
// Calibration Functions
//======================================================================

/**
 * @brief Initial clean-air waiting period before calibration.
 *
 * Displays a prompt on the LCD and waits for 5 seconds to allow the user
 * to place the device in a clean-air environment.
 *
 * This function is blocking and should only be called during startup.
 *
 * Assumptions:
 *  - Ambient CO2 is near baseline (~400 ppm)
 */
void calibrateInitWaiting() {
	// Wait 5 seconds to ensure device is in clean air area.
	lcd.clear();
	lcd.setCursor(0,0); lcd.print("Place in clean");
	lcd.setCursor(0,1); lcd.print("air (5 seconds)");

	Serial.println("Please put device in clean air area (approx. 400 ppm CO2...)");
	delay(5000);
}

/**
 * @brief Performs full sensor calibration and computes R0.
 *
 * Samples the MQ-135 analog output multiple times in clean air,
 * calculates the average sensor resistance (Rs), and derives the
 * reference resistance R0 using the standard MQ-135 clean-air ratio.
 *
 * Calibration steps:
 *  1. Take 50 analog samples at ~10 Hz
 *  2. Convert ADC readings to voltage
 *  3. Compute Rs for each sample
 *  4. Average Rs and divide by clean-air ratio (1.8)
 *
 * Side effects:
 *  - Updates global R0
 *  - Updates LCD with progress and test PPM
 *  - Prints diagnostic output to Serial
 *
 * Blocking: YES (~7 seconds)
 */
void calibrateSensor() {
	lcd.clear(); 
	lcd.setCursor(0,0); 
	lcd.print("Calibrating...");

	Serial.println("Calibrating ...");

	delay(2000);

	float sumRs=0; 
	int samples=50;
	for(int i = 0; i < samples; i++){
		int raw = analogRead(CO2_analog_pin);
		float volt = raw*(5.0/1023.0);
		sumRs += calculateRs(volt);

		// display progress
		lcd.setCursor(0,1);
		if (i<10) {
			lcd.print("0");
		} 
		lcd.print(i+1); 
		lcd.print("/"); 
		lcd.print(samples); 
		lcd.print(" samples     ");

		Serial.print(i+1);Serial.print("/");
		Serial.print(samples);Serial.print(" samples\r");
		delay(100);
	}

	float Rs_clean = sumRs/samples;
	R0 = Rs_clean/1.8;
	float testPPM = calculatePPM(analogRead(CO2_analog_pin)*(5.0/1023.0));

	lcd.setCursor(0,1); 
	lcd.print("Test: "); 
	lcd.print((int)testPPM); 
	lcd.print(" ppm"); 

	Serial.print("\nTest: ");Serial.print(testPPM,2);Serial.print(" ppm");
	debugSensor();
	delay(2000);
}

/**
 * @brief Determines whether periodic recalibration is due.
 *
 * Compares the current system time against the last calibration time.
 * If the elapsed time exceeds RECALIBRATION_INTERVAL, sets the
 * recalibrationDue flag.
 *
 * Handles millis() rollover safely.
 *
 * Does not perform recalibration directly.
 */
void checkRecalibration() {
	unsigned long currentTime = millis();
	if(currentTime < lastCalibrationTime) {
		lastCalibrationTime = currentTime;
	}
	if(currentTime - lastCalibrationTime >= RECALIBRATION_INTERVAL) {
		recalibrationDue = true;
	}
}

/**
 * @brief Executes a scheduled recalibration sequence.
 *
 * If recalibration is due, prompts the user to place the device in
 * clean air, performs a countdown, and reuses calibrateSensor()
 * to update R0.
 *
 * Conditions:
 *  - recalibrationDue must be true
 *
 * Side effects:
 *  - Updates global R0
 *  - Resets lastCalibrationTime
 *  - Clears recalibrationDue flag
 *
 * Blocking: YES (user-assisted)
 */
void performRegularRecalibration() {
	if(!recalibrationDue) {
		return;
	}

	lcd.clear(); 
	lcd.setCursor(0,0); lcd.print(" Rglr Recalib  ");
	lcd.setCursor(0,1); lcd.print("Place clean air");
	Serial.print("Regular recalibration due...");
	delay(2000);

	for(int i = 3; i > 0; i--){
		lcd.setCursor(0,1); 
		lcd.print(i); 
		lcd.print(" seconds     "); 
		delay(1000);
	}

	calibrateSensor();
	lastCalibrationTime = millis(); 
	recalibrationDue=false;
}

/**
 * @brief Performs a fast drift check without recalibrating.
 *
 * Takes a small number of samples, estimates a temporary R0 value,
 * and compares it to the original calibration reference.
 *
 * If deviation exceeds 10%, a warning is issued via Serial output.
 *
 * Purpose:
 *  - Early detection of sensor drift
 *  - Diagnostic use only (non-corrective)
 *
 * Does NOT modify R0.
 */
void quickRecalibrationCheck() {
	float sumRs=0; 
	int samples=10;
	for(int i = 0; i < samples; i++){
		float Rs = calculateRs(analogRead(CO2_analog_pin)*(5.0/1023.0));
		sumRs += Rs;
		delay(100);
	}

	float avgRs=sumRs/samples;
	float R0calc = avgRs/2.17;
	if(abs((R0calc/originalR0-1))*100>10) {
		Serial.println("WARNING: Sensor drift!");
	}
}
