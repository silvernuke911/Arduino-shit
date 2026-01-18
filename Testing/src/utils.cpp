#include "utils.h"
#include "globals.h"

//====================================================
// Sensor Reading
//====================================================
/* 
 * updatePPMReading
 * ----------------
 * Samples the MQ135 sensor every 20ms and stores the PPM value
 * into a circular buffer for averaging.
 *
 * Uses:
 *   - sensor_voltage : current sensor voltage reading
 *   - ppmReadings[]  : array holding recent PPM readings
 *   - readingIndex   : current index for storing new reading
 *   - SAMPLES_PER_READING : size of the buffer
 */
void updatePPMReading() {
	if (millis() - lastSampleTime >= 20) {
		lastSampleTime = millis();
		float samplePPM = calculatePPM(sensor_voltage);
		ppmReadings[readingIndex] = samplePPM;
		readingIndex = (readingIndex + 1) % SAMPLES_PER_READING;
	}
}

/* 
 * getAveragePPM
 * -------------
 * Returns the average of valid PPM readings in the circular buffer.
 * Ignores zero or uninitialized readings.
 *
 * Returns:
 *   float : average PPM value
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

/* 
 * calculateRs
 * -----------
 * Calculates the sensor resistance Rs based on the sensor voltage.
 *
 * Parameters:
 *   sensor_volt : float, voltage read from MQ135 sensor
 *
 * Returns:
 *   float : sensor resistance Rs in kΩ
 */
float calculateRs(float sensor_volt) {	
	return ((5.0 / sensor_volt) - 1.0) * RL;
}

/* 
 * calculatePPM
 * ------------
 * Calculates the estimated CO2 concentration in parts per million (PPM)
 * from the sensor voltage using the MQ135 formula and baseline resistance R0.
 *
 * Parameters:
 *   sensor_volt : float, voltage read from MQ135 sensor
 *
 * Returns:
 *   float : estimated CO2 concentration in PPM
 */
float calculatePPM(float sensor_volt) {
	float Rs = calculateRs(sensor_volt);
	float ratio = Rs / R0;
	return 400.0f * pow(1.8f / ratio, 10.0f);
}

//============================================================================
// AIR QUALITY THRESHOLDS
//============================================================================
// Based from this: 
// https://www.co2meter.com/blogs/news/carbon-dioxide-indoor-levels-chart
 /* -----------------
 * Determines air quality level based on CO2 PPM.
 *
 * Levels:
 *   0 : GOOD       (ppm < 450)
 *   1 : FAIR       (ppm < 800)
 *   2 : POOR       (ppm < PPM_THRESHOLD)
 *   3 : DANGEROUS  (ppm >= PPM_THRESHOLD)
 *
 * Parameters:
 *   ppm : float, CO2 concentration
 *
 * Returns:
 *   int : air quality level
 */
int getAirQualityLevel(float ppm) {
	if (ppm < 450) return 0;       // GOOD
	else if (ppm < 800) return 1;  // FAIR
	else if (ppm < PPM_THRESHOLD) return 2; // POOR
	else return 3;                  // DANGEROUS
}

/* 
 * getQualityText
 * --------------
 * Returns a descriptive string corresponding to the air quality level.
 *
 * Parameters:
 *   level : int, air quality level
 *
 * Returns:
 *   String : textual description of air quality
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
/* 
 * debugSensorValues
 * -----------------
 * Reads the sensor multiple times and prints detailed diagnostics
 * including ADC values, voltage, Rs, Rs/R0 ratio, and PPM to Serial.
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

/* 
 * logSensorData
 * -------------
 * Logs the PPM reading and air quality text to Serial. Optionally shows warning
 * status if isWarningActive is true.
 *
 * Parameters:
 *   ppm : float, CO2 concentration
 *   qualityText : String, textual air quality description
 */
void logSensorData(float ppm, String qualityText) {
	Serial.print("PPM: "); Serial.print(ppm,1);
	Serial.print(" | Quality: "); Serial.print(qualityText); Serial.print("  ");
	if (isWarningActive) {
		Serial.print(" | WARNING ACTIVE ");
	}
	//Serial.println();
}

/* 
 * MQ135SensorDirectData
 * --------------------
 * Reads analog and digital pins of MQ135 sensor and updates global variables:
 * adc, d0, sensor_voltage.
 */
void MQ135SensorDirectData() {
	adc = analogRead(CO2_analog_pin);
	d0  = digitalRead(CO2_digital_pin);
	sensor_voltage = adc * (5.0 / 1023.0);
}

//=======================
// Serial Debug Output
//=======================
/* 
 * debugSensor
 * -----------
 * Prints a formatted sensor status line including:
 * ADC value, digital output, sensor voltage, Rs, R0, and PPM.
 */
void debugSensor() {
	float Rs = calculateRs(sensor_voltage);
	float ppm = calculatePPM(sensor_voltage);
	// float ppmMAshort = updateMA(ppmHistoryShort, SAMPLES_SHORT, idxShort, ppm);
	// float ppmMAlong  = updateMA(ppmHistoryLong,  SAMPLES_LONG, idxLong, ppm);
	// float runningR0 = calculateRunningR0(Rs);

    Serial.print("ADC: "); Serial.print(adc);
    Serial.print(" | D0: "); Serial.print(d0);
    Serial.print(" | V: "); Serial.print(sensor_voltage, 3);
    Serial.print(" | Rs: "); Serial.print(Rs, 2);
    Serial.print(" kΩ | R0: "); Serial.print(R0, 2);
    Serial.print(" kΩ | PPM: "); Serial.print(ppm, 1);
    // Serial.print(" | PPM(shortMA): "); Serial.print(ppmMAshort, 1);
    // Serial.print(" | PPM(longMA): "); Serial.print(ppmMAlong, 1);
    // Serial.print(" | runningR0: "); Serial.print(runningR0, 2);
    Serial.println();
}

//=======================
// LCD Debug Output 
//=======================
/* 
 * lcdDebug
 * --------
 * Displays real-time sensor information on the connected LCD:
 * - Rs value
 * - ADC value
 * - R0 value
 * - PPM value
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