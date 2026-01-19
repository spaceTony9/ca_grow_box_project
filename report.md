# Plant Monitor prject

## Project Summary

This project presents automated plant monitor that is able to irrigate the indoor plants. The system has soil moisture sensor that serves as information source for the added water pump. Which in combination provides a possibility to provide plant care when the soil gets dry. There are remote capabilities presented. The communication between controller and discord bot enables user to effectively turn on/off pump service, see the status message and exersise manual watering service. 

Key features include continuous monitoring of temperature, humidity, and soil moisture levels, with data visualization on a local OLED display

## 1. Project Overview

### 1.1 Purpose and Objectives

The primary objective of this project is to develop an automated plant care system that:

- Monitors environmental conditions (temperature, humidity, soil moisture)
- Provides visual feedback on local OLED display
- Enables remote monitoring and control
- Automates irrigation based on soil moisture measurements

### 1.2 Project Target

- Indoor gardening and hydroponics
- Small-scale agricultural monitoring
- Automated plant care for remote or busy users

## 2. System Architecture

### 2.1 Hardware Components

| Component | Model/Type | Purpose |
|-----------|------------|---------|
| Microcontroller | ESP32 | Main logical unit |
| Temperature/Humidity Sensor | SHT31 | Provides temperature and air humidity measurements |
| Soil Moisture Sensor |Analog| Measures soil water content |
| Display | SSD1306 OLED (128x64) | Outputs collected data loccally |
| Water Pump | 2.5-6v Micro water pump | Pump water based on sensor readings |
| Power Supply x2 | 5V/3.3V regulated | Powers periphery devices and controller |

### 2.2 Software Architecture

#### Core Modules

- **Main Application (main.cpp):** Entry point into application. Coordinates all system operations, sensor readings, and control logic
- **OLED Driver (oled_ssd1306.c/h):** Custom driver that initializes i2c bus and prepares ssd1306 chip for information display
- **WiFi/MQTT Module (connectToWifi.cpp/h):** Responsible for wifi and communication service
- **Sensor Libraries:** Adafruit SHT31 for temperature/humidity sensing

## 3. Detailed System Description

### 3.1 Sensors

#### Temperature and Humidity 
SHT31 Temperature/ Humidity Sensor is connected to the controller with following schema: 
VIN -  3.3V ESP32, GND - GND ESP32, SDA - GPIO21, SLC - GPIO19. 

#### Soil Moisture Sensor
Capacitive soil moisture sensor is connected to the controller with following schema: VCC - 5V rail, GND - GND rail, AO - ADC 34 EPS32. The limit values calibrated as: Air = 4095, Water = 2200. Translated into 3 positions: DRY < 20% <= MOIST <= 60 % < WET <= 100%

### 3.2 OLED Screen

OLED screen is integrated into the system via ssd1306 driver (oled_ssd1306.c/.h). Driver initializes I2C bus for controller - sceen communication. Uses SSD1306 chip documentation to prepare screen for usage and includes next features:

- Pixel-level control reclangle drawing
- ASCII 5x7 font
- Methods to draw characted or geometric figure (rectangle and line)

### 3.3 Watering System

ESP32 regulates when to use water pump. If soil moisture drops to less than 20%, controller via GPIO 5 sends command to relay to turn on the water pump. Duration is 3 seconds, with 10 second cooldown period between activations. Water service can be enabled/disabled via discord bot.

#### Safety

- Pump cooldown, prevents overwatering
- Logging of every interaction with controller

### 3.4 Remote Control Features

The system responds to the following commands:

| Command | Function |
|---------|----------|
| !water | Manually activate pump (respects cooldown) |
| !pump_on | Enable automatic watering mode | 
| !pump_off | Disable automatic watering |
| !status | Request current sensor readings | 
| !plant | Get availiable to the user commands |

To use commands, user should be logged into their discord account and be able to write commands to discrod bot (Plant Monitor)

## 4. Project Result
Plant monitor system is able to read and interpret the data from sensors, communicate and transfer the data to remote server. Water service is able to irritate plant based on soil moisture levels. OLED screen is displaying sensor information locally.

## 6. Project scailability

### 6.1 Future features
In order for Plant Monitor to provide maximal automation for indoor plants care several future features are considered:

- Light intensity monitoring
- Email/SMS alerts for critical conditions
- Multiple sensor nodes for larger grow spaces
- Ergonomic box for the electronical parts

## 7. Conclusion

This project successfully integrated several periphery devices united with controller for logical operations. The purpose and results of the project match practical use situations. The system provides a robust solution for automated plant care with the flexibility of remote monitoring and control.

Modular software design allows for furter scallability and efficient maintenance and customisation. Developed "Low-language" driver allows high customisation and efficient power management. 

## Appendices

### Appendix A: Code Structure

#### Project Organization

- **main.cpp:** Main application logic, sensor reading, and control
- **oled_ssd1306.c/h:** Display driver with graphics library
- **connectToWifi.cpp/h:** WiFi and MQTT connectivity module
- **secrets.h:** WiFi credentials (not included in repository)

#### Key Functions

| Function | Module | Purpose |
|----------|--------|---------|
| setup() | main.cpp | Initialize all hardware and connections |
| loop() | main.cpp | Main execution loop |
| readSHT31Temperature() | main.cpp | Read temperature from SHT31 |
| readSHT31Humidity() | main.cpp | Read humidity from SHT31 |
| getSoilPercent() | main.cpp | Get calibrated soil moisture |
| runPump() | main.cpp | Automatic pump control logic |
| manualPump() | main.cpp | Manual pump activation |
| updateDisplay() | main.cpp | Update OLED with current data |
| sendMQTTStatus() | main.cpp | Publish status via MQTT |
| mqttCallback() | connectToWifi.cpp | Handle incoming MQTT messages |
| oled_init() | oled_ssd1306.c | Initialize OLED display |
| oled_print() | oled_ssd1306.c | Display text on OLED |


**Project Repository:** https://github.com/spaceTony9/ca_grow_box_project

**Project Developer** Anton Bilokon

**Date:** January 14, 2026