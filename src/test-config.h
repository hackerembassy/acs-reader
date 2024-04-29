#ifndef MCU_CONFIG_H_
#define MCU_CONFIG_H_

#include "secrets.h"

// Debug
#define DEBUG_ENABLED

// PN532 pins
#define PN532_IRQ 20
#define PN532_RST 21  // Must be connected to RSTPDN pin, not RSTO!
#define PN532_SDA 4
#define PN532_SCL 5

// Debug serial
#define DEBUG_SERIAL Serial

// Output serial
#define OUTPUT_SERIAL Serial
#define OUTPUT_SERIAL_SPEED 115200

// Dial pins
#define DIAL_INT           10
#define DIAL_PULSE         8

// LED
#define LED_DATA_PIN  9
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS    8
#define FIRST_LED 0
#define BRIGHTNESS  100

// Beeper
#define BEEPER 6

// WiFi
#define WIFI_SSID "MOWMeOW"
#define WIFI_PASSWORD SECRET_WIFI_PASSWORD

// WiFi
#define OTA_HOSTNAME "acs-reader-test"
#define OTA_PASSWORD SECRET_OTA_PASSWORD
#define OTA_PORT 3232

// MQTT
#define MQTT_HOST "minimi.lan"
#define MQTT_USERNAME "acs-reader"
#define MQTT_TOPIC "acs-test"

#endif