/**
 * @file misc.cpp
 * @brief System initialization, startup display, and preheating routines.
 *
 * This module contains all non-sensor, non-calculation utility routines
 * related to:
 *
 *  - Hardware pin initialization
 *  - Servo initialization
 *  - Sensor data structure initialization
 *  - Startup messaging and LCD banners
 *  - MQ-135 sensor preheating procedure
 *  - Initial system diagnostics
 *
 * These routines are intended to be executed during system startup
 * (setup phase) and are generally blocking by design.
 *
 * Dependencies:
 *  - globals.h : shared system state (LCD, pins, flags, buffers)
 *  - utils.h   : debugging and sensor diagnostic output
 *
 * Hardware:
 *  - Arduino Uno R3
 *  - MQ-135 air quality sensor
 *  - SG90 servo motor
 *  - 16x2 character LCD
 *  - Buzzer and status LED
 *
 * Timing:
 *  - Startup banners: ~4 seconds total
 *  - Sensor preheating: 20 seconds (configurable)
 *
 * Design notes:
 *  - Preheating is mandatory for MQ-135 accuracy
 *  - All delays are blocking and intentional
 */


#include "misc.h"
#include "globals.h"
#include "utils.h"

//====================================================
// Initialization
//====================================================
/**
 * @brief Configures all hardware I/O pins.
 *
 * Sets pin modes for:
 *  - Status LED
 *  - MQ-135 digital output
 *  - Buzzer
 *
 * This function must be called before any hardware interaction.
 */
void initializeHardwarePins() {
	pinMode(LED_output, OUTPUT);
	pinMode(CO2_digital_pin, INPUT);
	pinMode(Buzzer_output, OUTPUT);
	Serial.println("Initializing pins ...");
}

/**
 * @brief Initializes and resets the ventilation servo.
 *
 * Attaches the servo to its control pin and moves it to the
 * default closed position (0 degrees).
 */
void initializeServo() {
	DoorServo.attach(servoPin);
	DoorServo.write(0);
	Serial.println("Initializing servo ...");
}

/**
 * @brief Clears the rolling PPM buffer.
 *
 * Initializes the moving-average buffer used for PPM calculations
 * by setting all entries to zero.
 *
 * This prevents undefined behavior during early averaging.
 */
void initializeSensorArray() {
	for (int i=0; i<SAMPLES_PER_READING; i++) {
		ppmReadings[i] = 0;
	}
	Serial.println("Initializing sensor array ...");
}

/**
 * @brief Initializes sensor timing state.
 *
 * Marks the sensor as preheated and initializes the last
 * sample timestamp used by the rolling average logic.
 *
 * Note:
 *  - Preheating may be skipped prior to calling this function
 */
void initializeSensorTiming() {
	isPreheated = true;
	lastSampleTime = millis();
}

/**
 * @brief Runs initial sensor diagnostics.
 *
 * Outputs raw sensor values, voltage, resistance, and
 * computed PPM to the Serial monitor for verification.
 *
 * Intended for debugging and validation at startup.
 */
void performInitialDiagnostics() {
	debugSensorValues();
}

//====================================================
// Display / Startup
//====================================================

/**
 * @brief Displays startup banners on the LCD and Serial monitor.
 *
 * Shows system name and group attribution using timed
 * LCD messages and formatted Serial output.
 *
 * Blocking delay: ~4 seconds
 */
void displayStartupMessage() {
	lcd.clear();
	lcd.setCursor(0,0); lcd.print(" CO2 Detection  ");
	lcd.setCursor(0,1); lcd.print("     System     ");

	Serial.println("=====================================");
	Serial.println("        CO2 Detection System         ");
	Serial.println("        by Group 4 Chem 015          ");
	Serial.println("=====================================");
	delay(2000);

	lcd.setCursor(0,0); lcd.print("   by Group 4   ");
	lcd.setCursor(0,1); lcd.print("    CHEM 015    ");
	delay(2000);
}

/**
 * @brief Indicates that the system is fully initialized.
 *
 * Displays a "System Ready" message on the LCD and prints
 * a confirmation banner to the Serial monitor.
 *
 * Blocking delay: ~2 seconds
 */

void displaySystemReady() {
	lcd.clear();
	lcd.setCursor(0,0); lcd.print("System Ready!");
	Serial.println("=====================================");
	Serial.println("          SYSTEM READY               ");
	Serial.println("=====================================");
	delay(2000);
}

/**
 * @brief Executes MQ-135 sensor preheating sequence.
 *
 * Runs a 20-second warm-up period required for MQ-135 sensor
 * stability. Displays a countdown and animation on the LCD.
 *
 * Can be skipped using the skipPreheating flag (debug use).
 *
 * Blocking: YES (20 seconds)
 */
void performSensorPreheating() {
	if (skipPreheating) {
		return;
	}

	lcd.clear();
	lcd.setCursor(0,0); lcd.print("SensorPreheating");
	lcd.setCursor(0,1); lcd.print("Time: 20 s ");

	Serial.print("Sensor preheating");
	unsigned long startTime = millis();
	unsigned long lastAnim = 0;

	while (millis()-startTime < 20000) {
		if (millis()-lastAnim >= 500) {
			displayPreheatingAnimation(startTime);
			lastAnim = millis();
		}
		delay(50);
		Serial.print(".");
	}
	Serial.println();
}

/**
 * @brief Updates the LCD preheating animation and countdown.
 *
 * Displays a rotating character animation and remaining
 * preheating time in seconds.
 *
 * Parameters:
 *  @param startTime Timestamp marking the beginning of preheating
 *
 * Internal:
 *  - Uses a static frame counter for animation state
 */
void displayPreheatingAnimation(unsigned long startTime) {
	static int frame=0;
	String animation[4] = {"|","/","-","\\"};
	lcd.setCursor(15,1); lcd.print(animation[frame%4]);

	unsigned long remaining = (20000-(millis()-startTime))/1000;
	lcd.setCursor(0,1); lcd.print("Time: "); if(remaining<10) lcd.print("0"); lcd.print(remaining); lcd.print(" s     ");

	frame++;
}

