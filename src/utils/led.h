#ifndef UTILS_BEEPER_H_
#define UTILS_BEEPER_H_

#include <Arduino.h>
#include <cstdint>

void InitLED();
void StartLED();
void StartLEDRing();
void StopLEDRing();
void ErrorLED();
void ErrorPermanentLED();
void SuccessLED();

#endif