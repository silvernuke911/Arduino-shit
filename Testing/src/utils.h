#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>

void updatePPMReading();
float getAveragePPM();
float calculateRs(float sensor_volt);
float calculatePPM(float sensor_volt);
int getAirQualityLevel(float ppm);
String getQualityText(int level);
void debugSensorValues();
void logSensorData(float ppm, String qualityText);
void debugSensor();
void lcdDebug();
void MQ135SensorDirectData();

#endif
