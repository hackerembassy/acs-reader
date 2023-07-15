#include "beeper.h"

#include "../mcu_config.h"
#include <Arduino.h>

void InitBeeper() {
   //pinMode(BEEPER_GND, OUTPUT);
   //pinMode(BEEPER_VCC, OUTPUT);
   //digitalWrite(BEEPER_VCC, HIGH);
}

void Beep(const uint32_t* time, uint8_t count) {
    //pinMode(BEEPER_VCC, OUTPUT);
    //uint8_t state = HIGH;
    for (uint8_t i = 0; i < count; i = i + 2) {
        //if(time[i] > 0) tone(BEEPER_VCC, time[i]);
        //else noTone(BEEPER_VCC);
        delay(time[i+1]);
    }
    //noTone(BEEPER_VCC);
    //ledcDetachPin(BEEPER_VCC);
    //pinMode(BEEPER_VCC, INPUT);
    //digitalWrite(BEEPER_VCC, HIGH);
}

void StartBeep() {
    //pinMode(BEEPER_VCC, OUTPUT);
    //tone(BEEPER_VCC, 40);
}

void StopBeep() {
    //noTone(BEEPER_VCC);
    //ledcDetachPin(BEEPER_VCC);
    //pinMode(BEEPER_VCC, INPUT);
    //digitalWrite(BEEPER_VCC, HIGH);
}