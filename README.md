# acs-reader

Hacker Embassy ACS NFC/EMV/RFID reader based on ESP32-C3 and PN532.

forked from B4CKSP4CE

Allows to:
- Read UIDs from NFC tags
- Read PANs from EMVCo cards (including VISA, MasterCard, MIR)

To build the project you also require to create `src/secrets.h` with defines for:
```c
#define SECRET_WIFI_PASSWORD "wifipassword" // wifi password
#define SECRET_OTA_PASSWORD "otapassword" // ota update password
```

Also probably you need to change the `WIFI_SSID` and `OTA_HOSTNAME` defines in `src/mcu_config.h` for your specific environment.
