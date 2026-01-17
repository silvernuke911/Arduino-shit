#include <Arduino.h>

int CO2_analog_pin = A0;   // Air Quality Sensor analog pin connected to A0
int CO2_digital_pin = 2;  // Air Quality Sensor digital pin connected to pin 2
int LED_output = 13;      // LED activation pin on pin 13
int Buzzer_output = 11;   // Buzzer output pin on pin 11

int AirQualityThreshold = 20;  // ADC threshold value (RAW analog reading, NOT CO2 ppm)

void warning_buzzer(); // function declaration for buzzer warning

void setup() {
  Serial.begin(9600);  // sets the serial port to 9600 (sets the serial communication to 9600 baud rate)
  pinMode(LED_output, OUTPUT); // pin 13 is connected to the anode terminal of the LED as an output
  pinMode(CO2_digital_pin, INPUT);  // pin 2 of Arduino is connected to the Do pin of the MQ135 as an input
  pinMode(Buzzer_output, OUTPUT);  // pin 11 is connected to the buzzer as an output
}

void loop() {
  int sensorValue = analogRead(CO2_analog_pin); // read analogue input pin A0 (reads the analogue output Ao of MQ135)
  int digitalValue = digitalRead(CO2_digital_pin); // read and save the digital output from the MQ135 module

  if (sensorValue > AirQualityThreshold) {
    digitalWrite(LED_output, HIGH);  // if the analogue reading is greater than the threshold, turn LED ON
    warning_buzzer(); // activate buzzer warning when air quality exceeds threshold
  } else {
    digitalWrite(LED_output, LOW); // if the analogue reading is below the threshold, turn LED OFF
    digitalWrite(Buzzer_output, LOW); // ensure buzzer is OFF when air quality is safe
  }

  Serial.print("ADC: "); // prints the raw analogue value read from the sensor
  Serial.println(sensorValue); 
  Serial.print("Digital: ");
  Serial.println(digitalValue); 
  // prints the digital output state from the MQ135 module

  delay(500); 
  // wait 500 ms before taking the next reading
}

void warning_buzzer() { // function to generate buzzer warning sound
  digitalWrite(Buzzer_output, HIGH); // turn buzzer ON
  delay(750);
  digitalWrite(Buzzer_output, LOW);  // turn buzzer OFF
  delay(250);
}
