#include <Arduino.h>

int sensorValue; // (int variable to read analogue output reading)
int digitalValue; // (int variable to read digital output reading)

int CO2_analog_pin = 0; // Air Quality Sensor analog pin on pin 0
int CO2_digital_pin = 2; // Air Quality Sensor digital pin on pin 2
int LED_output = 13; // LED activation pin on pin 13

int AirQualityThreshold = 400; // Air quality ppm sensor threshold activation 

void setup() {
  Serial.begin(9600); // sets the serial port to 9600 (sets the serial communication to 9600 baud rate)
  pinMode(13, OUTPUT); // (pin 13 is connected to the anode terminal of the LED as an output)
  pinMode(2, INPUT); // (pin 2 of Arduino is connected to the Do pin of the MQ135 as an input)
}

void loop() {
  sensorValue = analogRead(CO2_analog_pin); // read analogue input pin 0 (to read the analogue input on Ao)
  digitalValue = digitalRead(CO2_digital_pin); //(to read and save the digital output on pin 2 of Arduino)
  if (sensorValue > AirQualityThreshold) {
    digitalWrite(LED_output, HIGH); // (if the analogue reading is greater than 400, then the LED turns ON)
  } else {
    digitalWrite(LED_output, LOW); //(if the analogue reading is less than 400, the LED turns OFF)
  } 
  Serial.println(sensorValue, DEC); // prints the value read
  Serial.println(digitalValue, DEC);
  delay(1000); // wait 100ms for the next reading   (analogue and digital output readings are displayed on the monitor)
}