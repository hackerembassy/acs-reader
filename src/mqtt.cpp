#include "mqtt.h"
#include "utils/debug.h"
#include "utils/led.h"
#include "utils/beeper.h"
#include "WiFi.h"
#include "config.h"
#include "esp_log.h"

#include <AsyncMqttClient.h>

TimerHandle_t mqttReconnectTimer;

static const char *TAG = "mqtt";

AsyncMqttClient mqttClient;

std::vector<uint32_t> kAuthBeeps{925, 70, 0, 10, 1165, 70, 0, 10, 1386, 70, 0, 10, 1850, 70};
std::vector<uint32_t> kWrongBeeps{654, 100, 0, 10, 734, 100, 0, 10, 824, 100, 0, 10, 654, 100, 0, 10, 734, 100, 0, 10, 654, 100};

std::vector<uint32_t> kMQTTDisconnectedBeeps{600, 100,   0, 20,   500, 100,   0, 20,   400, 100};
std::vector<uint32_t> kMQTTConnectedBeeps{400, 100,   0, 20,   300, 100,   0, 20,    900, 100};
std::vector<uint32_t> kMQTTConnectingBeeps{600, 25,    0, 100,    600, 25};

char *topic_will;

void PublishToMQTT(const char* type, const char* data) {
    char *topic;
    asprintf(&topic, "%s/%s", MQTT_TOPIC, type);
    mqttClient.publish(topic, 2, false, data);
}

void ConnectToMQTT() {
  BlueLEDRing();
  ESP_LOGI(TAG, "Connecting to MQTT...");
  Beep(kMQTTConnectingBeeps);
  mqttClient.connect();
  if(!mqttClient.connected()) FaultLEDRing();
}

void StopMQTTreconnect() {
    xTimerStop(mqttReconnectTimer, 0);
}

void StartMQTTreconnect() {
    xTimerStart(mqttReconnectTimer, 0);
    ConnectToMQTT();
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  ESP_LOGI(TAG, "Received MQTT msg: %s", topic);

    char *topic_result;
    char *topic_diag;
    asprintf(&topic_result, "%s/%s", MQTT_TOPIC, "result");
    asprintf(&topic_diag, "%s/%s", MQTT_TOPIC, "diag");

  if(strcmp(topic, topic_result) == 0) {
    if(strncmp(payload, "success", len) == 0) {
      SuccessLED();
      Beep(kAuthBeeps);
    } else if(strncmp(payload, "fail", len) == 0) {
      ErrorLED();
      Beep(kWrongBeeps);
    }
  } else if(strcmp(topic, topic_diag) == 0) {
    if(strncmp(payload, "reboot", len) == 0) {
      ErrorPermanentLED();
      StartBeep();
      esp_restart();
    }
  }
}

void onMqttConnect(bool sessionPresent) {
    StopMQTTreconnect();
    ESP_LOGI(TAG, "Connected to MQTT.");
    mqttClient.publish(topic_will, 1, true, "online");
    Beep(kMQTTConnectedBeeps);
    char *topic_result;
    char *topic_diag;
    asprintf(&topic_result, "%s/%s", MQTT_TOPIC, "result");
    asprintf(&topic_diag, "%s/%s", MQTT_TOPIC, "diag");
    mqttClient.subscribe(topic_result, 2);
    mqttClient.subscribe(topic_diag, 2);
    mqttClient.publish(topic_will, 1, true, "online");
    StopLEDRing();
    SuccessLED();
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  ESP_LOGI(TAG, "Disconnected from MQTT.");
  if (WiFi.isConnected()) {
    Beep(kMQTTDisconnectedBeeps);
    FaultLEDRing();
    StartMQTTreconnect();
  }
}

void InitMQTT()
{
    asprintf(&topic_will, "%s/%s", MQTT_TOPIC, "lwt");

    mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(10000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(ConnectToMQTT));
    StopMQTTreconnect();
    mqttClient.onConnect(onMqttConnect);
    mqttClient.onDisconnect(onMqttDisconnect);
    mqttClient.onMessage(onMqttMessage);
    mqttClient.setServer(MQTT_HOST, 1883);
    mqttClient.setClientId(OTA_HOSTNAME);
    mqttClient.setCredentials(MQTT_USERNAME, SECRET_MQTT_PASSWORD);
    mqttClient.setWill(topic_will, 1, true, "offline");
    mqttClient.setKeepAlive(10);
}
