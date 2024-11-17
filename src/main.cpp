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
    LOG_PRINTF("\nSHT30 Temperature: %.1fºF - Humidity: %d%%\nHP303B Temperature: %.1fºF - %2.f inHg\n\n", sht30_temperature, sht30_humidity, hp303b_temperature, hp303b_pressure);
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
  while (html->indexOf(escParam(SHT30_TEMPERATURE), 0) != -1) {
    html->replace(escParam(SHT30_TEMPERATURE), String(sht30_temperature));
  }
  while (html->indexOf(escParam(SHT30_HUMIDITY), 0) != -1) {
    html->replace(escParam(SHT30_HUMIDITY), String(sht30_humidity));
  }
  // while (html->indexOf(escParam(HP303B_TEMPERATURE), 0) != -1) {
  //   html->replace(escParam(HP303B_TEMPERATURE), String(hp303b_temperature));
  // }
  // while (html->indexOf(escParam(HP303B_PRESSURE), 0) != -1) {
  //   html->replace(escParam(HP303B_PRESSURE), String(hp303b_pressure));
  // }
}

void setup() {
    #ifdef BS_USE_TELNETSPY
    bs.setExtraRemoteCommands(setExtraRemoteCommands);
  #endif

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 64x48)

  display.fillScreen(BLACK);
  display.display();

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
  device.setModel("SHT30/HP303B");

  // configure sensors
  strcpy(sht30_tempSensorName,   (uniqueId + "_sht30_temperature_sensor").c_str());
  strcpy(sht30_humidSensorName,  (uniqueId + "_sht30_humdity_sensor").c_str());
  // strcpy(hp303b_tempSensorName,  (uniqueId + "_hp303b_temperature_sensor").c_str());
  // strcpy(hp303b_pressSensorName, (uniqueId + "_hp303b_pressure_sensor").c_str());
  // strcpy(voltageSensorName,      (uniqueId + "_battery_voltage_sensor").c_str());
  strcpy(ipAddressSensorName,    (uniqueId + "_ip_address_sensor").c_str());

  sht30_tempSensor   = new HASensorNumber(sht30_tempSensorName, HASensorNumber::PrecisionP1);
  sht30_humidSensor  = new HASensorNumber(sht30_humidSensorName, HASensorNumber::PrecisionP0);
  // hp303b_tempSensor  = new HASensorNumber(hp303b_tempSensorName, HASensorNumber::PrecisionP1);
  // hp303b_pressSensor = new HASensorNumber(hp303b_pressSensorName, HASensorNumber::PrecisionP2);
  // voltageSensor      = new HASensorNumber(voltageSensorName, HASensorNumber::PrecisionP3);
  ipAddressSensor    = new HASensor(ipAddressSensorName);
  
  sht30_tempSensor->setDeviceClass("temperature");
  sht30_tempSensor->setName("SHT30 Temperature");
  sht30_tempSensor->setUnitOfMeasurement("F");

  sht30_humidSensor->setDeviceClass("humidity");
  sht30_humidSensor->setName("SHT30 Humidity");
  sht30_humidSensor->setUnitOfMeasurement("%");

  // hp303b_tempSensor->setDeviceClass("temperature");
  // hp303b_tempSensor->setName("HP303B Temperature");
  // hp303b_tempSensor->setUnitOfMeasurement("F");

  // hp303b_pressSensor->setDeviceClass("atmospheric_pressure");
  // hp303b_pressSensor->setName("HP303B Pressure");
  // hp303b_pressSensor->setUnitOfMeasurement("inHg");

  // voltageSensor->setDeviceClass("voltage");
  // voltageSensor->setName("Battery Voltage");
  // voltageSensor->setUnitOfMeasurement("V");

  ipAddressSensor->setIcon("mdi:ip");
  ipAddressSensor->setName("IP Address");

  // start our HB303 sensor
  // HP303BSensor.begin();

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

  // we've wrapped
  if (lastUpdate != ULONG_MAX && lastUpdate > millis()) {
    lastUpdate = millis();
  }

  int16_t oversampling = 7;
  int16_t ret;

  if (lastUpdate == ULONG_MAX || millis() > lastUpdate + sht_config.publish_interval) {
    int32_t temperature;
    // ret = HP303BSensor.measureTempOnce(temperature, oversampling);
    // if (ret != 0) {
    //   LOG_PRINTF("HP303B Temperature Sensor Failed - ret=[%d]\n", ret);
    // } else {
    //   hp303b_temperature = temperature * 1.8 + 32.0;
    //   if (isSampleValid(hp303b_temperature)) hp303b_tempSensor->setValue(hp303b_temperature);
    //   LOG_PRINTF("HP303B Temperature=[%.2f]ºF", hp303b_temperature);
    // }

    int32_t pressure;
    // ret = HP303BSensor.measurePressureOnce(pressure, oversampling);
    // if (ret != 0) {
    //   LOG_PRINTF("\nHP303B Pressure Sensor Failed - ret=[%d]\n", ret);
    // } else {
    //   hp303b_pressure = pressure / 100.0 * HPA_TO_INHG;
    //   if (isSampleValid(hp303b_pressure)) hp303b_pressSensor->setValue(hp303b_pressure);
    //   LOG_PRINTF(" Pressure=[%.2f]inHg\n", hp303b_pressure);
    // }

    if (sht30.get() == 0) {
      sht30_temperature = sht30.fTemp + SENSOR_TEMPERATURE_VARIANCE;
      sht30_humidity = int(sht30.humidity);

      if (isSampleValid(sht30_temperature) && isSampleValid(sht30_humidity)) {
        LOG_PRINTF("SHT30 Temperature: %.1fºF / Humidity: %d%%\n", sht30_temperature, sht30_humidity);
        LOG_FLUSH();

        update_lcd(sht30_temperature, sht30_humidity);

        sht30_tempSensor->setValue(sht30_temperature);
        sht30_humidSensor->setValue(sht30_humidity);

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

    // pinMode(A0, INPUT);
    // const unsigned int raw = analogRead(A0);
    // const float volt=raw / 1023.0 * 4.2;
    
    // LOG_PRINTF("voltage=[%.4f]v", volt);
    // voltageSensor->setValue(volt);

    mqtt.loop();
    delay(500);

    // bs.requestDeepSleep(sht_config.publish_interval * 1000);
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
    
    display.getTextBounds(String( (temp - 32) * 5 / 9 ) + String(deg), 0, 0, &x, &y, &w, &h);
    display.setCursor(64 - w + 4, 0);
    display.printf("%d", (temp - 32) * 5 / 9 );
    
    display.setTextSize(2);
    display.getTextBounds(String(humidity) + "% ", 0, 0, &x, &y, &w, &h);
    display.setCursor(64 - w, 48 - h);
    display.printf("%d%% ", humidity);
    
    display.display();
}
