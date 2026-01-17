//============================================================================
//****************************************************************************
//
//             SMART CARBON DIOXIDE DETECTION & ALERT SYSTEM
//      Real-time CO2 Monitoring with Visual/Auditory Warning System
//
//**************************************************************************** 
//----------------------------------------------------------------------------
//      PLATFORM: Arduino Uno R3
//      SENSOR:   MQ-135 Air Quality Sensor
//      DISPLAY:  16x2 Character LCD (1602A)
//      ACTUATORS: SG90 Servo, Buzzer, R Status LED
//
//----------------------------------------------------------------------------
//      FEATURES:
//      - Real-time CO2 concentration measurement (PPM)
//      - 4-level air quality assessment (Good/Fair/Poor/Dangerous)
//      - Automatic calibration in clean air
//      - 50Hz sampling rate with moving average filtering
//      - Multi-mode warning system (LCD, Buzzer, Servo, LED)
//      - Serial monitor diagnostics and logging
//      - Automatic servo manipulation
//      - Regular recalibration every 5 mins
//
//      LIMITATIONS
//      - 20 second MQ135 Sensort preheating at startup
//      - 5 second Air quality Calibration
//      - Requires clean air (Assumption approx. 400 ppm)
//----------------------------------------------------------------------------
//      AIR QUALITY THRESHOLDS:
//      GOOD      (< 450 PPM)    : Green LED, Normal operation
//      FAIR      (450-800 PPM)  : Blue LED, Acceptable indoor air
//      POOR      (800-1500 PPM) : Yellow LED, Poor ventilation
//      DANGEROUS (> 1500 PPM)   : Red LED, Warning system activated
//
//----------------------------------------------------------------------------
//      ALERT THRESHOLD: 1500 PPM CO2
//      When exceeded:
//      1. SERVO opens 90° for ventilation/access
//      2. BUZZER emits continuous warning pattern
//      3. LED flashes bright red
//      4. LCD displays emergency warning message
//
//----------------------------------------------------------------------------
//      HARDWARE CONNECTIONS:
//      MQ-135 AO  -> A0    | SG90 Servo     -> D5
//      MQ-135 DO  -> D4    | Buzzer         -> D11
//      LCD RS     -> D2    | RGB Red        -> D10
//      LCD EN     -> D3    | RGB Green      -> D11
//      LCD D4-D7  -> D6-D9 | RGB Blue       -> D12
//      Warning LED-> D13   | Common Cathode -> GND
//
//----------------------------------------------------------------------------
//      VERSION: 1.1
//      DATE:    January 17, 2026
//      AUTHOR:  Silvernuke911
//      
//      "Mors vincit omnia"
//
//============================================================================
// TODO: Add manual recalibration button.
//       Arduino house casing
//
//============================================================================

//============================================================================
// LIBRARY INCLUSIONS
//============================================================================
#include <Arduino.h>
#include <LiquidCrystal.h>
#include <Servo.h>

//============================================================================
// HARDWARE PIN CONFIGURATION
//============================================================================

// Air Quality Sensor pins
int CO2_analog_pin = A0;  // MQ135 Analog Pin on pin A0
int CO2_digital_pin = 4;  // MQ135 Digital Pin on pin D4
int LED_output = 13;      // Warning LED input on pin D13
int Buzzer_output = 11;   // Buzzer input on pin D11

// Servo pin
Servo DoorServo;
const int servoPin = 5;   // Servo input on pin D5

// LCD pin connections (1602A)
const int rs = 2;  // RS pin on D2
const int en = 3;  // Enable pin on D3
const int d4 = 6;  // Data pins D4-D7
const int d5 = 7;  // on Uno pin D6-D9
const int d6 = 8;  // Half-byte transfer per cycle
const int d7 = 9;  // 

LiquidCrystal lcd(rs, en, d4, d5, d6, d7); // LCD Object

//============================================================================
// SENSOR CALIBRATION
//============================================================================

// Data from MQ135 Data sheet: 
// https://www.elprocus.com/mq135-air-quality-sensor/

float R0 = 76.63;
const float RL = 20.0;     // Load resistance: 20 kOhms

// CO2 calibration from table data:
// 10 ppm: Rs/R0 = 3.0
// 100 ppm: Rs/R0 = 2.5  
// 1000 ppm: Rs/R0 = 1.5

const float CO2_A = 0.358;
const float CO2_B = -4.248;

//============================================================================
// AIR QUALITY THRESHOLDS
//============================================================================
// Based from this: 
// https://www.co2meter.com/blogs/news/carbon-dioxide-indoor-levels-chart

const int PPM_THRESHOLD = 2000;

#define QUALITY_GOOD 0        // < 450 ppm
#define QUALITY_FAIR 1        // 450-800 ppm
#define QUALITY_POOR 2        // 800-1500 ppm
#define QUALITY_DANGEROUS 3   // > 1500 ppm

//============================================================================
// TIMING CONSTANTS
//============================================================================

const unsigned long PREHEAT_TIME = 20000;
const unsigned long STARTUP_DISPLAY_TIME = 2000;
const unsigned long CALIBRATION_PREP_TIME = 5000;
bool isPreheated = false;
bool skipPreheating = true;
bool isWarningActive = false;
unsigned long warningStartTime = 0;
const unsigned long WARNING_DISPLAY_TIME = 3000;

const unsigned long ANIMATION_INTERVAL = 500;

//============================================================================
// CALIBRATION TIMING
//============================================================================

const unsigned long RECALIBRATION_INTERVAL = 300000;  // 5 mins= 300,000 ms
unsigned long lastCalibrationTime = 0;
bool recalibrationDue = false;

// Keep track of original R0 for reference
float originalR0 = 76.63;

//============================================================================
// SENSOR AVERAGING SYSTEM
//============================================================================

const int SAMPLES_PER_READING = 50;
float ppmReadings[SAMPLES_PER_READING];
int readingIndex = 0;
unsigned long lastSampleTime = 0;
const unsigned long SAMPLE_INTERVAL = 20;

//============================================================================
// FUNCTION DECLARATIONS
//============================================================================
// Warning System Functions
void activateWarningSystem();
void deactivateWarningSystem();
void warning_buzzer();
// Preheating Display Function
void displayPreheatingAnimation(unsigned long startTime);
// Air Quality Assessment Functions
int getAirQualityLevel(float ppm);
String getQualityText(int level);
// Sensor Reading Functions
float calculatePPM(float sensor_volt);
float calculateRs(float sensor_volt);
float getAveragePPM();
void updatePPMReading();
// Calibration Functions
void calibrateSensor();
void calibrateInitWaiting();
void performRegularRecalibration();
// Debug Function
void debugSensorValues();
// Setup Functions
void initializeHardwarePins();
void initializeServo();
void initializeSensorArray();
void displayStartupMessage();
void performSensorPreheating();
void performSensorCalibration();
void displaySystemReady();
void initializeSensorTiming();
void performInitialDiagnostics();
void checkRecalibration();
// Data Processing Functions
void handleWarningState(float ppm, String qualityText);
void handleNormalState(float ppm, String qualityText);
void displayWarningMessage(float ppm);
void displayNormalMessage(float ppm, String qualityText);
void logSensorData(float ppm, String qualityText);

//============================================================================
// SETUP FUNCTION
//============================================================================

void setup() {
  Serial.begin(9600);
  initializeHardwarePins();
  lcd.begin(16, 2);
  initializeServo();
  initializeSensorArray();
  displayStartupMessage();
  performSensorPreheating();
  calibrateInitWaiting();
  performSensorCalibration();
  originalR0 = R0;
  lastCalibrationTime = millis();  // Start calibration timer
  displaySystemReady();
  initializeSensorTiming();
  performInitialDiagnostics();
}

//============================================================================
// MAIN LOOP
//============================================================================

void loop() {
  if (!isPreheated) return;
  updatePPMReading();
  static unsigned long lastProcessTime = 0;
  if (millis() - lastProcessTime >= 1000) {
    lastProcessTime = millis();
    checkRecalibration();
    if (recalibrationDue) {
      performRegularRecalibration();
    }
    float ppm = getAveragePPM();
    int qualityLevel = getAirQualityLevel(ppm);       // rolling average of 50 samples in the past second
    String qualityText = getQualityText(qualityLevel);
    
    bool isAboveThreshold = (ppm > PPM_THRESHOLD);
    
    if (isAboveThreshold) {
      handleWarningState(ppm, qualityText);
    } else {
      handleNormalState(ppm, qualityText);
    }
    logSensorData(ppm, qualityText);
  }
}


//============================================================================
// INITIALIZATION FUNCTIONS
//============================================================================

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
  for (int i = 0; i < SAMPLES_PER_READING; i++) {
    ppmReadings[i] = 0.0;
  }
}

//============================================================================
// STARTUP SEQUENCE FUNCTIONS
//============================================================================

void displayStartupMessage() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" CO2 Detection  ");
  lcd.setCursor(0, 1);
  lcd.print("     System     ");
  delay(2000);
  lcd.setCursor(0, 0);
  lcd.print("   by Group 4   ");
  lcd.setCursor(0, 1);
  lcd.print("    CHEM 015    ");
  delay(2000);
}

void performSensorPreheating() {
  lcd.clear();
  if (skipPreheating) {
    return;
  }
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
}

void performSensorCalibration() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("     Sensor     ");
  lcd.setCursor(0, 1);
  lcd.print("   Calibrating  ");
  delay(2000);

  calibrateSensor();
}

void displaySystemReady() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Ready!");
  delay(2000);
}

void initializeSensorTiming() {
  isPreheated = true;
  lastSampleTime = millis();
}

void performInitialDiagnostics() {
  debugSensorValues();
}

//============================================================================
// DATA PROCESSING FUNCTIONS
//============================================================================

void handleWarningState(float ppm, String qualityText) {
  if (!isWarningActive) {
    activateWarningSystem();
    warningStartTime = millis();
    isWarningActive = true;
  }
  
  warning_buzzer();
  displayWarningMessage(ppm);
}

void handleNormalState(float ppm, String qualityText) {
  if (isWarningActive) {
    deactivateWarningSystem();
    isWarningActive = false;
  }
  
  digitalWrite(LED_output, LOW);
  digitalWrite(Buzzer_output, LOW);
  displayNormalMessage(ppm, qualityText);
}

void displayWarningMessage(float ppm) {
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
    lcd.print(">");
    lcd.print(PPM_THRESHOLD); 
    lcd.print(" ppm!     ");
  }
}

void displayNormalMessage(float ppm, String qualityText) {
  lcd.setCursor(0, 0);
  lcd.print("CO2: ");
  lcd.print((int)ppm);
  lcd.print(" ppm   ");
  
  lcd.setCursor(0, 1);
  lcd.print("Quality: ");
  
  if (qualityText == "Good") {
    lcd.print("Good    ");
  } else if (qualityText == "Fair") {
    lcd.print("Fair    ");
  } else if (qualityText == "Poor") {
    lcd.print("Poor    ");
  } else {
    lcd.print(qualityText);
  }
}

void logSensorData(float ppm, String qualityText) {
  Serial.print("PPM: ");
  Serial.print(ppm, 1);
  Serial.print(" | Quality: ");
  Serial.print(qualityText);
  
  if (isWarningActive) {
    Serial.print(" | WARNING ACTIVE");
  }
  Serial.println();
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

float calculateRs(float sensor_volt) {
  float Rs = ((5.0 / sensor_volt) - 1.0) * RL;
  return Rs;
}

float calculatePPM(float sensor_volt) {
  float Rs = calculateRs(sensor_volt);
  float ratio = Rs / R0;
  
  float ppm = CO2_A * pow(ratio, CO2_B);
  
  if (ratio >= 3.0) {
    ppm = 10.0 * pow(ratio / 3.0, -3.0);
  } else if (ratio >= 2.5) {
    ppm = 10.0 + (90.0 * (3.0 - ratio) / 0.5);
  } else if (ratio >= 1.5) {
    ppm = 100.0 + (900.0 * (2.5 - ratio) / 1.0);
  } else {
    ppm = 1000.0 * pow(ratio / 1.5, -2.0);
  }
  
  return ppm;
}

//============================================================================
// CALIBRATION FUNCTIONS
//============================================================================
void calibrateInitWaiting() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Place in clean");
  lcd.setCursor(0, 1);
  lcd.print("air (5 seconds)");
  delay(5000);
}
void calibrateSensor() {
  // Assumes the outside air quality is approx 400 ppm
  Serial.println("==========================================");
  Serial.println("        MQ135 SENSOR CALIBRATION");
  Serial.println("==========================================");
  

  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Calibrating...");
  
  float sumRs = 0;
  int samples = 50;
  
  for (int i = 0; i < samples; i++) {
    int raw_adc = analogRead(CO2_analog_pin);
    float sensor_volt = raw_adc * (5.0 / 1023.0);
    float Rs = calculateRs(sensor_volt);
    sumRs += Rs;
    
    lcd.setCursor(0, 1);
    if (i+1 < 10) {
      lcd.print("0");
    }
    lcd.print(i+1);
    lcd.print("/");
    lcd.print(samples);
    lcd.print(" samples     ");
    
    delay(100);
  }
  
  float Rs_clean = sumRs / samples;
  const float CLEAN_AIR_RATIO = 2.17;
  R0 = Rs_clean / CLEAN_AIR_RATIO;
  
  Serial.print("Average Rs: ");
  Serial.print(Rs_clean, 2);
  Serial.println(" kOhms");
  Serial.print("Calculated R0: ");
  Serial.print(R0, 2);
  Serial.println(" kOhms");
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Testing cal...");
  
  float testPPM = calculatePPM(analogRead(CO2_analog_pin) * (5.0 / 1023.0));
  
  lcd.setCursor(0, 1);
  lcd.print("Test: ");
  lcd.print((int)testPPM);
  lcd.print(" ppm");
  delay(2000);
}

//============================================================================
// RECALIBRATION FUNCTIONS
//============================================================================

void checkRecalibration() {
  unsigned long currentTime = millis();
  
  // Handle millis() overflow (every ~50 days)
  if (currentTime < lastCalibrationTime) {
    lastCalibrationTime = currentTime;
    return;
  }
  
  // Check if recalibration is due
  if (currentTime - lastCalibrationTime >= RECALIBRATION_INTERVAL) {
    recalibrationDue = true;
    Serial.println("Recalibration due! Place sensor in clean air.");
  }
}

void performRegularRecalibration() {
  if (!recalibrationDue) return;
  
  Serial.println("\n==========================================");
  Serial.println("       REGULAR RECALIBRATION STARTED   ");
  Serial.println("==========================================");
  
  // Save current reading before recalibration
  float previousPPM = getAveragePPM();
  
  // Display message on LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Rglr Recalib");
  lcd.setCursor(0, 1);
  lcd.print("Place clean air");
  delay(2000);
  
  // Wait for user to place sensor in clean air
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Starting in...");
  
  for (int i = 3; i > 0; i--) {
    lcd.setCursor(0, 1);
    lcd.print(i);
    lcd.print(" seconds     ");
    delay(1000);
  }
  
  // Perform calibration
  calibrateSensor();
  
  // Update calibration time
  lastCalibrationTime = millis();
  recalibrationDue = false;
  
  // Display results
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Recalib Done!");
  lcd.setCursor(0, 1);
  lcd.print("R0: ");
  lcd.print(R0, 1);
  lcd.print("k");
  
  Serial.print("Previous R0: ");
  Serial.print(originalR0, 2);
  Serial.print(" kΩ | New R0: ");
  Serial.print(R0, 2);
  Serial.println(" kΩ");
  
  originalR0 = R0;  // Update reference
  
  delay(2000);
  lcd.clear();
  
  Serial.println("Recalibration complete!");
  Serial.println("==========================================\n");
}

void quickRecalibrationCheck() {
  // Quick calibration check without user intervention
  Serial.println("\n=== QUICK CALIBRATION CHECK ===");
  
  float sumRs = 0;
  int quickSamples = 10;
  
  for (int i = 0; i < quickSamples; i++) {
    int raw_adc = analogRead(CO2_analog_pin);
    float sensor_volt = raw_adc * (5.0 / 1023.0);
    float Rs = calculateRs(sensor_volt);
    sumRs += Rs;
    delay(100);
  }
  
  float avgRs = sumRs / quickSamples;
  float calculatedR0 = avgRs / 2.17;  // Using same clean air ratio
  
  Serial.print("Quick check - Current Rs: ");
  Serial.print(avgRs, 2);
  Serial.print(" kΩ, Calculated R0: ");
  Serial.print(calculatedR0, 2);
  Serial.print(" kΩ (");
  Serial.print((calculatedR0 / originalR0 - 1) * 100, 1);
  Serial.println("% change)");
  
  // If change is significant (>10%), suggest recalibration
  float changePercent = abs((calculatedR0 / originalR0 - 1)) * 100;
  if (changePercent > 10) {
    Serial.println("WARNING: Significant sensor drift detected!");
    Serial.println("Consider performing full recalibration.");
  }
}

//============================================================================
// WARNING SYSTEM FUNCTIONS
//============================================================================

void activateWarningSystem() {
  digitalWrite(LED_output, HIGH);
  DoorServo.write(90);
  delay(500);
  Serial.println("WARNING SYSTEM ACTIVATED!");
}

void warning_buzzer() {
  digitalWrite(Buzzer_output, HIGH);
  delay(500);
  digitalWrite(Buzzer_output, LOW);
  delay(50);
}

void deactivateWarningSystem() {
  digitalWrite(LED_output, LOW);
  digitalWrite(Buzzer_output, LOW);
  DoorServo.write(0);
  delay(500);
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
  if (ppm < 450) return QUALITY_GOOD;
  else if (ppm < 800) return QUALITY_FAIR;
  else if (ppm < PPM_THRESHOLD) return QUALITY_POOR;
  else return QUALITY_DANGEROUS;
}

String getQualityText(int level) {
  switch (level) {
    case QUALITY_GOOD:
      return "Good";
    case QUALITY_FAIR:
      return "Fair";
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
    Serial.print(" V=");
    Serial.print(voltage, 3);
    Serial.print("V Rs=");
    Serial.print(Rs, 2);
    Serial.print("k Rs/R0=");
    Serial.print(ratio, 3);
    Serial.print(" PPM=");
    Serial.println(ppm, 1);
    
    delay(1000);
  }
  
  Serial.println("\n=== EXPECTED VALUES ===");
  Serial.println("10 ppm: Rs/R0 = 3.0");
  Serial.println("100 ppm: Rs/R0 = 2.5");
  Serial.println("1000 ppm: Rs/R0 = 1.5");
  Serial.println("=========================\n");
}