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
 *  - Implementing non-blocking buzzer patterns for continuous operation
 *
 * The module does NOT:
 *  - Perform sensor sampling
 *  - Calculate PPM values
 *  - Decide when warnings should be triggered
 *  - Handle the main timing loop (buzzer updates must be called externally)
 *
 * All warning decisions are made by the main control loop.
 *
 * Dependencies:
 *  - globals.h : shared system state and hardware objects
 *  - misc.h    : LCD and hardware helpers
 *
 * Design notes:
 *  - Buzzer patterns use non-blocking millis() timing for system responsiveness
 *  - Warning state transitions include mechanical delays for servo stability
 *  - LCD warning display has a timed phase for maximum user attention
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
 * entering a warning state, then manages the warning display.
 * Note: Buzzer control has been moved to non-blocking routines
 * that must be called from the main loop.
 *
 * Parameters:
 *  @param ppm          Current averaged CO2 concentration (PPM)
 *  @param qualityText  Human-readable air quality label (unused here)
 *
 * Side effects:
 *  - Activates LED and servo via activateWarningSystem()
 *  - Records warning start time for display timing
 *  - Updates LCD with warning messages
 *  - Sets global isWarningActive flag
 *
 * Note: This function no longer calls the blocking warning_buzzer()
 */
void handleWarningState(float ppm, String qualityText){
    if(!isWarningActive){
        activateWarningSystem(); 
        // isWarningActive = true;
        // warningStartTime = millis(); // Redundant, already set in activate warning system
    }
    // warning_buzzer() removed - now handled by non-blocking updateBuzzer()
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
 *  - Deactivates warning system if active
 *  - Ensures LED and buzzer are off (safety redundancy)
 *  - Updates LCD with normal status display
 *  - Clears global warning state flag
 */
void handleNormalState(float ppm, String qualityText){
    if(isWarningActive){ 
        deactivateWarningSystem(); 
        isWarningActive = false; // Redundant safety, already handled in routine above, kept for security.
    }
    // Redundant safety - ensure outputs are off
    digitalWrite(LED_output, LOW);
    digitalWrite(Buzzer_output, LOW);
    displayNormalMessage(ppm, qualityText);
}

/**
 * @brief Displays warning information on the LCD.
 *
 * Shows a prominent warning banner for a fixed duration when the
 * warning state is first entered. After this period, the display
 * switches to showing the live CO2 concentration with threshold
 * comparison.
 *
 * Parameters:
 *  @param ppm Current averaged CO2 concentration (PPM)
 *
 * Display phases:
 *  1. Initial 3 seconds: "WARNING! HIGH CO2 LEVEL!" (full attention)
 *  2. After 3 seconds: "CO2: [value] ppm > 2000 ppm!" (continuous monitoring)
 *
 * Note:
 *  - Uses global warningStartTime for timing consistency
 *  - Does not clear LCD between updates to prevent flickering
 *  - Maintains fixed field widths for stable display layout
 */
void displayWarningMessage(float ppm) {
    // Show full warning for the first few seconds
    if (millis() - warningStartTime < WARNING_DISPLAY_TIME) {
        lcd.setCursor(0, 0);
        lcd.print("    WARNING!    ");
        lcd.setCursor(0, 1);
        lcd.print("HIGH CO2 LEVEL! ");
    } else {
        // After initial warning, show actual PPM with threshold comparison
        lcd.setCursor(0, 0);
        lcd.print("CO2: ");
        lcd.print((long)ppm);
        lcd.print(" ppm     ");
        
        lcd.setCursor(0, 1);
        lcd.print(">");
        lcd.print(PPM_THRESHOLD);
        lcd.print(" ppm!     ");
    }
}

/**
 * @brief Displays normal operating information on the LCD.
 *
 * Shows the current CO2 concentration and qualitative air quality
 * assessment during non-warning operation. Uses fixed-width formatting
 * for consistent display layout.
 *
 * Parameters:
 *  @param ppm          Current averaged CO2 concentration (PPM)
 *  @param qualityText  Human-readable air quality label (8 chars)
 *
 * Display format:
 *  Line 1: "CO2: [value] ppm   " (padded to 16 chars)
 *  Line 2: "Quality: [label]   " (label is 8 chars, padded)
 *
 * Note: The qualityText parameter should be pre-formatted to 8 characters
 *       (e.g., "Good     ", "Fair     ", "Poor     ", "DANGER   ")
 */
void displayNormalMessage(float ppm, String qualityText){
    lcd.setCursor(0, 0); 
    lcd.print("CO2: "); 
    lcd.print((long) ppm); 
    lcd.print(" ppm        ");
    
    lcd.setCursor(0, 1); 
    lcd.print("Quality: "); 
    lcd.print(qualityText);
}

//====================================================
// Warning System Control
//====================================================

/**
 * @brief Activates all warning hardware outputs.
 *
 * Engages the full warning system including visual (LED), mechanical
 * (servo), and audible (buzzer) indicators. Includes a mechanical
 * stabilization delay for the servo.
 *
 * Side effects:
 *  - LED turned ON (visual warning)
 *  - Servo opened to 90° (ventilation/access indication)
 *  - Non-blocking buzzer pattern started
 *  - Global warning state and timing set
 *  - Serial notification logged
 *  - 500ms blocking delay for servo stabilization
 *
 * Note: The buzzer uses non-blocking timing and must be updated
 *       regularly via updateBuzzer() from the main loop.
 */
void activateWarningSystem(){ 
    digitalWrite(LED_output, HIGH); 
    DoorServo.write(90); 
    startBuzzer();  // Start the non-blocking buzzer pattern
    
    isWarningActive = true;
    warningStartTime = millis();
    Serial.println("WARNING SYSTEM ACTIVATED!");
    delay(500);  // Mechanical stabilization delay for servo
}

/**
 * @brief Legacy blocking buzzer function - 
 * OBSOLETE; replaced with nonblocking routines
 *
 * DEPRECATED: This function blocks execution for 550ms, causing system
 * unresponsiveness. Replaced by updateBuzzer()/startBuzzer()/stopBuzzer()
 * non-blocking implementation.
 *
 * Original behavior: 500ms ON, 50ms OFF blocking pattern. Halts the entire program.
 *
 * Side effects:
 *  - Blocks execution for 550ms
 *  - Audible buzzer output
 *  - Freezes all other system functions during tone
 *
 * @warning Do not use in production. Retained only for reference.
 */
void warning_buzzer(){ 
    digitalWrite(Buzzer_output, HIGH); 
    delay(500); 
    digitalWrite(Buzzer_output, LOW); 
    delay(50);
}

/**
 * @brief Updates the non-blocking buzzer state based on timing.
 *
 * Implements a state machine that toggles buzzer output between
 * 500ms ON and 50ms OFF states without blocking execution. Must be
 * called regularly from the main loop (every iteration recommended).
 *
 * State machine logic:
 *  - ON state: 500ms duration, buzzer HIGH
 *  - OFF state: 50ms duration, buzzer LOW
 *  - Repeats continuously while buzzerActive is true
 *
 * Timing:
 *  - Uses millis() for non-blocking time tracking
 *  - Maintains precise 500ms/50ms duty cycle
 *  - No drift over extended operation
 *
 * Note: This function does nothing if buzzerActive is false.
 *       Control via startBuzzer() and stopBuzzer().
 */
void updateBuzzer() {
    if (!buzzerActive) {
        digitalWrite(Buzzer_output, LOW);
        return;
    }
    
    unsigned long currentTime = millis();
    
    if (buzzerState) {
        // Currently ON, check if 500ms elapsed
        if (currentTime - buzzerTimer >= 500) {
            digitalWrite(Buzzer_output, LOW);  // Turn OFF
            buzzerState = false;
            buzzerTimer = currentTime;
        }
    } else {
        // Currently OFF, check if 50ms elapsed
        if (currentTime - buzzerTimer >= 50) {
            digitalWrite(Buzzer_output, HIGH);  // Turn ON
            buzzerState = true;
            buzzerTimer = currentTime;
        }
    }
}

/**
 * @brief Starts the non-blocking buzzer warning pattern.
 *
 * Initializes the buzzer state machine and begins the 500ms ON,
 * 50ms OFF repeating pattern. The buzzer starts in the ON state
 * for immediate audible feedback.
 *
 * Side effects:
 *  - Sets buzzerActive flag to true
 *  - Initializes buzzerState to ON (true)
 *  - Records current time in buzzerTimer
 *  - Immediately sets buzzer output HIGH
 *
 * Note: updateBuzzer() must be called regularly after this to
 *       maintain proper timing.
 */
void startBuzzer() {
    buzzerActive = true;
    buzzerState = true;  // Start with ON state
    buzzerTimer = millis();
    digitalWrite(Buzzer_output, HIGH);
}

/**
 * @brief Stops the non-blocking buzzer pattern.
 *
 * Deactivates the buzzer state machine and ensures the buzzer
 * output is turned off. Safe to call even if buzzer is not active.
 *
 * Side effects:
 *  - Sets buzzerActive flag to false
 *  - Resets buzzerState to OFF (false)
 *  - Ensures buzzer output is LOW
 *
 * Note: This function provides a clean shutdown of buzzer operation
 *       and prevents any residual buzzing.
 */
void stopBuzzer() {
    buzzerActive = false;
    buzzerState = false;
    digitalWrite(Buzzer_output, LOW);
}

/**
 * @brief Deactivates all warning hardware outputs.
 *
 * Shuts down the complete warning system, returning all components
 * to their normal operating state. Includes mechanical stabilization
 * delay for the servo.
 *
 * Side effects:
 *  - LED turned OFF
 *  - Servo returned to 0° (closed position)
 *  - Non-blocking buzzer pattern stopped
 *  - Global warning state cleared
 *  - Serial notification logged
 *  - 500ms blocking delay for servo stabilization
 *
 * Note: Includes redundant buzzer deactivation for safety.
 */
void deactivateWarningSystem(){ 
    digitalWrite(LED_output, LOW); 
    DoorServo.write(0); 
    stopBuzzer();  // Stop the non-blocking buzzer pattern
    
    // Redundant safety - ensure buzzer is off
    digitalWrite(Buzzer_output, LOW);
    
    isWarningActive = false;
    Serial.println("Warning system deactivated.");
    delay(500);  // Mechanical stabilization delay for servo (might be unnecessary)
}