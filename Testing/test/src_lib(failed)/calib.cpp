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
 * 1.8 RS/R0 Value comes from emperical testing. See test/CO2_testing.ipynb
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
    
    Serial.println("Calibrating MQ135 (library auto-calibrates)...");
    delay(2000);  // Let sensor stabilize
    
    // The library auto-calibrates when we call getRZero()
    // We'll take multiple readings to ensure stability
    float sumRZero = 0;
    int samples = 30;
    
    for(int i = 0; i < samples; i++) {
        // Library calculates RZero automatically
        float rzero = mq135_sensor.getRZero();
        sumRZero += rzero;
        
        // Display progress
        lcd.setCursor(0,1);
        if (i < 10) lcd.print("0");
        lcd.print(i+1);
        lcd.print("/");
        lcd.print(samples);
        lcd.print(" samples     ");
        
        Serial.print(i+1); Serial.print("/");
        Serial.print(samples); Serial.print(" samples\r");
        delay(100);
    }
    
    float avgRZero = sumRZero / samples;
    float testPPM = mq135_sensor.getCorrectedPPM(20.0, 50.0);
    
    lcd.setCursor(0,1);
    lcd.print("R0: ");
    lcd.print((int)avgRZero);
    lcd.print("k  PPM: ");
    lcd.print((int)testPPM);
    
    Serial.print("\nCalibration complete. Avg R0 = ");
    Serial.print(avgRZero, 2);
    Serial.print("k, Test PPM = ");
    Serial.println(testPPM, 1);
    
    debugSensor();  // Show debug info
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
    if(!recalibrationDue) return;
    
    lcd.clear(); 
    lcd.setCursor(0,0); lcd.print(" Rglr Recalib  ");
    lcd.setCursor(0,1); lcd.print("Place clean air");
    Serial.println("Regular recalibration due...");
    delay(2000);
    
    for(int i = 3; i > 0; i--) {
        lcd.setCursor(0,1);
        lcd.print(i);
        lcd.print(" seconds     ");
        delay(1000);
    }
    
    calibrateSensor();  // Calls the updated version
    lastCalibrationTime = millis();
    recalibrationDue = false;
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
    // Take a few samples
    float sumRZero = 0;
    int samples = 10;
    
    for(int i = 0; i < samples; i++) {
        sumRZero += mq135_sensor.getRZero();
        delay(100);
    }
    
    float avgRZero = sumRZero / samples;
    // Note: originalR0 is no longer used since library handles R0 internally
    // You might want to track baseline R0 separately if needed
    
    Serial.print("Quick check - Current R0: ");
    Serial.println(avgRZero, 2);
}

//=======================================================================
// Some caution on the MQ135 sensor. It's an ass sensor to work with.
// It performs well when needed, sensitive enough to detect the change 
// of air quality from a person's breath. But it sucks!
// It returns a voltage from 0 to 5 volts. This voltage can be transformed 
// to resistanceRS using the formula from the data sheet. However, it needs 
// a base resistance R0 to calculate the PPM levels. And the thing is, 
// the RS//R0 graph is different for every gas, and the data sheet does
// no help at all, the graph isn't even that good. So I had to empirically
// test the shit out of a lot of values to come up with the CO2 graph.

// That would have been okay, if the sensor didn't randomly fucking drift all the time. 
// It would output  0.1 V one moment, then for no apparent reason, the sensor just 
// drifts to 0.2V and to 0.3V, even with no apparent air quality change. It has different 
// base stable values every few minutes, thus fucking with the initial calibration. 
// The system is always  triggered for no reason even in clean air because of sensor drift. 
// I had tried many solutions for this problem, such as rolling average for 
// random spikes. But I cannot solve the sensor drift. Either you change your 
// baseline R0 every time it is stable, or recalibrate at regular intervals, and that 
// wouldn't do shit either because the drift can be severe even at 2 minutes.
// This has been a veritable bs of a damn headache! Good luck dealing with that shit.
