#include "esp_wifi.h"
#include "WiFi.h"
#include "arduino_ota.h"
#include "utils/debug.h"
#include "utils/led.h"
#include "config.h"
#include "mqtt.h"
#include "esp_log.h"
#include "utils/beeper.h"

static const char *TAG = "wifiman";

TimerHandle_t wifi_reconnect_timer;

std::vector<uint32_t> kWiFiDisconnectedBeeps{400, 100,   0, 20,  500, 100,   0, 20,   300, 100};
std::vector<uint32_t> kWiFiConnectedBeeps{400, 100,  0, 20,  500, 100,  0, 20,  600, 100};
std::vector<uint32_t> kWiFiConnectingBeeps{200, 25,   0, 100,   200, 25};

void ConnectToWiFi() {
  YellowLEDRing();
  Beep(kWiFiConnectingBeeps);
  ESP_LOGI(TAG, "Connecting to WiFi...");
  WiFi.setHostname(OTA_HOSTNAME);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void WiFiEvent(WiFiEvent_t event) {
  ESP_LOGI(TAG, "Got WiFi event: %d", event);
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      ESP_LOGI(TAG, "WiFi connected");
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      xTimerStop(wifi_reconnect_timer, 0);
      ESP_LOGI(TAG, "IP address: %s\n", WiFi.localIP().toString().c_str());
      StartArduinoOTA();
      ESP_LOGI(TAG, "OTA server has been started\n");
      Beep(kWiFiConnectedBeeps);
      StopLEDRing();
      StartMQTTreconnect();
      break;
    case ARDUINO_EVENT_WIFI_STA_LOST_IP:
      ESP_LOGI(TAG, "Lost IP");
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      FaultLEDRing();
      Beep(kWiFiDisconnectedBeeps);
      ESP_LOGI(TAG, "WiFi lost connection\n");
      StopArduinoOTA();
      StopMQTTreconnect();
      xTimerStart(wifi_reconnect_timer, 0);
      ConnectToWiFi();
      break;
  }
}

void InitWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(OTA_HOSTNAME);
  wifi_reconnect_timer = xTimerCreate("wifi_timer", pdMS_TO_TICKS(5000),
                                      pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(ConnectToWiFi));
  WiFi.onEvent(WiFiEvent);
}

void StartWiFi() {
  ConnectToWiFi();
}