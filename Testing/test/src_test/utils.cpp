#include "utils.h"
#include "globals.h"

//---------------------------
// Sensor Reading
//---------------------------
void updatePPMReading() {
	if (millis() - lastSampleTime >= 20) {
		lastSampleTime = millis();
		int rawValue = analogRead(CO2_analog_pin);
		float voltage = rawValue * (5.0 / 1023.0);
		float samplePPM = calculatePPM(voltage);
		ppmReadings[readingIndex] = samplePPM;
		readingIndex = (readingIndex + 1) % SAMPLES_PER_READING;
	}
}

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

float calculateRs(float sensor_volt) {	
	return ((5.0 / sensor_volt) - 1.0) * RL;
}

float calculatePPM(float sensor_volt) {
	float Rs = calculateRs(sensor_volt);
	float ratio = Rs / R0;
	return 400.0f * pow(1.8f / ratio, 10.0f);
}

//---------------------------
// Air Quality Assessment
//---------------------------
int getAirQualityLevel(float ppm) {
	if (ppm < 450) return 0;       // GOOD
	else if (ppm < 800) return 1;  // FAIR
	else if (ppm < PPM_THRESHOLD) return 2; // POOR
	else return 3;                  // DANGEROUS
}

String getQualityText(int level) {
	switch(level) {
		case 0: return "Good";
		case 1: return "Fair";
		case 2: return "Poor";
		case 3: return "DANGER";
		default: return "Unknown";
	}
}

//---------------------------
// Debugging & Logging
//---------------------------
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
}

void logSensorData(float ppm, String qualityText) {
	Serial.print("PPM: "); Serial.print(ppm,1);
	Serial.print(" | Quality: "); Serial.print(qualityText);
	if (isWarningActive) Serial.print(" | WARNING ACTIVE");
	Serial.println();
}


