#include "beeper.h"

#include <Arduino.h>

void InitBeeper() {
  ledcSetup(0, 8000, 12);
  pinMode(BEEPER, OUTPUT);
  digitalWrite(BEEPER, LOW);
}

void Beep(const std::vector<uint32_t>& time) {

  ledcAttachPin(BEEPER, 0);
  for (uint32_t i = 1; i < (time[0] + 1); i = i + 2) {
      if(time[i] > 0) ledcWriteTone(0, time[i]);
      else ledcWrite(0, 0);
      delay(time[i+1]);
  }

  ledcWrite(0, 0);
  ledcDetachPin(BEEPER);
  digitalWrite(BEEPER, LOW);
  //pinMode(BEEPER, OUTPUT_OPEN_DRAIN);

}

void StartBeep() {
  ledcAttachPin(BEEPER, 0);
  ledcWriteTone(0, 20);
}

void StopBeep() {
  ledcWrite(0, 0);
  ledcDetachPin(BEEPER);
  digitalWrite(BEEPER, LOW);
  //pinMode(BEEPER, OUTPUT_OPEN_DRAIN);
}