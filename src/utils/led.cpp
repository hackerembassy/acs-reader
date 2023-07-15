#include "led.h"

#include "../mcu_config.h"
#include <Arduino.h>
#include <FastLED.h>
#include "freertos/FreeRTOS.h"

CRGB leds[NUM_LEDS];

uint8_t ringPosition = 0;
uint8_t ringRun = 0;


TimerHandle_t ringTimer;

void LEDRing(TimerHandle_t xTimer) {
  
  if(ringRun == 1) {
    fadeToBlackBy(leds, NUM_LEDS, 128);    //Dims the LEDs by 128/256 (1/2) and thus sets the trail's length.
    leds[ringPosition] = CRGB::White;    //setHue: variable to set the LEDs colour
    ringPosition++;    //Shifts all LEDs one step in the currently active direction    
    if (ringPosition == NUM_LEDS) ringPosition = 0;    //If one end is reached, reset the position to loop around
  } else {
    ringPosition++;
    if(ringPosition > 35) xTimerStop(ringTimer, pdMS_TO_TICKS(500));
    if (ringRun == 0) fadeToBlackBy(leds, NUM_LEDS, 80); 
    else fadeToBlackBy(leds, NUM_LEDS, 32);
  }
  FastLED.show();    //Finally, send the data to the LEDs
}

void InitLED() {
    FastLED.addLeds<LED_TYPE,LED_DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS)
      .setCorrection(TypicalLEDStrip)
      .setDither(BRIGHTNESS < 255);

    // set master brightness control
    FastLED.setBrightness(BRIGHTNESS);
    ringTimer = xTimerCreate("LEDRing", pdMS_TO_TICKS(40), pdTRUE, &ringTimer, &LEDRing);
    FastLED.show();
}

void StartLEDRing() {
    // for (uint8_t i = 0; i < NUM_LEDS; i++) {
    //     leds[i] = CRGB::Blue;
    // }
    // FastLED.show();
    ringPosition = 0;
    ringRun = 1;
    if(!xTimerIsTimerActive(ringTimer)) xTimerStart(ringTimer, pdMS_TO_TICKS(50));
}

void StopLEDRing() {
    // for (uint8_t i = 0; i < NUM_LEDS; i++) {
    //     leds[i] = CRGB(0, 0, 0);
    // }
    // FastLED.show();
    ringPosition = 0;
    ringRun = 0;
    if(!xTimerIsTimerActive(ringTimer)) xTimerStart(ringTimer, pdMS_TO_TICKS(50));
}

void ErrorLED() {
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB::Red;
    }
    FastLED.show();
    ringPosition = 0;
    ringRun = 3;
    if(!xTimerIsTimerActive(ringTimer)) xTimerStart(ringTimer, pdMS_TO_TICKS(50));
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
    if(!xTimerIsTimerActive(ringTimer)) xTimerStart(ringTimer, pdMS_TO_TICKS(50));
}