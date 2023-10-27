/***************************************************************************
Copyright © 2023 Shell M. Shrader <shell at shellware dot com>
----------------------------------------------------------------------------
This work is free. You can redistribute it and/or modify it under the
terms of the Do What The Fuck You Want To Public License, Version 2,
as published by Sam Hocevar. See the COPYING file for more details.
****************************************************************************/
#include "main.h"

void setup() {
  INIT_LED;
  
  LOG_WELCOME_MSG("\n\nBME280 diagnostics - Press ? for a list of commands\n");
  LOG_BEGIN(1500000);

  LOG_PRINTLN("\n\nBME280 Sensor Event Publisher v1.0.0");

  if (!bme.begin()) {
    LOG_PRINTLN("\nCould not find a valid BME280 sensor, check wiring!");
  } else {
    bme_temp->printSensorDetails();
    bme_pressure->printSensorDetails();
    bme_humidity->printSensorDetails();
  }
  
  // wire up EEPROM storage and config
  wireConfig();

  // start and mount our littlefs file system
  if (!LittleFS.begin()) {
    LOG_PRINTLN("\nAn Error has occurred while initializing LittleFS\n");
  } else {
    #ifdef BME280_DEBUG
      #ifdef esp32
        const size_t fs_size = LittleFS.totalBytes() / 1000;
        const size_t fs_used = LittleFS.usedBytes() / 1000;
      #else
        FSInfo fs_info;
        LittleFS.info(fs_info);
        const size_t fs_size = fs_info.totalBytes / 1000;
        const size_t fs_used = fs_info.usedBytes / 1000;
      #endif
    #endif
    LOG_PRINTLN();
    LOG_PRINTLN("    Filesystem size: [" + String(fs_size) + "] KB");
    LOG_PRINTLN("         Free space: [" + String(fs_size - fs_used) + "] KB");
    LOG_PRINTLN("          Free Heap: [" + String(ESP.getFreeHeap()) + "]");
  }

  // Connect to Wi-Fi network with SSID and password
  // or fall back to AP mode
  WiFi.persistent(false);
  WiFi.setAutoConnect(false);
  WiFi.setAutoReconnect(false);
  WiFi.hostname(config.hostname);
  WiFi.mode(wifimode);

  #ifdef esp32
    static const wifi_event_id_t disconnectHandler = WiFi.onEvent([](WiFiEvent_t event) 
      { 
        wifiState = event; 
        if (wifiState == WIFI_DISCONNECTED) {
          LOG_PRINTLN("\nWiFi disconnected");
          LOG_FLUSH();
        }
      });
  #else
    static const WiFiEventHandler disconnectHandler = WiFi.onStationModeDisconnected([](WiFiEventStationModeDisconnected event) 
      {
        LOG_PRINTF("\nWiFi disconnected - reason: %d\n", event.reason);
        LOG_FLUSH();
        wifiState = WIFI_DISCONNECTED;
      });
  #endif

  // WiFi.scanNetworks will return the number of networks found
  uint8_t nothing = 0;
  uint8_t *bestBssid;
  bestBssid = &nothing;
  short bestRssi = SHRT_MIN;

  LOG_PRINTLN("\nScanning Wi-Fi networks. . .");
  int n = WiFi.scanNetworks();

  // arduino is too stupid to know which AP has the best signal
  // when connecting to an SSID with multiple BSSIDs (WAPs / Repeaters)
  // so we find the best one and tell it to use it
  if (n > 0 && String(config.ssid).length() > 0) {
    for (int i = 0; i < n; ++i) {
      LOG_PRINTF("   ssid: %s - rssi: %d\n", WiFi.SSID(i), WiFi.RSSI(i));
      if (WiFi.SSID(i).equals(config.ssid) && WiFi.RSSI(i) > bestRssi) {
        bestRssi = WiFi.RSSI(i);
        bestBssid = WiFi.BSSID(i);
      }
    }
  }

  if (wifimode == WIFI_STA && bestRssi != SHRT_MIN) {
    WiFi.disconnect();
    delay(100);
    LOG_PRINTF("\nConnecting to %s / %d dB ", config.ssid, bestRssi);
    WiFi.begin(config.ssid, config.ssid_pwd, 0, bestBssid, true);
    for (tiny_int x = 0; x < 120 && WiFi.status() != WL_CONNECTED; x++) {
      blink();
      LOG_PRINT(".");
    }

    LOG_PRINTLN();

    if (WiFi.status() == WL_CONNECTED) {
      // initialize time
      struct tm timeinfo;
      char timebuf[255];

      configTime(0, 0, "pool.ntp.org");
      setenv("TZ", "EST+5EDT,M3.2.0/2,M11.1.0/2", 1);
      tzset();

      if (!getLocalTime(&timeinfo)) {
        LOG_PRINTLN("\nFailed to obtain time");
      } else {
        sprintf(timebuf, "%4d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        LOG_PRINT("\nCurrent Time: ");
        LOG_PRINTLN(timebuf);
      }
    }
  }

  if (WiFi.status() != WL_CONNECTED || wifimode == WIFI_AP) {
    wifimode = WIFI_AP;
    WiFi.mode(wifimode);
    WiFi.softAP(config.hostname);
    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
    LOG_PRINTLN("\nSoftAP [" + String(config.hostname) + "] started");
  }

  LOG_PRINTLN();      
  LOG_PRINT("    Hostname: "); LOG_PRINTLN(config.hostname);
  LOG_PRINT("Connected to: "); LOG_PRINTLN(wifimode == WIFI_STA ? config.ssid : config.hostname);
  LOG_PRINT("  IP address: "); LOG_PRINTLN(wifimode == WIFI_STA ? WiFi.localIP().toString() : WiFi.softAPIP().toString());
  LOG_PRINT("        RSSI: "); LOG_PRINTLN(String(WiFi.RSSI()) + " dB");

  // enable mDNS via espota and enable ota
  wireArduinoOTA(config.hostname);

  // begin Elegant OTA
  ElegantOTA.begin(&server);
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);

  LOG_PRINTLN("ElegantOTA started");

  // set device details
  String uniqueId = String(config.hostname);
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

  tempSensor         = new HASensorNumber(tempSensorName, HASensorNumber::PrecisionP3);
  humidSensor        = new HASensorNumber(humidSensorName, HASensorNumber::PrecisionP3);
  presSensor         = new HASensorNumber(presSensorName, HASensorNumber::PrecisionP3);
  altSensor          = new HASensorNumber(altSensorName, HASensorNumber::PrecisionP3);
  rssiSensor         = new HASensorNumber(rssiSensorName, HASensorNumber::PrecisionP0);
  seaLevelPresSensor = new HASensorNumber(seaLevelPresSensorName, HASensorNumber::PrecisionP3);
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

  updateHtmlTemplate("/setup.template.html", "", "", "", "", "", false);
  LOG_PRINTLN("setup.html updated");

  // wire up http server and paths
  wireWebServerAndPaths();

  // fire up mqtt client if in station mode
  if (wifimode == WIFI_STA) {
    mqtt.begin(config.mqtt_server, config.mqtt_user, config.mqtt_pwd);
    LOG_PRINTLN("MQTT started");
  }

  // wire up our custom watchdog
  #ifdef esp32
    watchDogTimer = timerBegin(2, 80, true);
    timerAttachInterrupt(watchDogTimer, &watchDogInterrupt, true);
    timerAlarmWrite(watchDogTimer, WATCHDOG_TIMEOUT_S * 1000000, false);
    timerAlarmEnable(watchDogTimer);
  #else
    ITimer.attachInterruptInterval(WATCHDOG_TIMEOUT_S * 1000000, TimerHandler);
  #endif

  LOG_PRINTLN("Watchdog started");

  // setup done
  LOG_PRINTLN("\nSystem Ready");
}

void loop() {
  // handle TelnetSpy if BME280_DEBUG is defined
  LOG_HANDLE();

  // handle a reboot request if pending
  if (esp_reboot_requested) {
    ElegantOTA.loop();
    delay(1000);
    LOG_PRINTLN("\nReboot triggered. . .");
    LOG_HANDLE();
    LOG_FLUSH();
    ESP.restart();
    while (1) {} // will never get here
  }

  // captive portal if in AP mode
  if (wifimode == WIFI_AP) {
    dnsServer.processNextRequest();
  } else {
    if (wifiState == WIFI_DISCONNECTED) {
      LOG_PRINTLN("sleeping for 180 seconds. . .");
      for (tiny_int x = 0; x < 180; x++) {
        delay(1000);
        watchDogRefresh();
      }
      LOG_PRINTLN("\nRebooting due to no wifi connection");
      esp_reboot_requested = true;
      return;
    }

    if (config.mqtt_server_flag == CFG_SET) {
      // handle MQTT
      mqtt.loop();
    }

    // check for OTA
    ArduinoOTA.handle();
    ElegantOTA.loop();
  }

  // rebuild setup.html on main thread
  if (setup_needs_update) {
    LOG_PRINTLN("\n----- rebuilding /setup.html");
    updateHtmlTemplate("/setup.template.html", "", "", "", "", "",false);
    LOG_PRINTLN("-----  /setup.html rebuilt");
    setup_needs_update = false;
  }

  const unsigned long sysmillis = millis();

  // recalibrate sea level hPa every 5 minutes
  if (config.nws_station_flag == CFG_SET && samples.sample_count == 0 && 
     (sysmillis - samples.last_pressure_calibration >= 300000 || samples.last_pressure_calibration == ULONG_MAX)) {
    SEALEVELPRESSURE_HPA = getSeaLevelPressure();
    LOG_PRINTLN("\nSea Level hPa = " + String(SEALEVELPRESSURE_HPA));
    samples.last_pressure_calibration = sysmillis;
  }

  // collect a sample every (publish_interval / samples_per_publish) seconds
  if (sysmillis - samples.last_update >= config.publish_interval / config.samples_per_publish || samples.last_update == ULONG_MAX) {
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

    LOG_PRINTLN("\nGathered Sample #" + String(samples.sample_count));

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

    if (samples.sample_count >= config.samples_per_publish) {
        // remove highest and lowest values (outliers)
        samples.temperature = samples.temperature - (samples.low_temperature + samples.high_temperature);
        samples.humidity = samples.humidity - (samples.low_humidity + samples.high_humidity);
        samples.altitude = samples.altitude - (samples.low_altitude + samples.high_altitude);
        samples.pressure = samples.pressure - (samples.low_pressure + samples.high_pressure);
        samples.rssi = samples.rssi - (samples.low_rssi + samples.high_rssi);

        // account for removed outliers
        samples.sample_count-=2;

        // use the average of what remains
        const float finalTemp = samples.temperature / samples.sample_count / 1000.0 * 1.8 + 32;
        const float finalHumid = samples.humidity / samples.sample_count / 1000.0;
        const float finalAlt = samples.altitude / samples.sample_count / 1000.0;
        const float finalPres = samples.pressure / samples.sample_count / 1000.0 * HPA_TO_INHG;
        const short finalRssi = samples.rssi / samples.sample_count * -1;

        // publish our normalized values 
        LOG_PRINTLN("\nNormalized Result (Published)");

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

        if (isSampleValid(finalTemp)) tempSensor->setValue(finalTemp);
        if (isSampleValid(finalHumid)) humidSensor->setValue(finalHumid);
        if (isSampleValid(finalAlt) && SEALEVELPRESSURE_HPA != INVALID_SEALEVELPRESSURE_HPA) altSensor->setValue(finalAlt);
        if (isSampleValid(finalPres)) presSensor->setValue(finalPres);
        if (isSampleValid(finalRssi)) rssiSensor->setValue(finalRssi);

        if (isSampleValid(SEALEVELPRESSURE_HPA) && config.nws_station_flag == CFG_SET) seaLevelPresSensor->setValue(SEALEVELPRESSURE_HPA * HPA_TO_INHG);

        const String ipAddr = wifimode == WIFI_STA ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
        ipAddressSensor->setValue(ipAddr.c_str());
        
        updateHtmlTemplate("/index.template.html", 
                            toFloatStr(finalTemp, 3), 
                            toFloatStr(finalHumid, 3), 
                            toFloatStr(finalAlt, 3), 
                            toFloatStr(finalPres, 3), 
                            String(finalRssi));

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
        blink();
    }
    samples.last_update = sysmillis;
  }

  // reboot if in AP mode and no activity for 5 minutes
  if (wifimode == WIFI_AP && !ap_mode_activity && millis() >= 300000UL) {
    LOG_PRINTF("\nNo AP activity for 5 minutes -- triggering reboot");
    esp_reboot_requested = true;
  }

  // 24 hour mandatory reboot
  if (millis() >= 86400000UL) {
    LOG_PRINTF("\nTriggering mandatory 24 hour reboot");
    esp_reboot_requested = true;
  }

  watchDogRefresh();
}
