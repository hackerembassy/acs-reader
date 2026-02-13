#include "beeper.h"
#include "debug.h"
#include "freertos/FreeRTOS.h"
#include "esp32-hal.h"
#include <Arduino.h>
#include "config.h"
#include "esp_log.h"

static const char *TAG = "beep";


std::vector<uint32_t> beepTones;
bool beepActive = false;

TaskHandle_t beep_task;


void InitBeeper() {
  //ledcSetup(0, 8000, 12); // removed in Arduino-ESP32 v3
  ESP_LOGI(TAG, "Beeper init");
  pinMode(BEEPER, OUTPUT);
  digitalWrite(BEEPER, LOW);
}

void BeepTask(void *pvParameters) {

  (void) pvParameters;

  ESP_LOGI(TAG, "BeepTask started on core %d", xPortGetCoreID());
  pinMode(BEEPER, OUTPUT);

  for (;;) {
      ledcAttach(BEEPER, 8000, 10); // pin, freq, res
      for (uint32_t i = 0; i < beepTones.size(); i = i + 2) {
          if(beepTones.size() - i <= 1) return;
          ESP_LOGI(TAG, "beeping %d for %d", beepTones[i], beepTones[i+1]);
          if(beepTones[i] > 0) ledcWriteTone(BEEPER, beepTones[i]);
          else ledcWrite(BEEPER, 0);
          vTaskDelay(pdMS_TO_TICKS((beepTones[i+1])));
      }

      ledcWrite(BEEPER, 0);
      ledcDetach(BEEPER);
      digitalWrite(BEEPER, LOW);
      beepActive = false;
      ESP_LOGI(TAG, "BeepTask exit");
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
  ledcAttach(BEEPER, 8000, 10); // pin, freq, res
  ledcWriteTone(BEEPER, 425);
}

void StopBeep() {
  ledcWrite(BEEPER, 0);
  ledcDetach(BEEPER);
  digitalWrite(BEEPER, LOW);
  //pinMode(BEEPER, OUTPUT_OPEN_DRAIN);
}