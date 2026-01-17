//============================================================================
//----------------------------------------------------------------------------
//      CARBON DIOXIDE DETECTION SYSTEM
//
//----------------------------------------------------------------------------
//      Arduino Uno - C++
//      January 17, 2026
//      By Silvernuke
//
//============================================================================
//

// Library inclusions
#include <Arduino.h>
#include <LiquidCrystal.h>
#include <Servo.h>


//============================================================================
// HARDWARE PIN CONFIGURATION
//============================================================================

// Air Quality Sensor pins
int CO2_analog_pin = A0;   // Air Quality Sensor analog pin connected to A0
int CO2_digital_pin = 4;   // Air Quality Sensor digital pin connected to pin 4 (D4)
int LED_output = 13;       // LED activation pin on pin 13
int Buzzer_output = 11;    // Buzzer output pin on pin 11

// Servo setup
Servo DoorServo;
const int servoPin = 5;     // Connected to servo signal pin

// LCD pin connections (1602A)
const int rs = 2;
const int en = 3;
const int d4 = 6;     
const int d5 = 7;
const int d6 = 8;
const int d7 = 9;

// Create LCD object
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

//============================================================================
// SENSOR CALIBRATION CONSTANTS 
//============================================================================

float R0 = 76.63;          // Will be calibrated during setup
const float RL = 20.0;     // CORRECTED: Load resistance is 20 kOhms (from datasheet)

// CORRECTED: CO2 calibration constants from your table data
// For CO2:
// - At 10 ppm: Rs/R0 = 3.0
// - At 100 ppm: Rs/R0 = 2.5  
// - At 1000 ppm: Rs/R0 = 1.5
// We can use power law: ppm = a * (Rs/R0)^b

// Using two points from your table to calculate a and b:
// Point 1: (Rs/R0 = 3.0, ppm = 10)
// Point 2: (Rs/R0 = 1.5, ppm = 1000)
// Solving: b = log(ppm2/ppm1) / log(ratio2/ratio1)
//          a = ppm1 / (ratio1^b)

const float CO2_A = 0.358;      // Calculated constant a for CO2
const float CO2_B = -4.248;     // Calculated constant b for CO2

// Alternatively, simpler linear interpolation for accuracy
// At 100ppm NH3 (calibration gas): Rs/R0 = 1.0 (by definition of R0)
// But for CO2 calibration, we need different approach

//============================================================================
// AIR QUALITY THRESHOLDS
//============================================================================

const int PPM_THRESHOLD = 1500;  // 1500 ppm warning threshold

// Air Quality Levels based on PPM
#define QUALITY_GOOD 0        // < 450 ppm (Fresh outdoor air)
#define QUALITY_ALRIGHT 1     // 450-800 ppm (Good indoor air)
#define QUALITY_POOR 2         // 800-1500 ppm (Poor ventilation)
#define QUALITY_DANGEROUS 3   // > 1500 ppm (Dangerous)

//============================================================================
// TIMING CONSTANTS
//============================================================================

const unsigned long PREHEAT_TIME = 20000;          // 20 seconds for sensor preheating
const unsigned long STARTUP_DISPLAY_TIME = 2000;   // 2 seconds for startup message
const unsigned long CALIBRATION_PREP_TIME = 5000;  // 5 seconds to prepare for calibration
bool isPreheated = false;

// Warning state variables
bool isWarningActive = false;
unsigned long warningStartTime = 0;
const unsigned long WARNING_DISPLAY_TIME = 3000;   // 3 seconds for warning display

// Animation timing (2 FPS = 500ms per frame)
const unsigned long ANIMATION_INTERVAL = 500;      // 500ms for 2 FPS

//============================================================================
// SENSOR AVERAGING SYSTEM
//============================================================================

const int SAMPLES_PER_SECOND = 50;          // 50 samples per second
const int SAMPLES_PER_READING = 50;         // Average 50 samples for each reading
float ppmReadings[SAMPLES_PER_READING];     // Circular buffer for averaging
int readingIndex = 0;
unsigned long lastSampleTime = 0;
const unsigned long SAMPLE_INTERVAL = 20;   // 20ms between samples = 50 samples/second

//============================================================================
// FUNCTION DECLARATIONS
//============================================================================

void activateWarningSystem();
void deactivateWarningSystem();
void displayPreheatingAnimation(unsigned long startTime);
int getAirQualityLevel(float ppm);
void warning_buzzer();
String getQualityText(int level);
float readPPM();
float calculatePPM(float sensor_volt);
float calculateRs(float sensor_volt);
void calibrateSensor();
void displayCalibrationPreparation();
float getAveragePPM();
void updatePPMReading();
void debugSensorValues();

//============================================================================
// SETUP FUNCTION
//============================================================================

void setup() {
  Serial.begin(9600);
  
  // Initialize hardware pins
  pinMode(LED_output, OUTPUT);
  pinMode(CO2_digital_pin, INPUT);
  pinMode(Buzzer_output, OUTPUT);
  
  // Initialize LCD
  lcd.begin(16, 2);

  DoorServo.attach(servoPin);
  DoorServo.write(0);  // Start at 0 degrees (closed position)

  // Initialize averaging array
  for (int i = 0; i < SAMPLES_PER_READING; i++) {
    ppmReadings[i] = 0.0;
  }

  // Display system title
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" CO2 Detection  ");
  lcd.setCursor(0, 1);
  lcd.print("     System     ");
  delay(STARTUP_DISPLAY_TIME);
  
  // Sensor preheating phase
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SensorPreheating");
  lcd.setCursor(0, 1);
  lcd.print("Time: 20 s ");
  
  unsigned long preheatStartTime = millis();
  unsigned long lastAnimationTime = 0;
  
  while (millis() - preheatStartTime < PREHEAT_TIME) {
    if (millis() - lastAnimationTime >= ANIMATION_INTERVAL) {
      displayPreheatingAnimation(preheatStartTime);
      lastAnimationTime = millis();
    }
    delay(50);
  }

  // Display calibration info
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("     Sensor     ");
  lcd.setCursor(0, 1);
  lcd.print("   Calibrating  ");
  delay(2000);

  // Sensor calibration
  calibrateSensor();
  
  // System ready
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Ready!");
  delay(2000);
  
  isPreheated = true;
  lastSampleTime = millis();
  
  // Debug initial readings
  debugSensorValues();
}

//============================================================================
// MAIN LOOP
//============================================================================

void loop() {

  if (!isPreheated) return;
  
  // Continuously update PPM readings at 50Hz
  updatePPMReading();
  
  // Process display and logic once per second
  static unsigned long lastProcessTime = 0;
  if (millis() - lastProcessTime >= 1000) {
    lastProcessTime = millis();
    
    // Get averaged PPM value
    float ppm = getAveragePPM();
    
    // Determine air quality level
    int qualityLevel = getAirQualityLevel(ppm);
    String qualityText = getQualityText(qualityLevel);
    
    // Check threshold (1500 ppm)
    bool isAboveThreshold = (ppm > PPM_THRESHOLD);
    
    if (isAboveThreshold) {
      if (!isWarningActive) {
        activateWarningSystem();
        warningStartTime = millis();
        isWarningActive = true;
      }
      warning_buzzer();
      
      if (millis() - warningStartTime < WARNING_DISPLAY_TIME) {
        lcd.setCursor(0, 0);
        lcd.print("    WARNING!    ");
        lcd.setCursor(0, 1);
        lcd.print("HIGH CO2 LEVEL! ");
      } else {
        lcd.setCursor(0, 0);
        lcd.print("CO2: ");
        lcd.print((int)ppm);
        lcd.print(" ppm   ");
        
        lcd.setCursor(0, 1);
        lcd.print(">1500 ppm!     ");
      }
      
    } else {
      if (isWarningActive) {
        deactivateWarningSystem();
        isWarningActive = false;
      }
      
      digitalWrite(LED_output, LOW);
      digitalWrite(Buzzer_output, LOW);
      
      // Display normal air quality information
      lcd.setCursor(0, 0);
      lcd.print("CO2: ");
      lcd.print((int)ppm);
      lcd.print(" ppm   ");
      
      lcd.setCursor(0, 1);
      lcd.print("Quality: ");
      
      // Adjust display for different quality text lengths
      if (qualityText == "Good") {
        lcd.print("Good    ");
      } else if (qualityText == "Alright") {
        lcd.print("Alright ");
      } else if (qualityText == "Poor") {
        lcd.print("Poor     ");
      } else {
        lcd.print(qualityText);
      }
    }
    
    // Serial monitor output
    Serial.print("PPM: ");
    Serial.print(ppm, 1);
    Serial.print(" | Quality: ");
    Serial.print(qualityText);
    
    if (isWarningActive) {
      Serial.print(" | WARNING ACTIVE");
    }
    Serial.println();
  }
}

//============================================================================
// SENSOR READING FUNCTIONS
//============================================================================

void updatePPMReading() {
  if (millis() - lastSampleTime >= SAMPLE_INTERVAL) {
    lastSampleTime = millis();
    
    int rawValue = analogRead(CO2_analog_pin);
    float voltage = rawValue * (5.0 / 1023.0);
    
    float samplePPM = calculatePPM(voltage);
    
    ppmReadings[readingIndex] = samplePPM;
    readingIndex = (readingIndex + 1) % SAMPLES_PER_READING;
  }
}

float getAveragePPM() {
  float sum = 0.0;
  int validSamples = 0;
  
  for (int i = 0; i < SAMPLES_PER_READING; i++) {
    if (ppmReadings[i] > 0) {
      sum += ppmReadings[i];
      validSamples++;
    }
  }
  
  if (validSamples > 0) {
    return sum / validSamples;
  } else {
    return 0.0;
  }
}

float readPPM() {
  int rawValue = analogRead(CO2_analog_pin);
  float voltage = rawValue * (5.0 / 1023.0);
  return calculatePPM(voltage);
}

// Calculate sensor resistance Rs
float calculateRs(float sensor_volt) {
  // Formula from datasheet: Rs = (Vc/VRL - 1) * RL
  // Where Vc = 5V (supply voltage), VRL = sensor_volt
  float Rs = ((5.0 / sensor_volt) - 1.0) * RL;
  return Rs;
}

// Calculate PPM from sensor voltage
float calculatePPM(float sensor_volt) {
  // Calculate sensor resistance Rs
  float Rs = calculateRs(sensor_volt);
  
  // Calculate ratio Rs/R0
  float ratio = Rs / R0;
  
  // Use power law formula for CO2 from your table data
  // ppm = a * (Rs/R0)^b
  // Where a = 0.358, b = -4.248 (calculated from your table)
  float ppm = CO2_A * pow(ratio, CO2_B);
  
  // Alternative: Linear interpolation method (simpler)
  // Using your table data points for CO2:
  // 10 ppm -> Rs/R0 = 3.0
  // 100 ppm -> Rs/R0 = 2.5
  // 1000 ppm -> Rs/R0 = 1.5
  
  // For ratios between known points, we can interpolate
  if (ratio >= 3.0) {
    // Below 10 ppm - use extrapolation
    ppm = 10.0 * pow(ratio / 3.0, -3.0); // Approximate
  } else if (ratio >= 2.5) {
    // Between 10-100 ppm
    ppm = 10.0 + (90.0 * (3.0 - ratio) / 0.5);
  } else if (ratio >= 1.5) {
    // Between 100-1000 ppm  
    ppm = 100.0 + (900.0 * (2.5 - ratio) / 1.0);
  } else {
    // Above 1000 ppm - use extrapolation
    ppm = 1000.0 * pow(ratio / 1.5, -2.0); // Approximate
  }
  
  return ppm;
}

//============================================================================
// CALIBRATION FUNCTIONS
//============================================================================

void calibrateSensor() {
  Serial.println("==========================================");
  Serial.println("        MQ135 SENSOR CALIBRATION");
  Serial.println("  Based on datasheet: R0 @ 100ppm NH3");
  Serial.println("  RL = 20 kΩ, Temperature 20°C, RH 65%");
  Serial.println("==========================================");
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Calibrating...");
  lcd.setCursor(0, 1);
  lcd.print("                "); // Clear second line
  
  Serial.println("Place sensor in CLEAN AIR for calibration...");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Place in clean");
  lcd.setCursor(0, 1);
  lcd.print("air (5 seconds)");
  delay(5000);
  
  // Clear display for calibration progress
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Calibrating...");
  
  // Take multiple readings in clean air
  float sumRs = 0;
  int samples = 50;
  
  for (int i = 0; i < samples; i++) {
    int raw_adc = analogRead(CO2_analog_pin);
    float sensor_volt = raw_adc * (5.0 / 1023.0);
    float Rs = calculateRs(sensor_volt);
    sumRs += Rs;
    
    // Display progress - CORRECTED FORMAT
    lcd.setCursor(0, 1);
    lcd.print(i+1);
    lcd.print("/");
    lcd.print(samples);
    lcd.print(" samples     "); // Clear any leftover characters
    
    // Optional: Also show Rs value
    // lcd.setCursor(9, 1);
    // lcd.print(Rs, 1);
    // lcd.print("k");
    
    delay(100);
  }
  
  // Calculate average Rs in clean air
  float Rs_clean = sumRs / samples;
  
  // Calibration for CO2 in clean air (~400ppm)
  const float CLEAN_AIR_CO2_PPM = 400.0;
  const float CLEAN_AIR_RATIO = 2.17; // Estimated Rs/R0 at 400ppm CO2
  
  R0 = Rs_clean / CLEAN_AIR_RATIO;
  
  Serial.println("==========================================");
  Serial.println("        CALIBRATION RESULTS");
  Serial.print("Average Rs in clean air: ");
  Serial.print(Rs_clean, 2);
  Serial.println(" kΩ");
  Serial.print("Estimated Rs/R0 at 400ppm CO2: ");
  Serial.println(CLEAN_AIR_RATIO, 2);
  Serial.print("Calculated R0: ");
  Serial.print(R0, 2);
  Serial.println(" kΩ");
  Serial.println("==========================================");
  
  // Test the calibration
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Testing cal...");
  
  float testPPM = calculatePPM(analogRead(CO2_analog_pin) * (5.0 / 1023.0));
  Serial.print("Test reading after calibration: ");
  Serial.print(testPPM, 0);
  Serial.println(" ppm (should be ~400)");
  
  lcd.setCursor(0, 1);
  lcd.print("Test: ");
  lcd.print((int)testPPM);
  lcd.print(" ppm");
  delay(2000);
}

//============================================================================
// WARNING SYSTEM FUNCTIONS
//============================================================================

void activateWarningSystem() {
  digitalWrite(LED_output, HIGH);
  DoorServo.write(90);  // Swing open
  delay(500);         // Wait for movement
  Serial.println("WARNING SYSTEM ACTIVATED!");
}

void warning_buzzer() {
  digitalWrite(Buzzer_output, HIGH);
  delay(850);
  digitalWrite(Buzzer_output, LOW);
  delay(50);
}

void deactivateWarningSystem() {
  digitalWrite(LED_output, LOW);
  digitalWrite(Buzzer_output, LOW);
  DoorServo.write(0);   // Swing closed
  delay(500);         // Wait for movement
  lcd.clear();
  Serial.println("Warning system deactivated.");
}

//============================================================================
// DISPLAY FUNCTIONS
//============================================================================

void displayPreheatingAnimation(unsigned long startTime) {
  static int frame = 0;
  String animation[4] = {"|", "/", "-", "\\"};
  
  lcd.setCursor(15, 1);
  lcd.print(animation[frame % 4]);
  
  unsigned long elapsed = millis() - startTime;
  unsigned long remaining = (PREHEAT_TIME - elapsed) / 1000;
  
  lcd.setCursor(0, 1);
  lcd.print("Time: ");
  if (remaining < 10) lcd.print("0");
  lcd.print(remaining);
  lcd.print(" s ");
  
  frame++;
}

//============================================================================
// AIR QUALITY ASSESSMENT FUNCTIONS
//============================================================================

int getAirQualityLevel(float ppm) {
  if (ppm < 450) return QUALITY_GOOD;          // Outdoor fresh air levels
  else if (ppm < 800) return QUALITY_ALRIGHT;  // Good indoor air
  else if (ppm < PPM_THRESHOLD) return QUALITY_POOR; // Poor ventilation
  else return QUALITY_DANGEROUS;               // Dangerous levels
}

String getQualityText(int level) {
  switch (level) {
    case QUALITY_GOOD:
      return "Good";
    case QUALITY_ALRIGHT:
      return "Alright";
    case QUALITY_POOR:
      return "Poor";
    case QUALITY_DANGEROUS:
      return "DANGER";
    default:
      return "Unknown";
  }
}

//============================================================================
// DEBUG FUNCTION
//============================================================================

void debugSensorValues() {
  Serial.println("\n=== SENSOR DIAGNOSTICS ===");
  
  for (int i = 0; i < 3; i++) {
    int rawValue = analogRead(CO2_analog_pin);
    float voltage = rawValue * (5.0 / 1023.0);
    float Rs = calculateRs(voltage);
    float ratio = Rs / R0;
    float ppm = calculatePPM(voltage);
    
    Serial.print("Reading ");
    Serial.print(i+1);
    Serial.print(": ADC=");
    Serial.print(rawValue);
    Serial.print(" (");
    Serial.print(voltage, 3);
    Serial.print("V), Rs=");
    Serial.print(Rs, 2);
    Serial.print(" kΩ, Rs/R0=");
    Serial.print(ratio, 3);
    Serial.print(", PPM=");
    Serial.println(ppm, 1);
    
    delay(1000);
  }
  
  // Show what ratios correspond to what PPM based on your table
  Serial.println("\n=== EXPECTED VALUES ===");
  Serial.println("For CO2:");
  Serial.println("10 ppm  -> Rs/R0 = 3.0");
  Serial.println("100 ppm -> Rs/R0 = 2.5");
  Serial.println("1000 ppm -> Rs/R0 = 1.5");
  Serial.println("=========================\n");
}