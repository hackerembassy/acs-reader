#include <Arduino.h>
#include "freertos/FreeRTOS.h"

TimerHandle_t dial_read_timer;

uint8_t numDialed();

void InitDial() {
    pinMode(DIAL_A, OUTPUT);
    pinMode(DIAL_B, INPUT_PULLUP);
    pinMode(DIAL_C, INPUT_PULLUP);

    digitalWrite(DIAL_A, LOW);
}

void ReadDial() {

};

void StartDial() {

}

void StopDial() {

}

void PrintNum() {

}