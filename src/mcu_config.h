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

// RDM6300 pins
#define RDM6300_SERIAL Serial1
#define RDM6300_RX 10

// Debug serial
#define DEBUG_SERIAL Serial

// Output serial
#define OUTPUT_SERIAL Serial1
#define OUTPUT_SERIAL_SPEED 115200

// Beeper
#define BEEPER_GND 27  // 16
#define BEEPER_VCC 26  // 35

// WiFi
#define WIFI_SSID "MOWMOW"
#define WIFI_PASSWORD SECRET_WIFI_PASSWORD

// WiFi
#define OTA_HOSTNAME "acs-reader"
#define OTA_PASSWORD SECRET_OTA_PASSWORD
#define OTA_PORT 3232

#endif