#ifndef UTILS_LED_H_
#define UTILS_LED_H_

#include <Arduino.h>
#include <cstdint>

void InitBeeper();
void Beep(const uint32_t* time, uint8_t count);
void StartBeep();
void StopBeep();

#endif