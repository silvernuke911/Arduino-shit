//============================================================================
//****************************************************************************
//
//             SMART CARBON DIOXIDE DETECTION & ALERT SYSTEM
//      Real-time CO2 Monitoring with Visual/Auditory Warning System
//
//**************************************************************************** 
//----------------------------------------------------------------------------
//      PLATFORM: Arduino Uno R3
//      SENSOR:   MQ-135 Air Quality Sensor
//      DISPLAY:  16x2 Character LCD (1602A)
//      ACTUATORS: SG90 Servo, Buzzer, R Status LED
//
//----------------------------------------------------------------------------
//      FEATURES:
//      - Real-time CO2 concentration measurement (PPM)
//      - 4-level air quality assessment (Good/Fair/Poor/Dangerous)
//      - Automatic calibration in clean air
//      - 50Hz sampling rate with moving average filtering
//      - Multi-mode warning system (LCD, Buzzer, Servo, LED)
//      - Serial monitor diagnostics and logging
//      - Automatic servo manipulation
//      - Regular recalibration every 5 mins
//        - Readings drift and become innacurate over time, requiring regular correction.
//
//      LIMITATIONS
//      - 20 second MQ135 Sensor preheating at startup
//      - 5 second Air quality Calibration
//      - Requires clean air (Assumption approx. 400 ppm)
//----------------------------------------------------------------------------
//      AIR QUALITY THRESHOLDS:
//      MQ135 is not a CO2 sensor, this is only an equivalent
//      Air quality estimate based on relative documentation
// 
//      GOOD      (< 450 PPM)    : Mormal operation
//      FAIR      (450-800 PPM)  : Acceptable indoor air
//      POOR      (800-1500 PPM) : Poor ventilation
//      DANGEROUS (> 1500 PPM)   : Unnaceptable levels (but survivable)
//                (2000 PPM)     : Warning system activated
//
//----------------------------------------------------------------------------
//      ALERT THRESHOLD: 2000 PPM CO2
//      When exceeded:
//      1. SERVO opens 90° for ventilation/access
//      2. BUZZER emits continuous warning pattern
//      3. LED flashes bright red
//      4. LCD displays emergency warning message
//
//----------------------------------------------------------------------------
//      HARDWARE CONNECTIONS:
//      MQ-135 AO  -> A0    | SG90 Servo     -> D5
//      MQ-135 DO  -> D4    | Buzzer         -> D11
//      LCD RS     -> D2    | Common Cathode -> GND
//      LCD EN     -> D3    | 
//      LCD D4-D7  -> D6-D9 | 
//      Warning LED-> D13   | 
//
//----------------------------------------------------------------------------
//      VERSION : 1.5.4
//      DATE    : January 17, 2026
//      AUTHOR  : Silvernuke911
//      
//      "Mors vincit omnia, usque ad finem vitae"
//
//============================================================================
// TODO: Add manual recalibration button.
//       Arduino house casing
//		Stabilize signal drift (sensor readings drift from baseline over time)
//		Cannot stay stable for a long time. Triggers the warning system 
//      due to drift. Solution: either regular recalibration (plus rolling average)
// 		or forced recalibration when measured R0 is stable for 1 minute and deviates
//      from original R0 by
//
//============================================================================

//============================================================================
// LIBRARY INCLUSIONS
//============================================================================
#include <Arduino.h>
#include <globals.h>
#include <utils.h>
#include <misc.h>
#include <calib.h>
#include <response.h>

//============================================================================
// INITIALIZATIONS
//============================================================================
void setup() {
    Serial.begin(9600);             // Serial data transfer
    initializeHardwarePins();       // Initializing hardware pin
    lcd.begin(16, 2);               // Initializing LCD
    initializeServo();              // Initializing servo motor
    initializeSensorArray();        // Initializing sensors
    displayStartupMessage();        // Display device name and group name
    performSensorPreheating();      // 20 second mandatory preheating for MQ135 Sensor
	originalR0 = R0;				// calibrate original R0 reading for sensor.
    calibrateInitWaiting();         // Calibration wating time for user
    calibrateSensor();              // Calibrate sensor
    lastCalibrationTime = millis(); // Start calibration timers
    displaySystemReady();           // user ready display
    initializeSensorTiming();       // Initializing timing of sensor for moving average
    performInitialDiagnostics();    // Diagnostic information
}

//============================================================================
// MAIN LOOP
//============================================================================

void loop() {
    if (!isPreheated) return;                               // make sure that the MQ135 sensor is preheated
    updatePPMReading();                                     // consistently update ppm reading

    static unsigned long lastProcessTime = 0;               // reset process time
    if (millis() - lastProcessTime >= 1000) {               // if last process time was a second ago, run subroutine below
        lastProcessTime = millis();                         // set last process time
        checkRecalibration();                               // check whether 5 mins has passed since last recalibration    
		MQ135SensorDirectData();			    			// Update sensor direct analog and digital data.
        float ppm = getAveragePPM();                        // get the current ppm reading
        int qualityLevel = getAirQualityLevel(ppm);         // get the air quality level
        String qualityText = getQualityText(qualityLevel);  // turn that to text
        bool isAboveThreshold = (ppm > PPM_THRESHOLD);      // check whether the ppm level is above the set threshold (2000 ppm)

        if (recalibrationDue 
            && (ppm < 700) 
            && !isWarningActive) {                          // check whether regular recalibration is due, and ppm levels are safe, and 
            performRegularRecalibration();                  // if warning systems are not running (to not interfere in emergencies)
        }                                                   // if all are satisfied, recalibrate device (assume 400-700 ppm air)

        if (isAboveThreshold || (sensor_voltage 			// if ppm is above ppm danger (active )threshold or above raw sensor threshold
					> SENSOR_VOLTAGE_THRESHOLD)) {          // (passive failsafe), the routine:
            handleWarningState(ppm, qualityText);           // activate warning systems
        } else {											// otherwise
            handleNormalState(ppm, qualityText);            // do normal processes (display ppm, close systems)
        }

        logSensorData(ppm, qualityText);                    // sensor data logging.
		debugSensor();										// data debugging.
    }
}

