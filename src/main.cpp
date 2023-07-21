#include <Arduino.h>

#include "mcu_config.h"
#include "utils/debug.h"
#include "125khz/handler.h"
#include "nfc/handler.h"
#include "esp_wifi.h"
#include "mqtt.h"
#include "arduino_ota.h"
#include "utils/beeper.h"
#include "utils/led.h"
#include "utils/output.h"

//TaskHandle_t _125khz_task;
TaskHandle_t nfc_task;

void Main125KHzTask(void*) {
  Handle125KHz();
}

void MainNFCTask(void*) {
  HandleNFC();
}

void Start125KHz() {
  //xTaskCreatePinnedToCore(Main125KHzTask, 
  //    "125khz_task", 10000, nullptr,      
  //    1, &_125khz_task, 1);        
}

void StartNFC() {
  xTaskCreatePinnedToCore(MainNFCTask, 
      "nfc_task", 100000, nullptr,      
      5, &nfc_task, 0);        
}

const uint32_t kStartupBeeps[] = {4000, 100, 0, 100, 4000, 100};

void StartupBeepTask(void*) {
  
}

void setup() {
  InitDebug();
  //Init125KHz();
  InitWiFi();
  InitArduinoOTA();
  InitBeeper();
  InitOutput();
  InitLED();
  bool okNFC = InitNFC();
  StartWiFi();
  InitMQTT();


  // Start125KHz();
  if(okNFC) {
    Beep(kStartupBeeps, sizeof(kStartupBeeps) / sizeof(kStartupBeeps[0]));
    StartNFC();
    StartLED();
  } else {
    StartBeep();
    ErrorPermanentLED();
  }
}

void loop() {
}
