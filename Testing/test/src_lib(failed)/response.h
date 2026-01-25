#ifndef RESPONSE_H
#define RESPONSE_H

#include <Arduino.h>

void handleWarningState(float ppm, String qualityText);
void handleNormalState(float ppm, String qualityText);
void displayWarningMessage(float ppm);
void displayNormalMessage(float ppm, String qualityText);
void activateWarningSystem();
void warning_buzzer();
void deactivateWarningSystem();
void updateBuzzer();
void startBuzzer();
void stopBuzzer();

#endif
