#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "sdkconfig.h"
#include <Arduino.h>
#include "arduino_ota.h"
#include "esp_wifi.h"
#include "nfc/handler.h"
#include "utils/beeper.h"
#include "utils/debug.h"
#include "utils/output.h"
#include "utils/led.h"
//#include "dial/dial.h"
#include "mqtt.h"
#include "config.h"
#include "nfc/interface/pn532.h"
#include <memory>

static const char *TAG = "main";

TaskHandle_t nfc_task;

void MainNFCTask(void*) {
  HandleNFC();
}

void StartNFC() {
  xTaskCreatePinnedToCore(MainNFCTask, "nfc_task", 10000, nullptr, 10,
                          &nfc_task, 0);
}

std::vector<uint32_t> kStartupBeeps{200, 100, 200};


void setup() {
  InitDebug();
  InitBeeper();
  InitLED();

  bool okNFC = InitNFC();
  InitWiFi();
  InitMQTT();
  InitArduinoOTA();
  InitOutput();
  //InitDial(DIAL_INT, DIAL_PULSE);

  StartWiFi();
  StartNFC();
  StartLED();

  if(okNFC) {
    ESP_LOGI(TAG, "NFC OK");
    BlueLEDRing();
  } else {
    ESP_LOGE(TAG, "NFC fail");
    ErrorPermanentLED();
  }
}

void loop() {}