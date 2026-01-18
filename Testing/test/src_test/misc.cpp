#include "misc.h"
#include "globals.h"
#include "utils.h"

//---------------------------
// Initialization
//---------------------------
void initializeHardwarePins() {
	pinMode(LED_output, OUTPUT);
	pinMode(CO2_digital_pin, INPUT);
	pinMode(Buzzer_output, OUTPUT);
}

void initializeServo() {
	DoorServo.attach(servoPin);
	DoorServo.write(0);
}

void initializeSensorArray() {
	for (int i=0; i<SAMPLES_PER_READING; i++) {
		ppmReadings[i] = 400;
	}
}

void initializeSensorTiming() {
	isPreheated = true;
	lastSampleTime = millis();
}

void performInitialDiagnostics() {
	debugSensorValues();
}

//---------------------------
// Display / Startup
//---------------------------
void displayStartupMessage() {
	lcd.clear();
	lcd.setCursor(0,0); lcd.print(" CO2 Detection  ");
	lcd.setCursor(0,1); lcd.print("     System     ");
	delay(2000);

	lcd.setCursor(0,0); lcd.print("   by Group 4   ");
	lcd.setCursor(0,1); lcd.print("    CHEM 015    ");
	delay(2000);
}

void displaySystemReady() {
	lcd.clear();
	lcd.setCursor(0,0); lcd.print("System Ready!");
	delay(2000);
}

void performSensorPreheating() {
	if (skipPreheating) {
		return;
	}

	lcd.clear();
	lcd.setCursor(0,0); lcd.print("SensorPreheating");
	lcd.setCursor(0,1); lcd.print("Time: 20 s ");

	unsigned long startTime = millis();
	unsigned long lastAnim = 0;

	while (millis()-startTime < 20000) {
		if (millis()-lastAnim >= 500) {
			displayPreheatingAnimation(startTime);
			lastAnim = millis();
		}
		delay(50);
	}
}

void displayPreheatingAnimation(unsigned long startTime) {
	static int frame=0;
	String animation[4] = {"|","/","-","\\"};
	lcd.setCursor(15,1); lcd.print(animation[frame%4]);

	unsigned long remaining = (20000-(millis()-startTime))/1000;
	lcd.setCursor(0,1); lcd.print("Time: "); if(remaining<10) lcd.print("0"); lcd.print(remaining); lcd.print(" s ");

	frame++;
}
