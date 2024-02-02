#ifndef MCU_CONFIG_H_
#define MCU_CONFIG_H_

#include "secrets.h"

// Debug
#define DEBUG_ENABLED

// PN532 pins
#define PN532_IRQ 22
#define PN532_RST 33  // Must be connected to RSTPDN pin, not RSTO!
#define PN532_SDA 23  // 3
#define PN532_SCL 19  // 2

// Debug serial
#define DEBUG_SERIAL Serial

// Output serial
#define OUTPUT_SERIAL Serial
#define OUTPUT_SERIAL_SPEED 115200

// LED
#define LED_DATA_PIN  25
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS    7
#define BRIGHTNESS  255

// Beeper
#define BEEPER 21

// WiFi
#define WIFI_SSID "MOWMeOW"
#define WIFI_PASSWORD SECRET_WIFI_PASSWORD

// WiFi
#define OTA_HOSTNAME "acs-reader"
#define OTA_PASSWORD SECRET_OTA_PASSWORD
#define OTA_PORT 3232

// MQTT
#define MQTT_HOST "minimi.lan"
#define MQTT_USERNAME "acs-reader"
#define MQTT_TOPIC "acs"

#endif