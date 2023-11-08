/***************************************************************************
Copyright © 2023 Shell M. Shrader <shell at shellware dot com>
----------------------------------------------------------------------------
This work is free. You can redistribute it and/or modify it under the
terms of the Do What The Fuck You Want To Public License, Version 2,
as published by Sam Hocevar. See the COPYING file for more details.
****************************************************************************/
#include "main.h"

#ifdef BS_USE_TELNETSPY
void setExtraRemoteCommands(char c) {
  if (c == '?') {
    LOG_PRINTLN(bs.builtInRemoteCommandsMenu + "G = Get Temp & Humidity\n? = This menu\n");
  }
  if (c == 'G') {
    LOG_PRINTF("\nTemperature: %.1fºF - Humidity: %d%%\n\n", _temperature, _humidity);
  }
}
#endif

void updateExtraConfigItem(const String item, String value) {
  if (item == MQTT_SERVER) {
    memset(sht_config.mqtt_server, CFG_NOT_SET, MQTT_SERVER_LEN);
    if (value.length() > 0) {
        value.toCharArray(sht_config.mqtt_server, MQTT_SERVER_LEN);
        sht_config.mqtt_server_flag = CFG_SET;
    } else {
        sht_config.mqtt_server_flag = CFG_NOT_SET;
    }
    return;
  }
  if (item == MQTT_USER) {
    memset(sht_config.mqtt_user, CFG_NOT_SET, MQTT_USER_LEN);
    if (value.length() > 0) {
        value.toCharArray(sht_config.mqtt_user, MQTT_USER_LEN);
        sht_config.mqtt_user_flag = CFG_SET;
    } else {
        sht_config.mqtt_user_flag = CFG_NOT_SET;
    }
    return;
  }
  if (item == MQTT_PWD) {
    memset(sht_config.mqtt_pwd, CFG_NOT_SET, MQTT_PWD_LEN);
    if (value.length() > 0) {
        value.toCharArray(sht_config.mqtt_pwd, MQTT_PWD_LEN);
        sht_config.mqtt_pwd_flag = CFG_SET;
    } else {
        sht_config.mqtt_pwd_flag = CFG_NOT_SET;
    }
    return;
  }
  if (item == PUBLISH_INTERVAL) {
    const unsigned long publish_interval = strtoul(value.c_str(), 0, 10);
    if (publish_interval >= MIN_PUBLISH_INTERVAL && publish_interval != DEFAULT_PUBLISH_INTERVAL) {
        sht_config.publish_interval = publish_interval;
        sht_config.publish_interval_flag = CFG_SET;
    } else {
        sht_config.publish_interval_flag = CFG_NOT_SET;
        sht_config.publish_interval = DEFAULT_PUBLISH_INTERVAL;
    }
    return;
  }
}
void updateExtraHtmlTemplateItems(String *html) {
  while (html->indexOf(escParam(MQTT_SERVER), 0) != -1) {
    html->replace(escParam(MQTT_SERVER), String(sht_config.mqtt_server));
  }
  while (html->indexOf(escParam(MQTT_USER), 0) != -1) {
    html->replace(escParam(MQTT_USER), String(sht_config.mqtt_user));
  }
  while (html->indexOf(escParam(MQTT_PWD), 0) != -1) {
    html->replace(escParam(MQTT_PWD), String(sht_config.mqtt_pwd));
  }
  while (html->indexOf(escParam(PUBLISH_INTERVAL), 0) != -1) {
    html->replace(escParam(PUBLISH_INTERVAL), String(sht_config.publish_interval));
  }
  while (html->indexOf(escParam(PUBLISH_INTERVAL_IN_SECONDS), 0) != -1) {
    html->replace(escParam(PUBLISH_INTERVAL_IN_SECONDS), String(int(sht_config.publish_interval / 1000)));
  }
  while (html->indexOf(escParam(TEMPERATURE), 0) != -1) {
    html->replace(escParam(TEMPERATURE), String(_temperature));
  }
  while (html->indexOf(escParam(HUMIDITY), 0) != -1) {
    html->replace(escParam(HUMIDITY), String(_humidity));
  }
}

void setup() {
  #ifdef BS_USE_TELNETSPY
    bs.setExtraRemoteCommands(setExtraRemoteCommands);
  #endif

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 64x48)

  // display.fillScreen(BLACK);
  // display.display();

  bs.setConfig(&sht_config, sizeof(sht_config));
  bs.updateExtraConfigItem(updateExtraConfigItem);
  bs.updateExtraHtmlTemplateItems(updateExtraHtmlTemplateItems);

  // if setup fails, we fail
  if (!bs.setup()) return;

  // // initialize our extended config struct if values are not set
  updateExtraConfigItem(MQTT_SERVER, String(sht_config.mqtt_server));
  updateExtraConfigItem(MQTT_USER, String(sht_config.mqtt_user));
  updateExtraConfigItem(MQTT_PWD, String(sht_config.mqtt_pwd));
  updateExtraConfigItem(PUBLISH_INTERVAL, String(sht_config.publish_interval));
  
  bs.updateSetupHtml();
  bs.updateIndexHtml();

  // set device details
  String uniqueId = String(sht_config.hostname);
  std::replace(uniqueId.begin(), uniqueId.end(), '-', '_');

  for (tiny_int i = 0; i < uniqueId.length(); i++) {
    deviceId[i] = (byte)uniqueId[i];
  }

  strcpy(deviceName, uniqueId.c_str());

  device.setUniqueId(deviceId, uniqueId.length());
  device.setName(deviceName);
  device.setSoftwareVersion("1.0.0");
  device.setManufacturer("Shell M. Shrader");
  device.setModel("BME280");

  // configure sensors
  strcpy(tempSensorName,         (uniqueId + "_temperature_sensor").c_str());
  strcpy(humidSensorName,        (uniqueId + "_humdity_sensor").c_str());
  strcpy(ipAddressSensorName,    (uniqueId + "_ip_address_sensor").c_str());

  tempSensor         = new HASensorNumber(tempSensorName, HASensorNumber::PrecisionP1);
  humidSensor        = new HASensorNumber(humidSensorName, HASensorNumber::PrecisionP0);
  ipAddressSensor    = new HASensor(ipAddressSensorName);
  
  tempSensor->setDeviceClass("temperature");
  tempSensor->setName("Temperature");
  tempSensor->setUnitOfMeasurement("F");

  humidSensor->setDeviceClass("humidity");
  humidSensor->setName("Humidity");
  humidSensor->setUnitOfMeasurement("%");

  ipAddressSensor->setIcon("mdi:ip");
  ipAddressSensor->setName("IP Address");

  // fire up mqtt client if in station mode and mqtt server configured
  if (bs.wifimode == WIFI_STA && sht_config.mqtt_server_flag == CFG_SET) {
    mqtt.begin(sht_config.mqtt_server, sht_config.mqtt_user, sht_config.mqtt_pwd);
    LOG_PRINTLN("MQTT started");
  }

  // setup done
  LOG_PRINTLN("\nSystem Ready\n");
}

void loop() {
  bs.loop();

  if (bs.wifimode == WIFI_STA && sht_config.mqtt_server_flag == CFG_SET) {
    // handle MQTT
    mqtt.loop();
  }

  static unsigned long lastUpdate = bs.resetReason == RESET_REASON_DEEP_SLEEP_AWAKE ? ULONG_MAX : 0;

  if (lastUpdate == ULONG_MAX || millis() > lastUpdate + sht_config.publish_interval) {
    if (sht30.get() == 0) {
      _temperature = sht30.fTemp + SENSOR_TEMPERATURE_VARIANCE;
      _humidity = int(sht30.humidity);

      if (isSampleValid(_temperature) && isSampleValid(_humidity)) {
        LOG_PRINTF("Temperature: %.1fºF / Humidity: %d%%\n", _temperature, _humidity);
        LOG_FLUSH();

        update_lcd(_temperature, _humidity);

        tempSensor->setValue(_temperature);
        humidSensor->setValue(_humidity);

        if (bs.wifimode == WIFI_STA) 
          ipAddressSensor->setValue(WiFi.localIP().toString().c_str());
        
        if (bs.resetReason != RESET_REASON_DEEP_SLEEP_AWAKE) bs.updateHtmlTemplate("/index.template.html", false);      
      } else {
        LOG_PRINTLN("temperature and humidity sample rejected!");
      }
    } else {
      LOG_PRINTLN("SHT30 results invalid!");
    }
    lastUpdate = millis();

    mqtt.loop();
    delay(500);

    bs.requestDeepSleep(sht_config.publish_interval * 1000);
  }
}

const bool isSampleValid(const float value) {
    return value < 150 && value > -50;
}

const String escParam(const char * param_name) {
  char buf[64];
  sprintf(buf, "{%s}", param_name);
  return String(buf);
}

void update_lcd(const tiny_int temp, const tiny_int humidity) {
    display.fillScreen(BLACK);
    display.setCursor(0,0);
    display.setTextSize(3);
    display.setTextColor(WHITE);

    char deg = 9;
    int16_t x = 0, y = 0;
    uint16_t w = 0, h = 0;

    display.getTextBounds(String(deg), 0, 0, &x, &y, &w, &h);
    display.setCursor(64 - w, -7);
    display.printf("%c", deg);
    
    display.getTextBounds(String(temp) + String(deg), 0, 0, &x, &y, &w, &h);
    display.setCursor(64 - w + 4, 0);
    display.printf("%d", temp);
    
    display.setTextSize(2);
    display.getTextBounds(String(humidity) + "% ", 0, 0, &x, &y, &w, &h);
    display.setCursor(64 - w, 48 - h);
    display.printf("%d%% ", humidity);
    
    display.display();
}
