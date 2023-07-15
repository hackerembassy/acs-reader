#include "led.h"

#include "../mcu_config.h"
#include "debug.h"
#include <Arduino.h>
#include <FastLED.h>
#include "freertos/FreeRTOS.h"

CRGB leds[NUM_LEDS];

uint8_t ringPosition = 0;
uint8_t ringRun = 0;
bool ledRun = false;

TaskHandle_t LedTask;

void ledTask(void * pvParameters) {
  DEBUG_PRINT("LED started on core %d\n", xPortGetCoreID());

  for (;;) {
    if(ledRun) {
        if(ringRun == 1) {
            fadeToBlackBy(leds, NUM_LEDS, 128);    //Dims the LEDs by 128/256 (1/2) and thus sets the trail's length.
            leds[ringPosition] = CRGB::White;    //setHue: variable to set the LEDs colour
            ringPosition++;    //Shifts all LEDs one step in the currently active direction    
            if (ringPosition == NUM_LEDS) ringPosition = 1;    //If one end is reached, reset the position to loop around
            vTaskDelay(pdMS_TO_TICKS(40));
        } else {
            ringPosition++;
            if(ringPosition > 40) ledRun = false;
            if (ringRun == 0) fadeToBlackBy(leds, NUM_LEDS, 16); 
            else fadeToBlackBy(leds, NUM_LEDS, 32);
            vTaskDelay(pdMS_TO_TICKS(20));
        }
        FastLED.show();    //Finally, send the data to the LEDs
    } else {
        vTaskDelay(pdMS_TO_TICKS(50));
    }
  }
}

void InitLED() {
    FastLED.addLeds<LED_TYPE,LED_DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS)
      .setCorrection(TypicalLEDStrip)
      .setDither(BRIGHTNESS < 255);

    // set master brightness control
    FastLED.setBrightness(BRIGHTNESS);
    ledRun = false;
    FastLED.show();
}

void StartLED() {
        xTaskCreatePinnedToCore(
                   ledTask,   /* Task function. */
                   "LEDTask",     /* name of task. */
                   10000,       /* Stack size of task */
                   NULL,        /* parameter of the task */
                   5,           /* priority of the task */
                   &LedTask,      /* Task handle to keep track of created task */
                   1);          /* pin task to core 1 */
}

void StartLEDRing() {
    // for (uint8_t i = 0; i < NUM_LEDS; i++) {
    //     leds[i] = CRGB::Blue;
    // }
    // FastLED.show();
    ringPosition = 1;
    ringRun = 1;
    ledRun = true;
}

void StopLEDRing() {
    // for (uint8_t i = 0; i < NUM_LEDS; i++) {
    //     leds[i] = CRGB(0, 0, 0);
    // }
    // FastLED.show();
    ringPosition = 0;
    ringRun = 0;
    ledRun = true;
}

void ErrorLED() {
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB::Red;
    }
    FastLED.show();
    ringPosition = 0;
    ringRun = 3;
    ledRun = true;
}

void ErrorPermanentLED() {
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB::Red;
    }
    FastLED.show();
}

void SuccessLED() {
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB::Green;
    }
    FastLED.show();
    ringPosition = 0;
    ringRun = 2;
    ledRun = true;
}