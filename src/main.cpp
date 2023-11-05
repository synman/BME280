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
    LOG_PRINTLN(bs.builtInRemoteCommandsMenu + "P = Sea Level Pressure\n? = This menu\n");
  }
  if (c == 'P') {
    SEALEVELPRESSURE_HPA = getSeaLevelPressure();
    LOG_PRINTLN("\nSea Level Pressure: [" + String(SEALEVELPRESSURE_HPA) + "]\n");        
  }
}
#endif

float finalTemp = 0.0;
float finalHumid = 0.0;
float finalAlt = 0.0;
float finalPres = 0.0;
short finalRssi = 0;

void updateExtraConfigItem(const String item, String value) {
    if (item == MQTT_SERVER) {
      memset(bme280_config.mqtt_server, CFG_NOT_SET, MQTT_SERVER_LEN);
      if (value.length() > 0) {
          value.toCharArray(bme280_config.mqtt_server, MQTT_SERVER_LEN);
          bme280_config.mqtt_server_flag = CFG_SET;
      } else {
          bme280_config.mqtt_server_flag = CFG_NOT_SET;
      }
      return;
    }

    if (item == MQTT_USER) {
      memset(bme280_config.mqtt_user, CFG_NOT_SET, MQTT_USER_LEN);
      if (value.length() > 0) {
          value.toCharArray(bme280_config.mqtt_user, MQTT_USER_LEN);
          bme280_config.mqtt_user_flag = CFG_SET;
      } else {
          bme280_config.mqtt_user_flag = CFG_NOT_SET;
      }
      return;
    }

    if (item == MQTT_PWD) {
      memset(bme280_config.mqtt_pwd, CFG_NOT_SET, MQTT_PWD_LEN);
      if (value.length() > 0) {
          value.toCharArray(bme280_config.mqtt_pwd, MQTT_PWD_LEN);
          bme280_config.mqtt_pwd_flag = CFG_SET;
      } else {
          bme280_config.mqtt_pwd_flag = CFG_NOT_SET;
      }
      return;
    }

    if (item == SAMPLES_PER_PUBLISH) {
      if (value.toInt() > MIN_SAMPLES_PER_PUBLISH) {
          bme280_config.samples_per_publish = value.toInt();
          bme280_config.samples_per_publish_flag = CFG_SET;
      } else {
          bme280_config.samples_per_publish_flag = CFG_NOT_SET;
          bme280_config.samples_per_publish = DEFAULT_SAMPLES_PER_PUBLISH;
      }
      return;
    }

    if (item ==PUBLISH_INTERVAL) {
      const unsigned long publish_interval = strtoul(value.c_str(), 0, 10);
      if (publish_interval > MIN_PUBLISH_INTERVAL && publish_interval != DEFAULT_PUBLISH_INTERVAL) {
          bme280_config.publish_interval = publish_interval;
          bme280_config.publish_interval_flag = CFG_SET;
      } else {
          bme280_config.publish_interval_flag = CFG_NOT_SET;
          bme280_config.publish_interval = DEFAULT_PUBLISH_INTERVAL;
      }
      return;
    }

    if (item == NWS_STATION) {
      memset(bme280_config.nws_station, CFG_NOT_SET, NWS_STATION_LEN);
      if (value.length() > 0) {
          value.toCharArray(bme280_config.nws_station, NWS_STATION_LEN);
          bme280_config.nws_station_flag = CFG_SET;
      } else {
          bme280_config.nws_station_flag = CFG_NOT_SET;
      }    
      return;
    }
}
void updateExtraHtmlTemplateItems(String *html) {
  while (html->indexOf(escParam(MQTT_SERVER), 0) != -1) {
    html->replace(escParam(MQTT_SERVER), String(bme280_config.mqtt_server));
  }

  while (html->indexOf(escParam(MQTT_USER), 0) != -1) {
    html->replace(escParam(MQTT_USER), String(bme280_config.mqtt_user));
  }

  while (html->indexOf(escParam(MQTT_PWD), 0) != -1) {
    html->replace(escParam(MQTT_PWD), String(bme280_config.mqtt_pwd));
  }

  while (html->indexOf(escParam(SAMPLES_PER_PUBLISH), 0) != -1) {
    html->replace(escParam(SAMPLES_PER_PUBLISH), String(bme280_config.samples_per_publish));
  }

  while (html->indexOf(escParam(PUBLISH_INTERVAL), 0) != -1) {
    html->replace(escParam(PUBLISH_INTERVAL), String(bme280_config.publish_interval));
  }

  while (html->indexOf(escParam(PUBLISH_INTERVAL_IN_SECONDS), 0) != -1) {
    html->replace(escParam(PUBLISH_INTERVAL_IN_SECONDS), String(bme280_config.publish_interval / 1000));
  }

  while (html->indexOf(escParam(NWS_STATION), 0) != -1) {
    html->replace(escParam(NWS_STATION), String(bme280_config.nws_station));
  }

  while (html->indexOf(escParam(TEMPERATURE), 0) != -1) {
    html->replace(escParam(TEMPERATURE), toFloatStr(finalTemp, 3));
  }

  while (html->indexOf(escParam(HUMIDITY), 0) != -1) {
    html->replace(escParam(HUMIDITY), toFloatStr(finalHumid, 3));
  }

  while (html->indexOf(escParam(ALTITUDE), 0) != -1) {
    html->replace(escParam(ALTITUDE), toFloatStr(finalAlt, 3));
  }

  while (html->indexOf(escParam(PRESSURE), 0) != -1) {
    html->replace(escParam(PRESSURE), toFloatStr(finalPres, 3));
  }

  while (html->indexOf(escParam(_RSSI), 0) != -1) {
    html->replace(escParam(_RSSI), String(finalRssi));
  }

  while (html->indexOf(escParam(SEA_LEVEL_ATMOSPHERIC_PRESSURE), 0) != -1) {
      html->replace(escParam(SEA_LEVEL_ATMOSPHERIC_PRESSURE), String(SEALEVELPRESSURE_HPA * HPA_TO_INHG));
  }
}

void setup() {
#ifdef BS_USE_TELNETSPY
  bs.setExtraRemoteCommands(setExtraRemoteCommands);
#endif
  bs.updateExtraConfigItem(updateExtraConfigItem);
  bs.saveExtraConfig([]() { bs.cfg(&bme280_config, sizeof(bme280_config)); });
  bs.updateExtraHtmlTemplateItems(updateExtraHtmlTemplateItems);
  bs.setConfigSize(sizeof(bme280_config));
  bs.setup();

  // get a fresh copy of our extended config struct and initialize it
  memcpy(&bme280_config, bs.cfg(), sizeof(bme280_config));

  updateExtraConfigItem(MQTT_SERVER, bme280_config.mqtt_server);
  updateExtraConfigItem(MQTT_USER, bme280_config.mqtt_user);
  updateExtraConfigItem(MQTT_PWD, bme280_config.mqtt_pwd);
  updateExtraConfigItem(SAMPLES_PER_PUBLISH, String(bme280_config.samples_per_publish));
  updateExtraConfigItem(PUBLISH_INTERVAL, String(bme280_config.publish_interval));
  updateExtraConfigItem(NWS_STATION, bme280_config.nws_station);

  if (!bme.begin()) {
    LOG_PRINTLN("\nCould not find a valid BME280 sensor, check wiring!");
  } else {
    bme_temp->printSensorDetails();
    bme_pressure->printSensorDetails();
    bme_humidity->printSensorDetails();
  }

  // set device details
  String uniqueId = String(bme280_config.hostname);
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
  strcpy(presSensorName,         (uniqueId + "_pressure_sensor").c_str());
  strcpy(altSensorName,          (uniqueId + "_altitude_sensor").c_str());
  strcpy(rssiSensorName,         (uniqueId + "_rssi_sensor").c_str());
  strcpy(seaLevelPresSensorName, (uniqueId + "_sea_level_pressure_sensor").c_str());
  strcpy(ipAddressSensorName,    (uniqueId + "_ip_address_sensor").c_str());

  tempSensor         = new HASensorNumber(tempSensorName, HASensorNumber::PrecisionP1);
  humidSensor        = new HASensorNumber(humidSensorName, HASensorNumber::PrecisionP0);
  presSensor         = new HASensorNumber(presSensorName, HASensorNumber::PrecisionP2);
  altSensor          = new HASensorNumber(altSensorName, HASensorNumber::PrecisionP1);
  rssiSensor         = new HASensorNumber(rssiSensorName, HASensorNumber::PrecisionP0);
  seaLevelPresSensor = new HASensorNumber(seaLevelPresSensorName, HASensorNumber::PrecisionP2);
  ipAddressSensor    = new HASensor(ipAddressSensorName);
  
  tempSensor->setDeviceClass("temperature");
  tempSensor->setName("Temperature");
  tempSensor->setUnitOfMeasurement("F");

  humidSensor->setDeviceClass("humidity");
  humidSensor->setName("Humidity");
  humidSensor->setUnitOfMeasurement("%");

  altSensor->setIcon("mdi:waves-arrow-up");
  altSensor->setName("Altitude");
  altSensor->setUnitOfMeasurement("M");
  
  presSensor->setDeviceClass("atmospheric_pressure");
  presSensor->setName("Barometer");
  presSensor->setUnitOfMeasurement("inHg");

  rssiSensor->setDeviceClass("signal_strength");
  rssiSensor->setName("rssi");
  rssiSensor->setUnitOfMeasurement("dB");

  seaLevelPresSensor->setDeviceClass("atmospheric_pressure");
  seaLevelPresSensor->setName("Sea Level Barometer");
  seaLevelPresSensor->setUnitOfMeasurement("inHg");

  ipAddressSensor->setIcon("mdi:ip");
  ipAddressSensor->setName("IP Address");

  // fire up mqtt client if in station mode and mqtt server configured
  if (bs.wifimode == WIFI_STA && bme280_config.mqtt_server_flag == CFG_SET) {
    mqtt.begin(bme280_config.mqtt_server, bme280_config.mqtt_user, bme280_config.mqtt_pwd);
    LOG_PRINTLN("MQTT started");
  }

  // setup done
  LOG_PRINTLN("\nSystem Ready\n");
}

void loop() {
  bs.loop();
  
  if (bs.wifimode == WIFI_STA && bme280_config.mqtt_server_flag == CFG_SET) {
    // handle MQTT
    mqtt.loop();
  }
  
  const unsigned long sysmillis = millis();

  // recalibrate sea level hPa every 5 minutes
  if (bme280_config.nws_station_flag == CFG_SET && samples.sample_count == 0 && 
     (sysmillis - samples.last_pressure_calibration >= 300000 || samples.last_pressure_calibration == ULONG_MAX)) {
    SEALEVELPRESSURE_HPA = getSeaLevelPressure();
    #ifdef BME280_LOG_LEVEL_BASIC
      LOG_PRINTLN("Sea Level hPa = " + String(SEALEVELPRESSURE_HPA));
    #endif
    samples.last_pressure_calibration = sysmillis;
  }

  // collect a sample every (publish_interval / samples_per_publish) seconds
  if (sysmillis - samples.last_update >= bme280_config.publish_interval / bme280_config.samples_per_publish || samples.last_update == ULONG_MAX) {
    sensors_event_t temp_event, pressure_event, humidity_event;
    samples.sample_count++;

    bme_temp->getEvent(&temp_event);
    const long currentTemp = round(temp_event.temperature * 1000);

    bme_pressure->getEvent(&pressure_event);
    const long currentPres = round(pressure_event.pressure * 1000);
    const long currentAlt = round(bme.readAltitude(SEALEVELPRESSURE_HPA == INVALID_SEALEVELPRESSURE_HPA ? DEFAULT_SEALEVELPRESSURE_HPA : SEALEVELPRESSURE_HPA) * 1000);

    bme_humidity->getEvent(&humidity_event);
    const long currentHumid = round(humidity_event.relative_humidity * 1000);

    const long currentRssi = abs(WiFi.RSSI());
    
    if (currentTemp > samples.high_temperature) samples.high_temperature = currentTemp;
    if (currentHumid > samples.high_humidity) samples.high_humidity = currentHumid;
    if (currentAlt > samples.high_altitude) samples.high_altitude = currentAlt;
    if (currentPres > samples.high_pressure) samples.high_pressure = currentPres;
    if (currentRssi > samples.high_rssi) samples.high_rssi = currentRssi;

    if (currentTemp < samples.low_temperature) samples.low_temperature = currentTemp;
    if (currentHumid < samples.low_humidity) samples.low_humidity = currentHumid;
    if (currentAlt < samples.low_altitude) samples.low_altitude = currentAlt;
    if (currentPres < samples.low_pressure) samples.low_pressure = currentPres;
    if (currentRssi < samples.low_rssi) samples.low_rssi = currentRssi;

    samples.temperature+=currentTemp;
    samples.humidity+=currentHumid;
    samples.altitude+=currentAlt;
    samples.pressure+=currentPres;
    samples.rssi+=currentRssi;

    #ifdef BME280_LOG_LEVEL_BASIC
      LOG_PRINTLN("Gathered Sample #" + String(samples.sample_count));
    #endif

    #ifdef BME280_LOG_LEVEL_FULL
      LOG_PRINT(F("Temperature = "));
      LOG_PRINT(currentTemp / 1000.0, 3);
      LOG_PRINTLN(" *C");

      LOG_PRINT(F("Humidity    = "));
      LOG_PRINT(currentHumid / 1000.0, 3);
      LOG_PRINTLN(" %");

      LOG_PRINT(F("Altitude    = "));
      LOG_PRINT(currentAlt / 1000.0, 3);
      LOG_PRINTLN(" m");

      LOG_PRINT(F("Pressure    = "));
      LOG_PRINT(currentPres / 1000.0, 3);
      LOG_PRINTLN(" hPa");

      LOG_PRINT(F("rssi        = "));
      LOG_PRINT(currentRssi * -1);
      LOG_PRINTLN(" dB");
    #endif

    if (samples.sample_count >= bme280_config.samples_per_publish) {
        // remove highest and lowest values (outliers)
        samples.temperature = samples.temperature - (samples.low_temperature + samples.high_temperature);
        samples.humidity = samples.humidity - (samples.low_humidity + samples.high_humidity);
        samples.altitude = samples.altitude - (samples.low_altitude + samples.high_altitude);
        samples.pressure = samples.pressure - (samples.low_pressure + samples.high_pressure);
        samples.rssi = samples.rssi - (samples.low_rssi + samples.high_rssi);

        // account for removed outliers
        samples.sample_count-=2;

        // use the average of what remains
        finalTemp = samples.temperature / samples.sample_count / 1000.0 * 1.8 + 32;
        finalHumid = samples.humidity / samples.sample_count / 1000.0;
        finalAlt = samples.altitude / samples.sample_count / 1000.0;
        finalPres = samples.pressure / samples.sample_count / 1000.0 * HPA_TO_INHG;
        finalRssi = samples.rssi / samples.sample_count * -1;

        // publish our normalized values 
        #ifdef BME280_LOG_LEVEL_BASIC
          LOG_PRINTLN("Normalized Result (Published)");
        #endif

        #ifdef BME280_LOG_LEVEL_FULL
          LOG_PRINT(F("Temperature = "));
          LOG_PRINT(finalTemp, 3);
          LOG_PRINT(" *F (");
          LOG_PRINT(samples.temperature / samples.sample_count / 1000.0, 3);
          LOG_PRINTLN(" *C)");

          LOG_PRINT(F("Humidity    = "));
          LOG_PRINT(finalHumid, 3);
          LOG_PRINTLN(" %");

          LOG_PRINT(F("Altitude    = "));
          LOG_PRINT(finalAlt, 3);
          LOG_PRINTLN(" m");

          LOG_PRINT(F("Pressure    = "));
          LOG_PRINT(finalPres, 3);
          LOG_PRINT(" inHg (");
          LOG_PRINT(samples.pressure / samples.sample_count / 1000.0, 3);
          LOG_PRINTLN(" hPa)");

          LOG_PRINT(F("rssi        = "));
          LOG_PRINT(finalRssi);
          LOG_PRINTLN(" dB");
        #endif

        if (isSampleValid(finalTemp)) tempSensor->setValue(finalTemp);
        if (isSampleValid(finalHumid)) humidSensor->setValue(finalHumid);
        if (isSampleValid(finalAlt) && SEALEVELPRESSURE_HPA != INVALID_SEALEVELPRESSURE_HPA) altSensor->setValue(finalAlt);
        if (isSampleValid(finalPres)) presSensor->setValue(finalPres);
        if (isSampleValid(finalRssi)) rssiSensor->setValue(finalRssi);

        if (isSampleValid(SEALEVELPRESSURE_HPA) && bme280_config.nws_station_flag == CFG_SET) seaLevelPresSensor->setValue(SEALEVELPRESSURE_HPA * HPA_TO_INHG);

        if (bs.wifimode == WIFI_STA) 
          ipAddressSensor->setValue(WiFi.localIP().toString().c_str());
        
        bs.updateHtmlTemplate("/index.template.html", false);

        // reset our samples structure
        samples.temperature = 0L;
        samples.humidity = 0L;
        samples.altitude = 0L;
        samples.pressure = 0L;
        samples.rssi = 0L;

        samples.high_temperature = LONG_MIN;
        samples.high_humidity = LONG_MIN;
        samples.high_altitude = LONG_MIN;
        samples.high_pressure = LONG_MIN;
        samples.high_rssi = LONG_MIN;

        samples.low_temperature = LONG_MAX;
        samples.low_humidity = LONG_MAX;
        samples.low_altitude = LONG_MAX;
        samples.low_pressure = LONG_MAX;
        samples.low_rssi = LONG_MAX;

        samples.sample_count = 0;

        printHeapStats();
        bs.blink();
    }
    samples.last_update = sysmillis;
  }
}

const bool isSampleValid(const float value) {
    return value < SHRT_MAX && value > SHRT_MIN;
}

const String toFloatStr(const float value, const short decimal_places) {
    char buf[20];
    sprintf(buf, "%.*f", decimal_places, value);
    return String(buf);
}

const float getSeaLevelPressure() {
    if (bme280_config.nws_station_flag == CFG_NOT_SET) {
        #ifdef BME280_LOG_LEVEL_FULL
          LOG_PRINTLN("NWS Station is not set - using default sea level pressure");
        #endif
        return DEFAULT_SEALEVELPRESSURE_HPA;
    }

    if (bs.wifimode == WIFI_AP) {
        #ifdef BME280_LOG_LEVEL_FULL
          LOG_PRINTLN("In AP mode - altitude will be ignored");
        #endif
        return INVALID_SEALEVELPRESSURE_HPA;
    }

    const String observationsUrl = "https://api.weather.gov/stations/" + String(bme280_config.nws_station) + "/observations/latest";

#ifdef esp32
    DynamicJsonDocument doc(8192);
    WiFiClientSecure httpsClient;
    
    httpsClient.setInsecure();
    httpsClient.setTimeout(5000);

    if (httpsClient.connect("api.weather.gov", 443)) {
        httpsClient.println("GET " + observationsUrl + " HTTP/1.0");
        httpsClient.println("Host: api.weather.gov");
        httpsClient.println("User-Agent: curl/8.1.2");
        httpsClient.println("Accept: application/json");
        httpsClient.println("Connection: close");
        httpsClient.println();
    } else {
        LOG_PRINTLN("Unable to connect to NWS - altitude will be ignored");
        return INVALID_SEALEVELPRESSURE_HPA;
    }

    // read headers
    while (httpsClient.connected()) {
        String line = httpsClient.readStringUntil('\n');
        if (line == "\r") break;
    }

    if (!httpsClient.connected()) {
        LOG_PRINTLN("NWS dropped connnection - altitude will be ignored");
        return INVALID_SEALEVELPRESSURE_HPA;
    }

    deserializeJson(doc, httpsClient.readString());
    httpsClient.stop();

    String properties = doc["properties"];
    deserializeJson(doc, properties);
    String seaLevelPressure = doc["seaLevelPressure"];
    deserializeJson(doc, seaLevelPressure);

    String value = doc["value"];

    if (isNumeric(value)) {
        return value.toInt() / 100.0;
    } 

    LOG_PRINTLN("Invalid response from NWS - altitude will be ignored");
    return INVALID_SEALEVELPRESSURE_HPA;
#else
    HTTPClient httpClient;
    std::unique_ptr<BearSSL::WiFiClientSecure>httpsClient(new BearSSL::WiFiClientSecure);

    httpsClient->setInsecure();
    httpsClient->setTimeout(3000);
    httpsClient->setBufferSizes(4096, 255);

    if (httpClient.begin(*httpsClient, observationsUrl)) {
        httpClient.addHeader("Host", "api.weather.gov");
        httpClient.addHeader("Accept", "application/json");
        httpClient.addHeader("User-Agent", "curl/8.1.2");
        httpClient.addHeader("Connection", "close");

        char last[257] = {0};
        char data[129] = {0};

        if (httpClient.GET() == HTTP_CODE_OK) {
          while (httpClient.getStream().available()) {
            size_t bytes = httpClient.getStream().readBytes(data, 128);
            data[bytes] = 0;
            strcat(last, data);

            if (strstr(last, "visibility")) {
              const String parse = String(last);
              const int start = parse.indexOf("\"value\": ");
              const int end = parse.indexOf(",", start);
              String pa = parse.substring(start + 9, end);

              if (isNumeric(pa)) {
                httpClient.end();
                return pa.toInt() / 100.0;
              } else {
                httpClient.end();
                LOG_PRINTLN("Bad response from NWS - altitude will be ignored");    
                return INVALID_SEALEVELPRESSURE_HPA;
              }
            }

            memcpy(last, data, bytes + 1);
          }
          httpClient.end();
          LOG_PRINTLN("Sea Level Barometer missing from NWS response - altitude will be ignored");    
          return INVALID_SEALEVELPRESSURE_HPA;
        } else {
            httpClient.end();
            LOG_PRINTLN("Bad HTTP Response Code from NWS - altitude will be ignored");
            return INVALID_SEALEVELPRESSURE_HPA;
        }
    } else {
        httpClient.end();
        LOG_PRINTLN("Unable to connect to NWS - altitude will be ignored");
        return INVALID_SEALEVELPRESSURE_HPA;
    }
#endif
}

const bool isNumeric(const String str) {
  bool seenDecimal = false;
 
  if (str.length() == 0) return false;
  
  for (tiny_int i = 0; i < str.length() ; ++i) {
    if (isDigit(str.charAt(i))) continue;
    if (str.charAt(i) == '.') {
      if (seenDecimal) return false;
      seenDecimal = true;
      continue;
    }
    return false;
  }
  return true;
}

const String escParam(const char * param_name) {
  char buf[64];
  sprintf(buf, "{%s}", param_name);
  return String(buf);
}

void printHeapStats() {
  #ifdef BME280_LOG_LEVEL_BASIC
    uint32_t myfree;
    uint32_t mymax;
    uint8_t myfrag;

    #ifdef esp32 
      const uint32_t size = ESP.getHeapSize();
      const uint32_t free = ESP.getFreeHeap();
      const uint32_t max = ESP.getMaxAllocHeap();
      const uint32_t min = ESP.getMinFreeHeap();
      LOG_PRINTF("(%ld) -> size: %5d - free: %5d - max: %5d - min: %5d <-\n", millis(), size, free, max, min);
    #else
      ESP.getHeapStats(&myfree, &mymax, &myfrag);
      LOG_PRINTF("(%ld) -> free: %5d - max: %5d - frag: %3d%% <-\n", millis(), myfree, mymax, myfrag);
    #endif
  #endif

  return;
}
