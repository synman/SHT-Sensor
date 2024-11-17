/***************************************************************************
Copyright © 2023 Shell M. Shrader <shell at shellware dot com>
----------------------------------------------------------------------------
This work is free. You can redistribute it and/or modify it under the
terms of the Do What The Fuck You Want To Public License, Version 2,
as published by Sam Hocevar. See the COPYING file for more details.
****************************************************************************/
#include "Bootstrap.h"

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoHA.h>
#include <WEMOS_SHT3X.h>
#include <LOLIN_HP303B.h>

Adafruit_SSD1306 display(0);
SHT3X sht30(0x45);
// LOLIN_HP303B HP303BSensor;

#ifdef BS_USE_TELNETSPY
    TelnetSpy SerialAndTelnet;
    Bootstrap bs = Bootstrap(PROJECT_NAME, &SerialAndTelnet, 1500000);
#else
    Bootstrap bs = Bootstrap(PROJECT_NAME);
#endif

#define MQTT_SERVER                    "mqtt_server"
#define MQTT_USER                      "mqtt_user"
#define MQTT_PWD                       "mqtt_pwd"
#define PUBLISH_INTERVAL               "publish_interval"
#define PUBLISH_INTERVAL_IN_SECONDS    "publish_interval_in_seconds"
#define DEFAULT_PUBLISH_INTERVAL       60000
#define MIN_PUBLISH_INTERVAL           1000

#define SHT30_TEMPERATURE              "sht30_temperature"
#define SHT30_HUMIDITY                 "sht30_humidity"

#define HP303B_TEMPERATURE              "hp303b_temperature"
#define HP303B_PRESSURE                 "hp303b_pressure"

#define MQTT_SERVER_LEN                16
#define MQTT_USER_LEN                  16
#define MQTT_PWD_LEN                   32

#define SENSOR_TEMPERATURE_VARIANCE    -0.0

typedef struct sht_config_type : config_type {
    tiny_int      mqtt_server_flag;
    char          mqtt_server[MQTT_SERVER_LEN];
    tiny_int      mqtt_user_flag;
    char          mqtt_user[MQTT_USER_LEN];
    tiny_int      mqtt_pwd_flag;
    char          mqtt_pwd[MQTT_PWD_LEN];
    tiny_int      publish_interval_flag;
    unsigned long publish_interval;
} SHT_CONFIG_TYPE;

SHT_CONFIG_TYPE sht_config;

float           hp303b_temperature = 0.0;
float           hp303b_pressure = 0.0;

float           sht30_temperature = 0.0;
tiny_int        sht30_humidity = 0;

const float HPA_TO_INHG                  = 0.02952998057228486;

HADevice device;
WiFiClient wifiClient;
HAMqtt mqtt(wifiClient, device, 10);

byte deviceId[40];
char deviceName[40];

char sht30_tempSensorName[80];
char sht30_humidSensorName[80];
char hp303b_tempSensorName[80];
char hp303b_pressSensorName[80];
char voltageSensorName[80];
char ipAddressSensorName[80];

HASensorNumber* sht30_tempSensor;
HASensorNumber* sht30_humidSensor;
HASensorNumber* hp303b_tempSensor;
HASensorNumber* hp303b_pressSensor;
HASensorNumber* voltageSensor;
HASensor* ipAddressSensor;

const bool isSampleValid(const float value);
const String escParam(const char *param_name);
void update_lcd(const tiny_int temp, const tiny_int humidity);
