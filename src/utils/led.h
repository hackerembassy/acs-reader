#ifndef UTILS_LED_H_
#define UTILS_LED_H_

#include <Arduino.h>
#include <cstdint>

void InitLED();
void StartLED();
void StartLEDRing();
void BlueLEDRing();
void YellowLEDRing();
void StopLEDRing();
void ErrorLED();
void ErrorPermanentLED();
void SuccessLED();

#endif