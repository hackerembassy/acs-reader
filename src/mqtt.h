#include <Arduino.h>

#ifndef MQTT_H_
#define MQTT_H_

void InitMQTT();
void StopMQTT();
void StartMQTT();
void PublishToMQTT(const char* type, const char* data);

#endif