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
#include <WEMOS_SHT3X.h>
#include <ArduinoHA.h>

Adafruit_SSD1306 display(0);
SHT3X sht30(0x45);

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

#define TEMPERATURE                    "temperature"
#define HUMIDITY                       "humidity"

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

float           _temperature = 0.0;
tiny_int        _humidity = 0;

HADevice device;
WiFiClient wifiClient;
HAMqtt mqtt(wifiClient, device, 10);

byte deviceId[40];
char deviceName[40];

char tempSensorName[80];
char humidSensorName[80];
char ipAddressSensorName[80];

HASensorNumber* tempSensor;
HASensorNumber* humidSensor;
HASensor* ipAddressSensor;

const bool isSampleValid(const float value);
const String escParam(const char *param_name);
void update_lcd(const tiny_int temp, const tiny_int humidity);


