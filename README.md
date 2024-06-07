# acs-reader

Hacker Embassy ACS NFC-EMV reader based on ESP32-C3 and PN532, supporting ECP frames with some ✨magic✨ for Express Transit Cards on iPhone/Watch.

Allows to:
- Read PANs from EMVCo cards (including VISA, MasterCard, MIR)
- Send it to access control server and display authorization result.


Our goal is to develop support of complete EMV transaction flow to be able to verify card's credentials using PKI. Unfortunately this functionality requires extensive research and is not available yet.

This project would be impossible without code and support by B4CKSP4CE members.

To build the project you need to create `src/secrets.h` with defines for:
```c
#define SECRET_WIFI_PASSWORD "wifipassword" // wifi password
#define SECRET_OTA_PASSWORD "otapassword" // ota update password
```

Also probably you need to change the `WIFI_SSID` and `OTA_HOSTNAME` defines in `src/mcu_config.h` for your specific environment.
