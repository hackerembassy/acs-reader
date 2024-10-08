#include "esp_wifi.h"

#include <WiFi.h>
#include "arduino_ota.h"
#include "utils/debug.h"
#include "utils/led.h"
#include "config.h"
#include "mqtt.h"

TimerHandle_t wifi_reconnect_timer;

void ConnectToWiFi(TimerHandle_t) {
  BlueLEDRing();
  DEBUG_PRINT("Connecting to WiFi...\n");
  WiFi.setHostname(OTA_HOSTNAME);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void WiFiEvent(WiFiEvent_t event) {
  DEBUG_PRINT("Got WiFi event: %d\n", event);
  switch (event) {
    case SYSTEM_EVENT_STA_GOT_IP:
      DEBUG_PRINT("WiFi connected\n");
      DEBUG_PRINT("IP address: %s\n", WiFi.localIP().toString().c_str());
      StartArduinoOTA();
      DEBUG_PRINT("OTA server has been started\n");
      xTimerStop(wifi_reconnect_timer, 0);
      StartMQTT();
      StopLEDRing();
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      DEBUG_PRINT("WiFi lost connection\n");
      StopArduinoOTA();
      StopMQTT();
      xTimerStart(wifi_reconnect_timer, 0);
      break;
  }
}

void InitWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(OTA_HOSTNAME);
  wifi_reconnect_timer = xTimerCreate("wifi_timer", pdMS_TO_TICKS(2000),
                                      pdFALSE, nullptr, ConnectToWiFi);
  WiFi.onEvent(WiFiEvent);
}

void StartWiFi() {
  ConnectToWiFi(nullptr);
}