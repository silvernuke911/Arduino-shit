#include <Arduino.h>
#include <LiquidCrystal.h>

// LCD pin connections
const int rs = 2;
const int en = 3;
const int d4 = 6;
const int d5 = 7;
const int d6 = 8;
const int d7 = 9;

// Create LCD object
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

void setup() {
  // Initialize serial communication at 9600 bps
  Serial.begin(9600);
  
  // Initialize LCD (16 columns, 2 rows)
  lcd.begin(16, 2);
  
  // Optional: Display startup message
  lcd.setCursor(0, 0);
  lcd.print("Voltage Monitor");
  delay(2000); // Show for 2 seconds
  lcd.clear();
}

void loop() {
  // Read the input on analog pin A0 (connected to potentiometer VO pin)
  int sensorValue = analogRead(A0);
  
  // Convert to voltage (assuming 5V reference)
  float voltage = 5.0 * sensorValue / 1024.0;
  
  // Print to Serial Monitor
  Serial.print("Voltage: ");
  Serial.print(voltage);
  Serial.println(" V");
  
  // Display on LCD
  lcd.setCursor(0, 0);
  lcd.print("Voltage: ");
  lcd.print(voltage, 2); // Display with 2 decimal places
  lcd.print(" V");
  
  // Clear the rest of the line (optional)
  lcd.print("     ");
  
  // Optional: You can add other information on the second line
  lcd.setCursor(0, 1);
  lcd.print("Raw: ");
  lcd.print(sensorValue);
  lcd.print("      "); // Clear rest of line
  
  // Delay between readings
  delay(500);
}