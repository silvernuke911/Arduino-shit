/**
 * @file response.cpp
 * @brief Warning and normal operation response logic.
 *
 * This module implements the system response layer, responsible for
 * transitioning between normal operation and emergency warning states
 * based on measured CO2 concentration.
 *
 * Responsibilities include:
 *  - Activating and deactivating warning hardware (LED, buzzer, servo)
 *  - Managing warning state transitions
 *  - Displaying warning and normal messages on the LCD
 *
 * The module does NOT:
 *  - Perform sensor sampling
 *  - Calculate PPM values
 *  - Decide when warnings should be triggered
 *
 * All decisions are made by the main control loop.
 *
 * Dependencies:
 *  - globals.h : shared system state and hardware objects
 *  - misc.h    : LCD and hardware helpers
 *
 * Design notes:
 *  - Warning behavior is intentionally blocking (delays) for user visibility
 *  - State transitions are guarded by isWarningActive
 */

#include "response.h"
#include "globals.h"
#include "misc.h"

//====================================================
// Warning/Normal Handling
//====================================================
/**
 * @brief Handles system behavior during a warning condition.
 *
 * Ensures that the warning system is activated exactly once when
 * entering a warning state, then continuously updates the buzzer
 * and LCD warning display.
 *
 * Parameters:
 *  @param ppm          Current averaged CO2 concentration (PPM)
 *  @param qualityText  Human-readable air quality label (unused here)
 *
 * Side effects:
 *  - Activates LED, servo, and buzzer
 *  - Updates LCD with warning messages
 *  - Sets isWarningActive flag
 */
void handleWarningState(float ppm, String qualityText){
	if(!isWarningActive){
		 activateWarningSystem(); 
		 isWarningActive=true; 
	}
	warning_buzzer();
	displayWarningMessage(ppm);
}

/**
 * @brief Handles system behavior during normal operation.
 *
 * Ensures that any previously active warning system is fully
 * deactivated, then displays the current CO2 level and air quality.
 *
 * Parameters:
 *  @param ppm          Current averaged CO2 concentration (PPM)
 *  @param qualityText  Human-readable air quality label
 *
 * Side effects:
 *  - Turns off LED and buzzer
 *  - Resets servo to closed position
 *  - Clears warning state flag
 *  - Updates LCD with normal status display
 */
void handleNormalState(float ppm, String qualityText){
	if(isWarningActive){ 
		deactivateWarningSystem(); 
		isWarningActive=false; 
	}
	digitalWrite(LED_output,LOW);
	digitalWrite(Buzzer_output,LOW);
	displayNormalMessage(ppm,qualityText);
}

/**
 * @brief Displays warning information on the LCD.
 *
 * Shows a prominent warning banner for a fixed duration when the
 * warning state is first entered. After this period, the display
 * switches to showing the live CO2 concentration.
 *
 * Parameters:
 *  @param ppm Current averaged CO2 concentration (PPM)
 *
 * Internal behavior:
 *  - Uses a static timestamp to track warning display duration
 *
 * Note:
 *  - This function does not clear the LCD to avoid flicker
 */
void displayWarningMessage(float ppm) {
	static unsigned long warningStartTime = millis();

	// Show full warning for the first few seconds
	if (millis() - warningStartTime < WARNING_DISPLAY_TIME) {
		lcd.setCursor(0,0);
		lcd.print("    WARNING!    ");
		lcd.setCursor(0,1);
		lcd.print("HIGH CO2 LEVEL! ");
	} else {
	// After initial warning, show actual PPM
		lcd.setCursor(0,0);
		lcd.print("CO2: ");
		lcd.print((int)ppm);
		lcd.print(" ppm     ");

		lcd.setCursor(0,1);
		lcd.print(">");
		lcd.print(PPM_THRESHOLD);
		lcd.print(" ppm!     ");
	}
}

/**
 * @brief Displays normal operating information on the LCD.
 *
 * Shows the current CO2 concentration and qualitative air quality
 * assessment during non-warning operation.
 *
 * Parameters:
 *  @param ppm          Current averaged CO2 concentration (PPM)
 *  @param qualityText  Human-readable air quality label
 */
void displayNormalMessage(float ppm, String qualityText){
	lcd.setCursor(0,0); lcd.print("CO2: "); 
	lcd.print((int)ppm); lcd.print(" ppm   ");
	lcd.setCursor(0,1); lcd.print("Quality: "); 
	lcd.print(qualityText);
}


//====================================================
// Warning System Control
//====================================================
/**
 * @brief Activates all warning hardware outputs.
 *
 * Turns on the status LED, opens the ventilation servo to 90 degrees,
 * and introduces a short delay for mechanical stability.
 *
 * Side effects:
 *  - LED ON
 *  - Servo opened
 *  - Blocking delay
 */
void activateWarningSystem(){ 
	digitalWrite(LED_output,HIGH); 
	DoorServo.write(90); 
	delay(500); 
}

/**
 * @brief Emits a warning buzzer pulse.
 *
 * Generates a short audible alert by toggling the buzzer output.
 * Intended to be called repeatedly while in warning state.
 *
 * Side effects:
 *  - Audible buzzer output
 *  - Blocking delays
 */
void warning_buzzer(){ 
	digitalWrite(Buzzer_output,HIGH); 
	delay(500); digitalWrite(Buzzer_output,LOW); 
	delay(50);
}

/**
 * @brief Deactivates all warning hardware outputs.
 *
 * Turns off the LED and buzzer and returns the servo to the closed
 * position.
 *
 * Side effects:
 *  - LED OFF
 *  - Buzzer OFF
 *  - Servo closed
 *  - Blocking delay
 */
void deactivateWarningSystem(){ 
	digitalWrite(LED_output,LOW); 
	digitalWrite(Buzzer_output,LOW); 
	DoorServo.write(0); delay(500); 
}
