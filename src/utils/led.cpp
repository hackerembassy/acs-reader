#include "led.h"

#include "debug.h"
#include <Arduino.h>
#include <FastLED.h>
#include "freertos/FreeRTOS.h"
#include "config.h"

#define STOP_RING 0
#define WHITE_RING 1
#define SUCCESS_RING 2
#define ERROR_RING 3
#define BLUE_RING 4
#define YELLOW_RING 5
#define DIAL_RING 6
#define FAULT_RING 7


CRGB leds[NUM_LEDS];                                                                                                    

uint8_t ringPosition = 0;
uint8_t ringRun = 0;
bool ledRun = false;

unsigned long yellowRingStart;

TaskHandle_t LedTask;

void ledTask(void * pvParameters) {
  DEBUG_PRINT("LED started on core %d\n", xPortGetCoreID());

  for (;;) {
    if(ledRun) {
        if(ringRun == WHITE_RING || ringRun == BLUE_RING || ringRun == YELLOW_RING || ringRun == FAULT_RING) {
            fadeToBlackBy(leds, NUM_LEDS, 128);    //Dims the LEDs by 128/256 (1/2) and thus sets the trail's length.
            if(ringRun == WHITE_RING) leds[ringPosition] = CRGB::White;    //setHue: variable to set the LEDs colour
            else if(ringRun == BLUE_RING) leds[ringPosition] = CRGB::Blue;
            else if(ringRun == YELLOW_RING) leds[ringPosition] = CRGB::Yellow;
            else if(ringRun == FAULT_RING) leds[ringPosition] = CRGB::Red;
            ringPosition++;    //Shifts all LEDs one step in the currently active direction    
            
            if (ringPosition == NUM_LEDS) ringPosition = FIRST_LED;    //If one end is reached, reset the position to loop around
            if(ringRun == YELLOW_RING && (millis() - yellowRingStart) > 10000) ringRun = 0;
            
            if(ringRun == FAULT_RING) vTaskDelay(pdMS_TO_TICKS(120));
            else vTaskDelay(pdMS_TO_TICKS(40));
        } else if(ringRun != DIAL_RING) {
            ringPosition++;
            if(ringPosition > 100) ledRun = false;
            if (ringRun == STOP_RING) fadeToBlackBy(leds, NUM_LEDS, 16); 
            else fadeToBlackBy(leds, NUM_LEDS, 32);
            vTaskDelay(pdMS_TO_TICKS(20));
        }
        FastLED.show();    //Finally, send the data to the LEDs
    } else {
        vTaskDelay(pdMS_TO_TICKS(100));
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
    yellowRingStart = 0;
}

void StartLED() {
        xTaskCreatePinnedToCore(
                   ledTask,   /* Task function. */
                   "LEDTask",     /* name of task. */
                   10000,       /* Stack size of task */
                   NULL,        /* parameter of the task */
                   5,           /* priority of the task */
                   &LedTask,      /* Task handle to keep track of created task */
                   0);          /* pin task to core 0 */
}

void StartLEDRing() {
    if(ringRun != WHITE_RING && ringRun != BLUE_RING && ringRun != YELLOW_RING && ringRun != FAULT_RING) {
        ringPosition = 0;
    }
    ringRun = WHITE_RING;
    ledRun = true;
}

void FaultLEDRing() {
    if(ringRun != WHITE_RING && ringRun != BLUE_RING && ringRun != YELLOW_RING && ringRun != FAULT_RING) {
        ringPosition = 0;
    }
    ringRun = FAULT_RING;
    ledRun = true;
}

void BlueLEDRing() {
    if(ringRun != WHITE_RING && ringRun != BLUE_RING && ringRun != YELLOW_RING && ringRun != FAULT_RING) {
        ringPosition = 0;
    }
    ringRun = BLUE_RING;
    ledRun = true;
}

void YellowLEDRing() {
    if(ringRun != WHITE_RING && ringRun != BLUE_RING && ringRun != YELLOW_RING && ringRun != FAULT_RING) {
        ringPosition = 0;
    }
    yellowRingStart = millis();
    ringRun = YELLOW_RING;
    ledRun = true;
}

void DialLEDRing(uint8_t pinidx) {
    ringPosition = 0;
    ledRun = true;
    ringRun = DIAL_RING;

    for(uint8_t i; i < NUM_LEDS; i++)
    {
        leds[i] = CRGB::Black;
    }

    for(uint8_t i = pinidx + FIRST_LED; i < FIRST_LED + 6; i++)
    {
        leds[i] = CRGB::Red;
    }

    for(uint8_t i = FIRST_LED; i < (pinidx + FIRST_LED); i++)
    {
        leds[i] = CRGB::White;
    }
}

void FaultRing() {

}

void StopLEDRing() {
    // for (uint8_t i = 0; i < NUM_LEDS; i++) {
    //     leds[i] = CRGB(0, 0, 0);
    // }
    // FastLED.show();
    if(ringRun != STOP_RING) {
        ringPosition = 0;
        ringRun = STOP_RING;
        ledRun = true;
    }
}

void ErrorLED() {
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB::Red;
    }
    FastLED.show();
    ringPosition = 0;
    ringRun = ERROR_RING;
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
    ringRun = SUCCESS_RING;
    ledRun = true;
}