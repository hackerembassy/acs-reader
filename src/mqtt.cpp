#include "mqtt.h"
#include "utils/debug.h"
#include "utils/led.h"
#include "utils/beeper.h"
#include "WiFi.h"

#include <AsyncMqttClient.h>

TimerHandle_t mqttReconnectTimer;

AsyncMqttClient mqttClient;

void PublishToMQTT(const char* type, const char* data) {
    char *topic;
    asprintf(&topic, "%s/%s", MQTT_TOPIC, type);
    mqttClient.publish(topic, 2, false, data);
}

void StopMQTT() {
    xTimerStop(mqttReconnectTimer, 0);
}

void StartMQTT() {
    xTimerStart(mqttReconnectTimer, 0);
}

std::vector<uint32_t> kAuthBeeps{14, 1500, 50, 0, 10, 1600, 50, 0, 10, 1700, 50, 0, 10, 1800, 50};
std::vector<uint32_t> kWrongBeeps{22, 1500, 100, 0, 10, 1400, 100, 0, 10, 1300, 100, 0, 10, 1200, 100, 0, 10, 1100, 100, 0, 10, 1000, 100};

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  DEBUG_PRINT("Received MQTT msg: ");
  DEBUG_PRINT(topic);
  //DEBUG_PRINT(" | ");
  //DEBUG_PRINT(payload,);
  DEBUG_PRINT("\n");

  char *topic_suc;
  char *topic_fail;
  asprintf(&topic_suc, "%s/%s", MQTT_TOPIC, "success");
  asprintf(&topic_fail, "%s/%s", MQTT_TOPIC, "failed");

  if(strcmp(topic, topic_suc) == 0) {
    SuccessLED();
    Beep(kAuthBeeps);
  } else if(strcmp(topic, topic_fail) == 0) {
    ErrorLED();
    Beep(kWrongBeeps);
  }
}

void onMqttConnect(bool sessionPresent) {
    DEBUG_PRINT("Connected to MQTT.\n");
    char *topic_suc;
    char *topic_fail;
    asprintf(&topic_suc, "%s/%s", MQTT_TOPIC, "success");
    asprintf(&topic_fail, "%s/%s", MQTT_TOPIC, "failed");
    mqttClient.subscribe(topic_suc, 2);
    mqttClient.subscribe(topic_fail, 2);
    StopLEDRing();
    SuccessLED();
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  DEBUG_PRINT("Disconnected from MQTT.\n");
  BlueLEDRing();
  if (WiFi.isConnected()) {
    StartMQTT();
  }
}

void connectToMQTT() {
  BlueLEDRing();
  DEBUG_PRINT("Connecting to MQTT...\n");
  mqttClient.connect();
}



void InitMQTT()
{

    mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMQTT));
    StopMQTT();
    mqttClient.onConnect(onMqttConnect);
    mqttClient.onDisconnect(onMqttDisconnect);
    mqttClient.onMessage(onMqttMessage);
    mqttClient.setServer(MQTT_HOST, 1883);
    mqttClient.setCredentials(MQTT_USERNAME, SECRET_MQTT_PASSWORD);
}
