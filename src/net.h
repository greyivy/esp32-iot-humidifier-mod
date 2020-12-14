#pragma once

#include <MQTT.h>

extern MQTTClient mqttClient;

extern String topic_power;
extern String topic_empty;
extern String topic_tank;
extern String topic_humidity;
extern String topic_temperature;
extern String topic_total;
extern String topic_average;
extern String topic_percent;
extern String topic_remain;

void setupNet();
void loopNet();

bool isConnected();