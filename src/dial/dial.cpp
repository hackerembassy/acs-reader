#include "dial.h"
#include "../mqtt.h"
#include "../utils/beeper.h"
#include "../utils/debug.h"
#include "../utils/output.h"
#include "../utils/led.h"

#include "esp32-hal-gpio.h"
#include <Arduino.h>
#include <cstring>
#include "esp32-hal.h"
#include "freertos/FreeRTOS.h"
#include <iomanip>
#include <sstream>


#define MAX_PIN_WAIT_TIME 5000

SemaphoreHandle_t sem_num_int;
const long debouncing_time = 150;
volatile unsigned long last_millis = 0;

uint8_t pulse_cnt = 0;

uint8_t received_pin[16];
uint8_t received_idx = 0;

void deb_num_int()
{
    if ((long)(millis() - last_millis) >= debouncing_time) {
        xSemaphoreGiveFromISR(sem_num_int, NULL);
        last_millis = millis();
    }
}

void InitDial(int interrupt, int pulse)
{
    pinMode(interrupt, INPUT_PULLUP);
    pinMode(pulse, INPUT_PULLUP);
    sem_num_int = xSemaphoreCreateBinary();
    attachInterrupt(interrupt, deb_num_int, FALLING);
    xTaskCreate(dial_task, "dial_task", 512, NULL, 15, NULL);
}

const std::vector<uint32_t> kImpBeep{500, 20};
const std::vector<uint32_t> kDialBeep{925, 70};
const std::vector<uint32_t> kDialCompleteBeep{824, 50, 0, 50, 925, 50};

void pincode_handler_task(void *pvParameters)
{
    (void) pvParameters;

    unsigned long time_num_last_touch;

    DEBUG_PRINT("Dial started on core %d\n", xPortGetCoreID());

    for (;;) {

        if (!digitalRead(DIAL_INT)) {
            pulse_cnt = 0;
            time_num_last_touch = millis();

            while(!digitalRead(DIAL_INT)) {
                vTaskDelay(pdMS_TO_TICKS(20));
                if(digitalRead(DIAL_PULSE)) {
                    vTaskDelay(pdMS_TO_TICKS(20));
                    Beep(kImpBeep);
                    pulse_cnt++;
                    while(digitalRead(DIAL_PULSE)) {vTaskDelay(pdMS_TO_TICKS(5));}
                }
            }
        }

        if (pulse_cnt > 0) {
            if (pulse_cnt >= 10) {
                pulse_cnt = 0;
            }
            received_pin[received_idx] = pulse_cnt;
            received_idx++;
            DialLEDRing(received_idx);
            DEBUG_PRINT("DIAL recv\n");
            pulse_cnt = 0;
            //Beep(kDialBeep);
        }

        if (received_idx == 6) {
            char received_pin_buf[16];
            sprintf(received_pin_buf, "%d%d%d%d%d%d", received_pin[0], received_pin[1],received_pin[2],received_pin[3],received_pin[4],received_pin[5]);
            //DEBUG_PRINT("%s\n", received_pin_buf);
            YellowLEDRing();
            Beep(kDialCompleteBeep);
            PublishToMQTT("PIN", received_pin_buf);
            goto out;
        }

        if ((long)(millis() - time_num_last_touch) >= MAX_PIN_WAIT_TIME) {
            DEBUG_PRINT("MAX_PIN_WAIT_TIME limit\n");
            StopLEDRing();
            goto out;
        }
    }

out:
    received_idx = 0;
    attachInterrupt(DIAL_INT, deb_num_int, FALLING);
    vTaskDelete(NULL);
}

void dial_task(void *pvParameters)
{
    (void) pvParameters;

    for (;;) {

        if (xSemaphoreTake(sem_num_int, portMAX_DELAY) == pdTRUE) {
            detachInterrupt(DIAL_INT);
            DialLEDRing(0);
            xTaskCreate(pincode_handler_task, "pincode_handler_task", 2048, NULL, 5, NULL);
        }

    }
}