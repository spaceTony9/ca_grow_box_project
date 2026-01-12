#include "connectToWifi.h"
#include <secrets.h>

WiFiClient espClient;
PubSubClient mqttClient(espClient);
bool pumpServiceEnabled = true;

// Add this at the top of the file
extern const int pumpPin;

void connectToWifi() {
    Serial.print("Connecting to WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_NETWORK, WIFI_PASSWORD);
    
    unsigned long startAttemptTime = millis();
    
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < WIFI_TIMEOUT_MS) {
        delay(500);
        Serial.print(".");
    }
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println();
        Serial.println("Failed to connect to WiFi!");
        Serial.println("Timeout reached.");
    } else {
        Serial.println();
        Serial.println("WiFi connected!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    }
}

void setupMQTT() {
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);
}

void reconnectMQTT() {
    while (!mqttClient.connected()) {
        Serial.print("Attempting MQTT connection...");
        String clientId = "ESP32-PlantMonitor-" + String(random(0xffff), HEX);
        
        if (mqttClient.connect(clientId.c_str())) {
            Serial.println("connected!");
            mqttClient.subscribe(MQTT_TOPIC_CONTROL);
            Serial.println("Subscribed to control topic");
            
            // Send online notification
            mqttClient.publish(MQTT_TOPIC_STATUS, "{\"status\":\"online\"}");
        } else {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String message = "";
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("]: ");
    Serial.println(message);
    Serial.print("Message length: ");
    Serial.println(length);
    Serial.print("pumpServiceEnabled before: ");
    Serial.println(pumpServiceEnabled);
    
    // Declare external functions
    extern float readSHT31Temperature();
    extern float readSHT31Humidity();
    extern int getSoilPercent();
    extern void sendMQTTStatus(float temp, float humidity, int soilPercent);
    extern void manualPump();
    
    // Handle commands from Discord
    if (message == "PUMP_ON") {
        Serial.println("✓ PUMP_ON command matched!");
        manualPump();
    } else if (message == "PUMP_ENABLE") {
        Serial.println("✓ PUMP_ENABLE command matched!");
        pumpServiceEnabled = true;
        digitalWrite(pumpPin, LOW);
        mqttClient.publish(MQTT_TOPIC_STATUS, "{\"event\":\"pump_enabled\"}");
        Serial.println("Pump service ENABLED");
    } else if (message == "PUMP_DISABLE") {
        Serial.println("✓ PUMP_DISABLE command matched!");
        pumpServiceEnabled = false;
        digitalWrite(pumpPin, LOW);
        mqttClient.publish(MQTT_TOPIC_STATUS, "{\"event\":\"pump_disabled\"}");
        Serial.println("Pump service DISABLED - pump stopped");
    } else if (message == "STATUS") {
        Serial.println("✓ STATUS command matched!");
        float temp = readSHT31Temperature();
        float humidity = readSHT31Humidity();
        int soilPercent = getSoilPercent();
        sendMQTTStatus(temp, humidity, soilPercent);
    } else {
        Serial.println("✗ No command matched - ignoring message");
    }
    
    Serial.print("pumpServiceEnabled after: ");
    Serial.println(pumpServiceEnabled);
}