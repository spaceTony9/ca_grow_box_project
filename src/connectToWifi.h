#ifndef CONNECT_TO_WIFI_H
#define CONNECT_TO_WIFI_H

#include <WiFi.h>
#include <PubSubClient.h>

// WiFi credentials
// #define WIFI_NETWORK "iPhone von Anton"
// #define WIFI_PASSWORD "00000000"
#define WIFI_TIMEOUT_MS 20000

// MQTT settings
#define MQTT_BROKER "broker.hivemq.com"
#define MQTT_PORT 1883
#define MQTT_TOPIC_CONTROL "esp32/control"
#define MQTT_TOPIC_STATUS "esp32/status"

// LED pin
#define LED_PIN 2

// Function declarations
void connectToWifi();
void setupMQTT();
void reconnectMQTT();
void mqttCallback(char* topic, byte* payload, unsigned int length);
String getWifiNetwork();
String getWifiPassword();

// External objects
extern WiFiClient espClient;
extern PubSubClient mqttClient;
extern bool pumpServiceEnabled;

#endif