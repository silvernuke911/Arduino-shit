#include "calib.h"
#include "globals.h"
#include "utils.h"
#include <Arduino.h>
#include <math.h>

//---------------------------
// Calibration
//---------------------------
void calibrateInitWaiting() {
	lcd.clear();
	lcd.setCursor(0,0); lcd.print("Place in clean");
	lcd.setCursor(0,1); lcd.print("air (5 seconds)");
	delay(5000);
}

void calibrateSensor() {
	lcd.clear(); 
	lcd.setCursor(0,0); 
	lcd.print("Calibrating...");
	delay(2000);

	float sumRs=0; 
	int samples=50;
	for(int i = 0; i < samples; i++){
		int raw = analogRead(CO2_analog_pin);
		float volt = raw*(5.0/1023.0);
		sumRs += calculateRs(volt);

		// display progress
		lcd.setCursor(0,1); 
		lcd.print(i+1); 
		lcd.print("/"); 
		lcd.print(samples); 
		lcd.print(" samples     ");
		delay(100);
	}
	float Rs_clean = sumRs/samples;
	R0 = Rs_clean/1.8;
	float testPPM = calculatePPM(analogRead(CO2_analog_pin)*(5.0/1023.0));

	lcd.setCursor(0,1); 
	lcd.print("Test: "); 
	lcd.print((int)testPPM); 
	lcd.print(" ppm"); 
	delay(2000);
}

void checkRecalibration() {
	unsigned long currentTime = millis();
	if(currentTime < lastCalibrationTime) {
		lastCalibrationTime = currentTime;
	}
	if(currentTime - lastCalibrationTime >= 300000) {
		recalibrationDue = true;
	}
}

void performRegularRecalibration() {
	if(!recalibrationDue) {
		return;
	}

	lcd.clear(); 
	lcd.setCursor(0,0); lcd.print(" Rglr Recalib  ");
	lcd.setCursor(0,1); lcd.print("Place clean air");
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
	if(abs((R0calc/originalR0-1))*100>10) Serial.println("WARNING: Sensor drift!");
}
