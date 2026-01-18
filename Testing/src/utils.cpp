/**
 * @file utils.cpp
 * @brief Sensor data processing and utility functions.
 *
 * This module implements the core sensor processing logic for the CO2 detection
 * system. It handles raw sensor data conversion, filtering, air quality assessment,
 * and diagnostic logging.
 *
 * Responsibilities include:
 *  - Sampling MQ-135 sensor at regular intervals
 *  - Converting ADC readings to CO2 concentration (PPM)
 *  - Maintaining a moving average of recent readings
 *  - Assessing air quality based on concentration thresholds
 *  - Providing diagnostic output for system debugging
 *
 * The module does NOT:
 *  - Control hardware outputs (LEDs, buzzer, servo)
 *  - Handle user interface displays
 *  - Manage calibration procedures
 *
 * Dependencies:
 *  - globals.h : shared system state, calibration constants, and hardware pins
 *  - utils.h   : function declarations and constants
 *
 * Design notes:
 *  - Implements a 50Hz sampling rate (20ms intervals) for responsive detection
 *  - Uses a circular buffer for moving average calculation
 *  - Exponential formula derived from MQ-135 datasheet characteristics
 *  - All floating-point operations optimized for 8-bit microcontroller
 */

#include "utils.h"
#include "globals.h"

//====================================================
// Sensor Reading
//====================================================

/**
 * @brief Samples the MQ-135 sensor and updates the moving average buffer.
 *
 * Executes at 20ms intervals (50Hz) to maintain responsive detection while
 * providing sufficient filtering against electrical noise. Each sample is
 * converted to PPM and stored in a circular buffer for later averaging.
 *
 * Timing control:
 *  - Uses non-blocking millis() comparison for precise 20ms intervals
 *  - Maintains lastSampleTime to prevent drift
 *
 * Data flow:
 *  - Reads current sensor voltage from global variable
 *  - Converts voltage to PPM using calculatePPM()
 *  - Stores result in ppmReadings circular buffer
 *  - Updates readingIndex for next sample
 *
 * Note: The function expects sensor_voltage to be updated elsewhere
 *       (typically by MQ135SensorDirectData()).
 */
void updatePPMReading() {
    if (millis() - lastSampleTime >= 20) {
        lastSampleTime = millis();
        float samplePPM = calculatePPM(sensor_voltage);
        ppmReadings[readingIndex] = samplePPM;
        readingIndex = (readingIndex + 1) % SAMPLES_PER_READING;
    }
}

/**
 * @brief Calculates the moving average of valid PPM readings.
 *
 * Computes the arithmetic mean of all non-zero samples in the circular buffer.
 * This provides stable readings by filtering out transient spikes and sensor noise.
 *
 * Filter characteristics:
 *  - Buffer size: SAMPLES_PER_READING (typically 50)
 *  - Time window: ~1 second at 50Hz sampling
 *  - Response time: <1 second for significant concentration changes
 *
 * Returns:
 *  @return float - Average CO2 concentration in PPM
 *  @return 0.0 - If no valid samples are available (should not occur in normal operation)
 *
 * Edge cases:
 *  - Ignores zero values (uninitialized buffer positions)
 *  - Returns zero if buffer contains no valid samples
 */
float getAveragePPM() {
    float sum = 0;
    int validSamples = 0;
    for (int i = 0; i < SAMPLES_PER_READING; i++) {
        if (ppmReadings[i] > 0) {
            sum += ppmReadings[i];
            validSamples++;
        }
    }
    return (validSamples > 0) ? sum / validSamples : 0;
}

/**
 * @brief Calculates the sensor resistance Rs from voltage reading.
 *
 * Implements the voltage divider formula to determine the MQ-135 sensor
 * resistance based on the analog voltage reading.
 *
 * Electrical model:
 *  - MQ-135 forms one leg of a voltage divider with load resistor RL
 *  - Formula derived from: Vout = Vin * (RL / (Rs + RL))
 *  - Rearranged to solve for Rs
 *
 * Parameters:
 *  @param sensor_volt - Measured analog voltage (0-5V)
 *
 * Returns:
 *  @return float - Sensor resistance Rs in kilo-ohms (kΩ)
 *
 * Formula: Rs = ((Vcc / Vout) - 1) * RL
 * Where: Vcc = 5.0V, RL = 20.0 kΩ (from datasheet)
 */
float calculateRs(float sensor_volt) {    
    return ((5.0 / sensor_volt) - 1.0) * RL;
}

/**
 * @brief Converts sensor resistance to CO2 concentration (PPM).
 *
 * Applies the MQ-135 transfer function to estimate CO2 concentration
 * based on the ratio of current sensor resistance to baseline resistance.
 *
 * Calibration basis:
 *  - Derived from MQ-135 datasheet response curve
 *  - Assumes 400 PPM in clean air corresponds to Rs/R0 = 1.8
 *  - Uses power law approximation for full concentration range
 *
 * Parameters:
 *  @param sensor_volt - Measured analog voltage (0-5V)
 *
 * Returns:
 *  @return float - Estimated CO2 concentration in parts per million
 *
 * Formula: PPM = 400 * (1.8 / (Rs/R0))^10
 * Where: Rs/R0 is the normalized sensor resistance
 *
 * Note: This provides a reasonable approximation but is not laboratory-grade
 *       accuracy. Regular calibration in known conditions is essential.
 */
float calculatePPM(float sensor_volt) {
    float Rs = calculateRs(sensor_volt);
    float ratio = Rs / R0;
    return 400.0f * pow(1.8f / ratio, 10.0f);
}

//============================================================================
// AIR QUALITY THRESHOLDS
//============================================================================

/**
 * @brief Classifies air quality based on CO2 concentration.
 *
 * Implements a four-tier air quality scale based on established indoor
 * air quality guidelines and MQ-135 sensor characteristics.
 *
 * Thresholds based on:
 *  - ASHRAE Standard 62.1 (Ventilation for Acceptable Indoor Air Quality)
 *  - OSHA guidelines for workplace safety
 *  - Typical indoor air quality assessments
 *
 * Parameters:
 *  @param ppm - CO2 concentration in parts per million
 *
 * Returns:
 *  @return int - Air quality level code:
 *    - 0: GOOD       (< 450 PPM) - Normal outdoor conditions
 *    - 1: FAIR       (< 800 PPM) - Acceptable indoor air
 *    - 2: POOR       (< 2000 PPM) - Poor ventilation, action recommended
 *    - 3: DANGEROUS  (≥ 2000 PPM) - Unacceptable, immediate action required
 *
 * Reference: https://www.co2meter.com/blogs/news/carbon-dioxide-indoor-levels-chart
 */
int getAirQualityLevel(float ppm) {
    if (ppm < 450) return 0;           // GOOD
    else if (ppm < 800) return 1;      // FAIR
    else if (ppm < PPM_THRESHOLD) return 2; // POOR
    else return 3;                      // DANGEROUS
}

/**
 * @brief Converts air quality level code to human-readable string.
 *
 * Provides consistent text labels for display purposes. Each label is padded
 * to 8 characters for proper LCD formatting.
 *
 * Parameters:
 *  @param level - Air quality level code (0-3)
 *
 * Returns:
 *  @return String - Formatted text label:
 *    - "Good     " (Level 0)
 *    - "Fair     " (Level 1)
 *    - "Poor     " (Level 2)
 *    - "DANGER   " (Level 3)
 *    - "Unknown  " (Invalid level)
 *
 * Display consideration:
 *  - Fixed width ensures consistent LCD layout
 *  - Uppercase "DANGER" emphasizes critical condition
 */
String getQualityText(int level) {
    switch(level) {
        case 0: return "Good     ";
        case 1: return "Fair     ";
        case 2: return "Poor     ";
        case 3: return "DANGER   ";
        default: return "Unknown  ";
    }
}

//====================================================
// Debugging & Logging
//====================================================

/**
 * @brief Performs comprehensive sensor diagnostic tests.
 *
 * Captures three sequential sensor readings and displays detailed metrics
 * to the Serial monitor for system validation and troubleshooting.
 *
 * Output includes:
 *  - Raw ADC value (0-1023)
 *  - Calculated voltage (0-5V)
 *  - Sensor resistance Rs (kΩ)
 *  - Rs/R0 normalization ratio
 *  - Estimated CO2 concentration (PPM)
 *
 * Diagnostic purpose:
 *  - Verifies sensor electrical connectivity
 *  - Validates calibration constants
 *  - Confirms signal processing chain
 *  - Provides baseline for troubleshooting
 *
 * Note: Introduces 1-second delays between readings to allow observation.
 */
void debugSensorValues() {
    Serial.println("\n=== SENSOR DIAGNOSTICS ===");
    for (int i=0; i<3; i++) {
        int raw = analogRead(CO2_analog_pin);
        float volt = raw * (5.0/1023.0);
        float Rs = calculateRs(volt);
        float ratio = Rs / R0;
        float ppm = calculatePPM(volt);
        Serial.print("Reading "); Serial.print(i+1);
        Serial.print(": ADC="); Serial.print(raw);
        Serial.print(" V="); Serial.print(volt,3);
        Serial.print(" Rs="); Serial.print(Rs,2);
        Serial.print("k Rs/R0="); Serial.print(ratio,3);
        Serial.print(" PPM="); Serial.println(ppm,1);
        delay(1000);
    }
    Serial.println("\n=========================");
    Serial.println();
}

/**
 * @brief Logs system status to Serial monitor.
 *
 * Provides real-time status updates during normal operation, including
 * current CO2 concentration, air quality assessment, and warning status.
 *
 * Log format:
 *  - "PPM: [value] | Quality: [label]"
 *  - Appends " | WARNING ACTIVE " when warning system is engaged
 *
 * Parameters:
 *  @param ppm - Current averaged CO2 concentration
 *  @param qualityText - Formatted air quality label
 *
 * Operational use:
 *  - Primary debugging tool during development
 *  - Continuous monitoring during field deployment
 *  - Data collection for performance analysis
 *
 * Note: Serial output should be disabled in production to reduce power
 *       consumption and processing overhead.
 */
void logSensorData(float ppm, String qualityText) {
    Serial.print("PPM: "); Serial.print(ppm,1);
    Serial.print(" | Quality: "); Serial.print(qualityText); Serial.print("  ");
    if (isWarningActive) {
        Serial.print(" | WARNING ACTIVE ");
    }
    //Serial.println();  // Commented to allow custom formatting by caller
}

/**
 * @brief Performs direct sensor reading and updates global variables.
 *
 * Reads both analog and digital outputs from the MQ-135 sensor in a single
 * operation, updating the shared sensor state variables.
 *
 * Updates:
 *  - adc: Raw analog-to-digital converter value (0-1023)
 *  - d0: Digital output state (HIGH/LOW based on threshold)
 *  - sensor_voltage: Calculated analog voltage (0-5V)
 *
 * Electrical interface:
 *  - Analog output (AO): Connected to ADC pin, provides concentration data
 *  - Digital output (DO): Connected to digital pin, provides threshold detection
 *
 * Note: The digital output threshold is factory-set and may not align with
 *       the system's PPM_THRESHOLD. Used primarily as a hardware backup.
 */
void MQ135SensorDirectData() {
    adc = analogRead(CO2_analog_pin);
    d0  = digitalRead(CO2_digital_pin);
    sensor_voltage = adc * (5.0 / 1023.0);
}

//=======================
// Serial Debug Output
//=======================

/**
 * @brief Displays comprehensive sensor status in condensed format.
 *
 * Provides a single-line summary of all critical sensor parameters,
 * optimized for continuous monitoring via Serial plotter or terminal.
 *
 * Displayed metrics:
 *  - ADC: Raw sensor reading (0-1023)
 *  - D0: Digital threshold state (0/1)
 *  - V: Sensor voltage (0-5V, 3 decimal places)
 *  - Rs: Sensor resistance (kΩ, 2 decimal places)
 *  - R0: Baseline resistance (kΩ, 2 decimal places)
 *  - PPM: Estimated CO2 concentration (1 decimal place)
 *
 * Design notes:
 *  - Fixed field widths for consistent column alignment
 *  - Limited precision to reduce Serial bandwidth
 *  - Single line format compatible with data logging
 *
 * Note: Commented code shows optional moving average and dynamic R0
 *       calculations for advanced diagnostics.
 */
void debugSensor() {
    float Rs = calculateRs(sensor_voltage);
    float ppm = calculatePPM(sensor_voltage);
    
    Serial.print("ADC: "); Serial.print(adc);
    Serial.print(" | D0: "); Serial.print(d0);
    Serial.print(" | V: "); Serial.print(sensor_voltage, 3);
    Serial.print(" | Rs: "); Serial.print(Rs, 2);
    Serial.print(" kΩ | R0: "); Serial.print(R0, 2);
    Serial.print(" kΩ | PPM: "); Serial.print(ppm, 1);
    
    // Optional extended diagnostics (commented):
    // Serial.print(" | PPM(shortMA): "); Serial.print(ppmMAshort, 1);
    // Serial.print(" | PPM(longMA): "); Serial.print(ppmMAlong, 1);
    // Serial.print(" | runningR0: "); Serial.print(runningR0, 2);
    
    Serial.println();
}

//=======================
// LCD Debug Output 
//=======================

/**
 * @brief Displays real-time sensor data on the LCD screen.
 *
 * Provides a compact, at-a-glance view of key sensor parameters for
 * field diagnostics without requiring a computer connection.
 *
 * LCD layout (16x2):
 *  - Line 1: "RS:[value] ADC:[value]"
 *  - Line 2: "RO:[value] PPM:[value]"
 *
 * Display characteristics:
 *  - Whole numbers only (no decimal points for readability)
 *  - Fixed field positions for consistency
 *  - Complete screen refresh each call
 *
 * Use case:
 *  - Quick verification of sensor operation
 *  - Field calibration assistance
 *  - Troubleshooting without serial connection
 *
 * Note: LCD updates are relatively slow (~100ms). Avoid calling in
 *       time-critical code sections.
 */
void lcdDebug() {
    int adc = analogRead(CO2_analog_pin);
    float voltage = adc * (5.0 / 1023.0);
    float Rs = calculateRs(voltage);
    float ppm = calculatePPM(voltage);
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("RS:"); lcd.print(Rs, 0);
    lcd.print(" ADC:"); lcd.print(adc);
    
    lcd.setCursor(0, 1);
    lcd.print("RO:"); lcd.print(R0, 0);
    lcd.print(" PPM:"); lcd.print(ppm, 0);
}