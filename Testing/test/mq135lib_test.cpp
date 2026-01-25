#include <Arduino.h>
#include <MQ135.h>

// Create sensor object on analog pin A0
MQ135 mq135(A0);

void setup() {
    Serial.begin(9600);
    Serial.println("========================================");
    Serial.println("  PHOENIX1747 MQ135 LIBRARY TEST");
    Serial.println("========================================");
    
    // Important: MQ135 needs warm-up time
    Serial.println("Warming up sensor (20 seconds)...");
    for(int i = 20; i > 0; i--) {
        if(i % 5 == 0) {
            Serial.print(i);
            Serial.print(" ");
        }
        delay(1000);
    }
    Serial.println("\nGO!");
}

void loop() {
    // Get all available measurements
    float resistance = mq135.getResistance();
    float correctedResistance = mq135.getCorrectedResistance(20.0, 50.0);
    float ppm_raw = mq135.getPPM();
    float ppm_corrected = mq135.getCorrectedPPM(20.0, 50.0);
    float rzero = mq135.getRZero();
    float correctedRZero = mq135.getCorrectedRZero(20.0, 50.0);
    
    // Display results
    Serial.println("\n--- MQ135 Readings ---");
    Serial.print("Resistance: ");
    Serial.print(resistance, 2);
    Serial.println(" kOhm");
    
    Serial.print("Corrected Resistance: ");
    Serial.print(correctedResistance, 2);
    Serial.println(" kOhm");
    
    Serial.print("PPM (raw): ");
    Serial.print(ppm_raw, 1);
    Serial.println(" ppm");
    
    Serial.print("PPM (corrected): ");
    Serial.print(ppm_corrected, 1);
    Serial.println(" ppm");
    
    Serial.print("RZero: ");
    Serial.print(rzero, 2);
    Serial.println(" kOhm");
    
    Serial.print("Corrected RZero: ");
    Serial.print(correctedRZero, 2);
    Serial.println(" kOhm");
    
    Serial.println("----------------------");
    
    // Test with different temperature/humidity values
    Serial.println("Testing different conditions:");
    
    // Hot & humid
    float ppm_hot_humid = mq135.getCorrectedPPM(30.0, 80.0);
    Serial.print("30°C, 80% RH: ");
    Serial.print(ppm_hot_humid, 1);
    Serial.println(" ppm");
    
    // Cold & dry
    float ppm_cold_dry = mq135.getCorrectedPPM(10.0, 20.0);
    Serial.print("10°C, 20% RH: ");
    Serial.print(ppm_cold_dry, 1);
    Serial.println(" ppm");
    
    Serial.println("========================================");
    
    delay(10000); // Wait 10 seconds between readings
}