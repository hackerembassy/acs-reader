#ifndef MCU_CONFIG_H_
#define MCU_CONFIG_H_

#include "secrets.h"

// Debug
//#define DEBUG_ENABLED

// PN532 pins
#define PN532_IRQ 6
#define PN532_RST 8 // Must be connected to RSTPDN pin, not RSTO!
#define PN532_SDA 5
#define PN532_SCL 4

// RDM6300 pins
#define RDM6300_SERIAL Serial1
#define RDM6300_RX 11

// Debug serial
#define DEBUG_SERIAL Serial

// Output serial
#define OUTPUT_SERIAL Serial
#define OUTPUT_SERIAL_SPEED 115200

// Beeper
#define BEEPER_GND 3
#define BEEPER_VCC 10

// LED
#define LED_DATA_PIN  7
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS    7
#define BRIGHTNESS  255

// WiFi
#define WIFI_SSID "MOWMeOW"
#define WIFI_PASSWORD SECRET_WIFI_PASSWORD

// WiFi
#define OTA_HOSTNAME "acs-reader"
#define OTA_PASSWORD SECRET_OTA_PASSWORD
#define OTA_PORT 3232

#define MQTT_HOST "minimi.lan"
#define MQTT_USERNAME "acs-reader"
#define MQTT_TOPIC "acs"

#endif