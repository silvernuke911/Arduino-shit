#include "response.h"
#include "globals.h"
#include "misc.h"

//---------------------------
// Warning/Normal Handling
//---------------------------
void handleWarningState(float ppm, String qualityText){
	if(!isWarningActive){
		 activateWarningSystem(); 
		 isWarningActive=true; 
	}
	warning_buzzer();
	displayWarningMessage(ppm);
}

void handleNormalState(float ppm, String qualityText){
	if(isWarningActive){ 
		deactivateWarningSystem(); 
		isWarningActive=false; 
	}
	digitalWrite(LED_output,LOW);
	digitalWrite(Buzzer_output,LOW);
	displayNormalMessage(ppm,qualityText);
}

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
		lcd.print(" ppm   ");

		lcd.setCursor(0,1);
		lcd.print(">");
		lcd.print(PPM_THRESHOLD);
		lcd.print(" ppm!     ");
	}
}

void displayNormalMessage(float ppm, String qualityText){
	lcd.setCursor(0,0); lcd.print("CO2: "); 
	lcd.print((int)ppm); lcd.print(" ppm   ");
	lcd.setCursor(0,1); lcd.print("Quality: "); 
	lcd.print(qualityText);
}

//---------------------------
// Warning System Control
//---------------------------
void activateWarningSystem(){ 
	digitalWrite(LED_output,HIGH); 
	DoorServo.write(90); 
	delay(500); 
}

void warning_buzzer(){ 
	digitalWrite(Buzzer_output,HIGH); 
	delay(500); digitalWrite(Buzzer_output,LOW); 
	delay(50);
}

void deactivateWarningSystem(){ 
	digitalWrite(LED_output,LOW); 
	digitalWrite(Buzzer_output,LOW); 
	DoorServo.write(0); delay(500); 
}
