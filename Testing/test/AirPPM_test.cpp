#include <Arduino.h>
#include <LiquidCrystal.h>
#include <math.h>

//============================================================================
// LCD
//============================================================================
LiquidCrystal lcd(2, 3, 6, 7, 8, 9);

//============================================================================
// MQ135
//============================================================================
const int MQ_PIN = A0;    // analog input
const int MQ_DIGITAL = 4; // digital output
const float VCC = 5.0;
const float RL  = 20.0;     // kΩ
const float TARGET_RATIO = 1.8;

//============================================================================
// Calibration
//============================================================================
float R0 = 0.0;
bool calibrated = false;

//============================================================================
// Moving averages
//============================================================================
const int MA_SHORT = 5;      // Short-term moving average (~fast spikes)
const int MA_LONG  = 50;     // Long-term moving average (~baseline)
float ppmHistoryShort[MA_SHORT];
int idxShort = 0;
float ppmHistoryLong[MA_LONG];
int idxLong = 0;

//============================================================================
// Alarm tracking
//============================================================================
const float PPM_THRESHOLD = 2000.0;  // dangerous level
const int CONSECUTIVE_HIGH = 3;
int consecutiveHigh = 0;

//============================================================================
// Calculations
//============================================================================
float calculateRs(float v) {
  return ((VCC / v) - 1.0) * RL;
}

float calculatePPM(float v) {
  float Rs = calculateRs(v);
  float ratio = Rs / R0;
  return 400.0f * pow(1.8f / ratio, 10.0f);
}

float updateMA(float *history, int size, int &idx, float value) {
  history[idx++] = value;
  if (idx >= size) idx = 0;

  float sum = 0;
  for (int i = 0; i < size; i++) sum += history[i];
  return sum / size;
}

// Calculate a "running R0" assuming the air is at baseline_ppm
float calculateRunningR0(float Rs, float baseline_ppm=500.0) {
  float ratio = 1.8 / pow(baseline_ppm / 400.0, 0.1); // Rs/R0 = 1.8 / (ppm/400)^0.1
  return Rs / ratio;
}

//============================================================================
// Setup
//============================================================================
void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.clear();
  pinMode(MQ_DIGITAL, INPUT);

  //================ INITIALIZE MOVING AVERAGES WITH BASELINE =================
  const float BASELINE_PPM = 400.0;
  for (int i = 0; i < MA_SHORT; i++) ppmHistoryShort[i] = BASELINE_PPM;
  for (int i = 0; i < MA_LONG; i++)  ppmHistoryLong[i]  = BASELINE_PPM;
  idxShort = 0;
  idxLong  = 0;

  //================ FIXED R0 CALIBRATION =================
  Serial.println("Calibrating R0 for 5 seconds (baseline assumed 400 ppm)...");
  unsigned long start = millis();
  int samples = 0;
  float sumR0 = 0.0;

  while (millis() - start < 5000) { // 5 seconds
    int adc = analogRead(MQ_PIN);
    float voltage = adc * (VCC / 1023.0);
    if (voltage < 0.01) continue;

    float Rs = calculateRs(voltage);
    sumR0 += Rs / TARGET_RATIO;
    samples++;

    delay(20); // 50 Hz sampling
  }

  R0 = sumR0 / samples;
  calibrated = true;
  Serial.print("Calibration done. R0 = ");
  Serial.println(R0, 2);
}

//============================================================================
// Loop
//============================================================================
void loop() {
  int adc = analogRead(MQ_PIN);
  float voltage = adc * (VCC / 1023.0);
  int d0 = digitalRead(MQ_DIGITAL); // read digital output

  if (voltage < 0.01) return;

  float Rs = calculateRs(voltage);
  float ppm = calculatePPM(voltage);

  //================ MOVING AVERAGES =================
  float ppmMAshort = updateMA(ppmHistoryShort, MA_SHORT, idxShort, ppm);
  float ppmMAlong  = updateMA(ppmHistoryLong, MA_LONG, idxLong, ppm);

  //================ ALARM DETECTION =================
  if (ppmMAshort > PPM_THRESHOLD) {
    consecutiveHigh++;
    if (consecutiveHigh >= CONSECUTIVE_HIGH) {
      Serial.println("!!! DANGEROUS PPM SPIKE DETECTED !!!");
      // Here you can trigger your alarm / response
    }
  } else {
    consecutiveHigh = 0;
  }

  //================ RUNNING R0 CHECK =================
  float runningR0 = calculateRunningR0(Rs, 500.0); // assume baseline 500ppm for check
  if (fabs(runningR0 - R0) / R0 > 0.20) { // >20% deviation
    Serial.println("Recalibration triggered due to R0 drift >20%");
    unsigned long start = millis();
    int samples = 0;
    float sumR0 = 0.0;
    while (millis() - start < 5000) {
      int adc_temp = analogRead(MQ_PIN);
      float voltage_temp = adc_temp * (VCC / 1023.0);
      if (voltage_temp < 0.01) continue;
      float Rs_temp = calculateRs(voltage_temp);
      sumR0 += Rs_temp / TARGET_RATIO;
      samples++;
      delay(20);
    }
    R0 = sumR0 / samples;
    Serial.print("New R0 = "); Serial.println(R0, 2);
  }

  //================ LCD OUTPUT =================
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("RS:");
  lcd.print(Rs, 0);
  lcd.print(" ADC:");
  lcd.print(adc);

  lcd.setCursor(0, 1);
  lcd.print("RO:");
  lcd.print(R0, 0);
  lcd.print(" PPM:");
  lcd.print(ppmMAshort, 0);

  //================ SERIAL OUTPUT =================
  Serial.print("ADC: "); Serial.print(adc);
  Serial.print(" | D0: "); Serial.print(d0);
  Serial.print(" | V: "); Serial.print(voltage, 3);
  Serial.print(" | Rs: "); Serial.print(Rs, 2);
  Serial.print(" kΩ | R0: "); Serial.print(R0, 2);
  Serial.print(" kΩ | PPM: "); Serial.print(ppm, 1);
  Serial.print(" | PPM(shortMA): "); Serial.print(ppmMAshort, 1);
  Serial.print(" | PPM(longMA): "); Serial.print(ppmMAlong, 1);
  Serial.print(" | runningR0: "); Serial.print(runningR0, 2);
  Serial.println();

  delay(1000); // 1 Hz display update
}
