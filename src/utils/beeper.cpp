#include "beeper.h"

#include "../mcu_config.h"
#include <Arduino.h>

void InitBeeper() {
   ledcSetup(0, 8000, 12);
   
   pinMode(BEEPER_VCC, OUTPUT);
   digitalWrite(BEEPER_VCC, LOW);
}

void Beep(const uint32_t* time, uint8_t count) {
    ledcAttachPin(BEEPER_VCC, 0);
    for (uint8_t i = 0; i < count; i = i + 2) {
        if(time[i] > 0) ledcWriteTone(0, time[i]);
        else ledcWrite(0, 0);
        delay(time[i+1]);
    }
    ledcWrite(0, 0);
    ledcDetachPin(BEEPER_VCC);
    digitalWrite(BEEPER_VCC, LOW);
    pinMode(BEEPER_VCC, INPUT);
}

void StartBeep() {
    ledcAttachPin(BEEPER_VCC, 0);
    ledcWriteTone(0, 40);
}

void StopBeep() {
    ledcWrite(0, 0);
    ledcDetachPin(BEEPER_VCC);
    digitalWrite(BEEPER_VCC, LOW);
    pinMode(BEEPER_VCC, INPUT);
}