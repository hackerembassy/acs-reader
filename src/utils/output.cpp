#include "output.h"
#include "config.h"
#include "esp_log.h"

static const char *TAG = "OUT";

void InitOutput() {
  //OUTPUT_SERIAL.begin(OUTPUT_SERIAL_SPEED);
}

void OutputReadID(const char* type, const char* data) {
  ESP_LOGI(TAG, "%s: %s", type, data);
  // OUTPUT_SERIAL.flush(true);
}