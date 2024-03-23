#include <Arduino.h>

#include "arduino_ota.h"
#include "esp_wifi.h"
#include "nfc/handler.h"
#include "utils/beeper.h"
#include "utils/debug.h"
#include "utils/output.h"
#include "utils/led.h"
#include "mqtt.h"


TaskHandle_t nfc_task;

void MainNFCTask(void*) {
  HandleNFC();
}

void StartNFC() {
  xTaskCreatePinnedToCore(MainNFCTask, "nfc_task", 10000, nullptr, 10,
                          &nfc_task, 1);
}

std::vector<uint32_t> kStartupBeeps{200, 100, 200};

#include "nfc/interface/pn532.h"

#include <memory>

void setup() {
  InitDebug();

  InitLED();
  bool okNFC = InitNFC();
  InitWiFi();
  InitMQTT();
  InitArduinoOTA();
  InitBeeper();
  InitOutput();

  StartWiFi();

    // Start125KHz();
  if(okNFC) {
    DEBUG_PRINT("NFC OK\n");
    StartNFC();
    StartLED();
    BlueLEDRing();
  } else {
    StartBeep();
    ErrorPermanentLED();
  }
}

void loop() {}