#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SHT31.h>
#include <oled_ssd1306.h>
#include "connectToWifi.h"

Adafruit_SHT31 sht31 = Adafruit_SHT31();

extern bool pumpServiceEnabled;

// Pin definitions
const int LED = 23;
const int I2C_SDA = 19;
const int I2C_SCL = 21;
const int soilHumiditySensor = 34;
extern const int pumpPin = 5;

// Calibration values
const int AIR_VALUE = 4095;
const int WATER_VALUE = 2200;
const int DRY_THRESHOLD = 20;
const int WET_THRESHOLD = 60;

// Pump settings
const int PUMP_DURATION = 3000;
const unsigned long PUMP_COOLDOWN = 10000;
unsigned long lastPumpTime = 0;

// MQTT update interval
unsigned long lastMQTTUpdate = 0;
const unsigned long MQTT_UPDATE_INTERVAL = 10000;

// Function prototypes
float readSHT31Temperature();
float readSHT31Humidity();
int getSoilRaw();
int getSoilPercent();
void initPumpService();
void runPump();
void manualPump(); 
void updateDisplay(float temp, float humidity, int soilPercent);
void sendMQTTStatus(float temp, float humidity, int soilPercent);

void setup() {
    Serial.begin(115200);

    Wire.begin(I2C_SDA, I2C_SCL);
    delay(100);
    
    pinMode(soilHumiditySensor, INPUT);
    initPumpService();
  
    if (!sht31.begin(0x44)) {
        Serial.println("Check circuit. SHT31 not found!");
        while (1) delay(1000);
    }
    
    pinMode(LED, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED, LOW);
    digitalWrite(LED_PIN, LOW);
    
    oled_init(I2C_SDA, I2C_SCL);
    oled_clear();
    connectToWifi();
    setupMQTT();
}

void loop() {
    static unsigned long lastSensorRead = 0;
    const unsigned long SENSOR_INTERVAL = 5000;
    
    
    if (!mqttClient.connected()) {
        Serial.println("MQTT disconnected - reconnecting...");
        reconnectMQTT();
    }
    mqttClient.loop();
    
    unsigned long currentTime = millis();
    if (currentTime - lastSensorRead >= SENSOR_INTERVAL) {
        lastSensorRead = currentTime;
        
        float temp = readSHT31Temperature();
        float humidity = readSHT31Humidity();
        int soilPercent = getSoilPercent();
        int soilRaw = getSoilRaw();
        
        updateDisplay(temp, humidity, soilPercent);
        
        Serial.print("Temp: "); Serial.print(temp); Serial.print("C  ");
        Serial.print("Humidity: "); Serial.print(humidity); Serial.print("%  ");
        Serial.print("Soil Moisture: ");
        Serial.print(soilPercent);
        Serial.print(" % (Raw: ");
        Serial.print(soilRaw);
        Serial.println(")");
        
        if (soilPercent < 30) {
            Serial.println("   Status: DRY - Needs water");
        } else if (soilPercent < 60) {
            Serial.println("   Status: MOIST - Good");
        } else {
            Serial.println("   Status: WET");
        }
        
        if (currentTime - lastMQTTUpdate >= MQTT_UPDATE_INTERVAL) {
            sendMQTTStatus(temp, humidity, soilPercent);
            lastMQTTUpdate = currentTime;
        }
    }
    
    if (pumpServiceEnabled) {
        runPump();
    }
    
    delay(10);
}

void sendMQTTStatus(float temp, float humidity, int soilPercent) {
    String status = "{";
    status += "\"temperature\":" + String(temp, 1) + ",";
    status += "\"humidity\":" + String(humidity, 1) + ",";
    status += "\"soil_moisture\":" + String(soilPercent) + ",";
    status += "\"pump_enabled\":" + String(pumpServiceEnabled ? "true" : "false") + ",";
    
    String soilStatus;
    if (soilPercent < 30) {
        soilStatus = "DRY";
    } else if (soilPercent < 60) {
        soilStatus = "OK";
    } else {
        soilStatus = "WET";
    }
    status += "\"status\":\"" + soilStatus + "\"";
    status += "}";
    
    mqttClient.publish(MQTT_TOPIC_STATUS, status.c_str());
    Serial.println("MQTT Status sent: " + status);
}

float readSHT31Temperature() {
    float temp = sht31.readTemperature();
    if (isnan(temp)) {
        Serial.println("Failed to read temperature!");
        return 0;
    }
    return temp;
}

float readSHT31Humidity() {
    float humidity = sht31.readHumidity();
    if (isnan(humidity)) {
        Serial.println("Failed to read humidity!");
        return 0;
    }
    return humidity;
}

int getSoilRaw() {
    return analogRead(soilHumiditySensor);
}

int getSoilPercent() {
    int soilPercent = map(analogRead(soilHumiditySensor), AIR_VALUE, WATER_VALUE, 0, 100);
    return constrain(soilPercent, 0, 100);
}

void initPumpService() {
    pinMode(pumpPin, OUTPUT);
    digitalWrite(pumpPin, LOW);
}

void runPump() {
    // Check if pump service is enabled
    if (!pumpServiceEnabled) {
        return;
    }
    
    int soilPercent = getSoilPercent();
    unsigned long currentTime = millis();
    
    if (soilPercent <= DRY_THRESHOLD) {
        if (currentTime - lastPumpTime >= PUMP_COOLDOWN) {
            Serial.println("PUMP ON - Watering plant...");
            digitalWrite(pumpPin, HIGH);
            delay(PUMP_DURATION);
            digitalWrite(pumpPin, LOW);
            
            lastPumpTime = currentTime;
            
            Serial.print("Watering complete (");
            Serial.print(PUMP_DURATION / 1000);
            Serial.println(" seconds)");
            
            mqttClient.publish(MQTT_TOPIC_STATUS, "{\"event\":\"pump_activated\"}");
            
            delay(10000);
        } else {
            unsigned long timeLeft = (PUMP_COOLDOWN - (currentTime - lastPumpTime)) / 1000;
            Serial.print("Pump cooldown: ");
            Serial.print(timeLeft);
            Serial.println(" seconds remaining");
        }
    } else if (soilPercent >= WET_THRESHOLD) {
        digitalWrite(pumpPin, LOW);
    }
}

void updateDisplay(float temp, float humidity, int soilPercent) {
    oled_clear();
    
    oled_print(0, 0, "Temp:");
    oled_print_float(36, 0, temp, 1);
    oled_print(72, 0, "C");
    
    oled_print(0, 12, "Hum:");
    oled_print_float(30, 12, humidity, 1);
    oled_print(66, 12, "%");
    
    oled_print(0, 24, "Soil:");
    oled_print_number(36, 24, soilPercent);
    oled_print(60, 24, "%");
    
    oled_draw_rect(0, 36, 100, 8, 1);
    oled_fill_rect(2, 38, (soilPercent * 96) / 100, 4, 1);
    
    if (soilPercent < 30) {
        oled_print(0, 50, "Status: DRY");
    } else if (soilPercent < 60) {
        oled_print(0, 50, "Status: OK");
    } else {
        oled_print(0, 50, "Status: WET");
    }
    
    oled_display();
}

void manualPump() {
    unsigned long currentTime = millis();
    
    // Check cooldown period
    if (currentTime - lastPumpTime >= PUMP_COOLDOWN) {
        Serial.println("MANUAL PUMP ACTIVATED - Watering plant...");
        digitalWrite(pumpPin, HIGH);
        delay(PUMP_DURATION);
        digitalWrite(pumpPin, LOW);
        
        lastPumpTime = currentTime;
        
        Serial.print("Manual watering complete (");
        Serial.print(PUMP_DURATION / 1000);
        Serial.println(" seconds)");
        
        // Send notification via MQTT
        mqttClient.publish(MQTT_TOPIC_STATUS, "{\"event\":\"pump_activated\",\"type\":\"manual\"}");
    } else {
        unsigned long timeLeft = (PUMP_COOLDOWN - (currentTime - lastPumpTime)) / 1000;
        Serial.print("Pump cooldown active: ");
        Serial.print(timeLeft);
        Serial.println(" seconds remaining");
        
        // Send cooldown message
        String cooldownMsg = "{\"event\":\"pump_cooldown\",\"seconds\":" + String(timeLeft) + "}";
        mqttClient.publish(MQTT_TOPIC_STATUS, cooldownMsg.c_str());
    }
}