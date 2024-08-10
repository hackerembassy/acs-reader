#include "beeper.h"
#include "debug.h"
#include "freertos/FreeRTOS.h"
#include "esp32-hal.h"
#include <Arduino.h>
#include "config.h"


std::vector<uint32_t> beepTones;
bool beepActive = false;

TaskHandle_t beep_task;


void InitBeeper() {
  ledcSetup(0, 8000, 12);
  pinMode(BEEPER, OUTPUT);
  digitalWrite(BEEPER, LOW);
}

void BeepTask(void *pvParameters) {

  (void) pvParameters;

  DEBUG_PRINT("Beep started on core %d\n", xPortGetCoreID());

  for (;;) {
      ledcAttachPin(BEEPER, 0);
      for (uint32_t i = 0; i < beepTones.size(); i = i + 2) {
          if(beepTones[i] > 0) ledcWriteTone(0, beepTones[i]);
          else ledcWrite(0, 0);
          vTaskDelay(pdMS_TO_TICKS((beepTones[i+1])));
      }

      ledcWrite(0, 0);
      ledcDetachPin(BEEPER);
      digitalWrite(BEEPER, LOW);
      beepActive = false;
      vTaskDelete(NULL);
  }
}

void Beep(const std::vector<uint32_t>& time) {
  if(!beepActive) {
    beepTones = time;
    beepActive = true;
    xTaskCreate(&BeepTask, "beep_task", 2000, NULL, 10, &beep_task);
  }
}

void StartBeep() {
  ledcAttachPin(BEEPER, 0);
  ledcWriteTone(0, 425);
}

void StopBeep() {
  ledcWrite(0, 0);
  ledcDetachPin(BEEPER);
  digitalWrite(BEEPER, LOW);
  //pinMode(BEEPER, OUTPUT_OPEN_DRAIN);
}