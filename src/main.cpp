/***************************************************************************
  This is a library for the BME280 humidity, temperature & pressure sensor
  This example shows how to take Sensor Events instead of direct readings

  Designed specifically to work with the Adafruit BME280 Breakout
  ----> http://www.adafruit.com/products/2652

  These sensors use I2C or SPI to communicate, 2 or 4 pins are required
  to interface.

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing products
  from Adafruit!

  Written by Limor Fried & Kevin Townsend for Adafruit Industries.
  BSD license, all text above must be included in any redistribution

  Customizations added by Shell M. Shrader - Oct 15 2023
 ***************************************************************************/

#include "main.h"

void setup() {
  INIT_LED;
  
  SerialAndTelnet.setWelcomeMsg(F("BME280 diagnostics\r\n\n"));
  LOG.begin(1500000);

  LOG.println("");
  LOG.println("");
  LOG.println("BME280 Sensor Event Publisher v1.0.0");

  while (!bme.begin()) {
    LOG.println(F("Could not find a valid BME280 sensor, check wiring!"));
    delay(1000);
  }

  bme_temp->printSensorDetails();
  bme_pressure->printSensorDetails();
  bme_humidity->printSensorDetails();

  // wire up EEPROM storage and config
  wireConfig();

  // Connect to Wi-Fi network with SSID and password
  // or fall back to AP mode
  WiFi.hostname(config.hostname);
  WiFi.mode(wifimode);

  // WiFi.scanNetworks will return the number of networks found
  uint8_t *bestBssid;
  short bestRssi = SHRT_MIN;
  int n = WiFi.scanNetworks();

  // arduino is too stupid to know which AP has the best signal
  // when connecting to an SSID with multiple BSSIDs (WAPs / Repeaters)
  // so we find the best one and tell it to use it
  if (n > 0 && String(config.ssid).length() > 0) {
    for (int i = 0; i < n; ++i) {
      if (WiFi.SSID(i).equals(config.ssid) && WiFi.RSSI(i) > bestRssi) {
        bestRssi = WiFi.RSSI(i);
        bestBssid = WiFi.BSSID(i);
      }
    }
  }

  if (wifimode == WIFI_STA && bestRssi != SHRT_MIN) {
    LOG.print("Connecting to "); LOG.print(config.ssid);
    WiFi.begin(config.ssid, config.ssid_pwd, 0, bestBssid, true);
    for (tiny_int x = 0; x < 60 && WiFi.status() != WL_CONNECTED; x++) {
      blink();
      LOG.print(".");
    }

    LOG.println("");

    if (WiFi.status() == WL_CONNECTED) {
      // initialize time
      configTime(0, 0, "pool.ntp.org");
      setenv("TZ", "EST+5EDT,M3.2.0/2,M11.1.0/2", 1);
      tzset();

      if (!getLocalTime(&timeinfo)) {
        LOG.println("Failed to obtain time");
      }
      sprintf(buf, "%4d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
      LOG.print("\nCurrent Time: ");
      LOG.println(buf);
    }
  }

  if (WiFi.status() != WL_CONNECTED || wifimode == WIFI_AP) {
    wifimode = WIFI_AP;
    WiFi.mode(wifimode);
    WiFi.softAP(config.hostname);
    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
    LOG.println("\nSoftAP [" + String(config.hostname) + "] started\n");
  }
      
  LOG.print("    Hostname: "); LOG.println(config.hostname);
  LOG.print("Connected to: "); LOG.println(wifimode == WIFI_STA ? config.ssid : config.hostname);
  LOG.print("  IP address: "); LOG.println(wifimode == WIFI_STA ? WiFi.localIP().toString() : WiFi.softAPIP().toString());
  LOG.print("        RSSI: "); LOG.println(String(WiFi.RSSI()) + " dB\n");

  // enable mDNS via espota and enable ota
  wireArduinoOTA(config.hostname);

  // begin Elegant OTA
  ElegantOTA.begin(&server);
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);

  LOG.println("ElegantOTA started");

  if (!LittleFS.begin()) {
    LOG.println("An Error has occurred while mounting LittleFS");
  } else {
    LOG.println("LittleFS started");
  }

  // set device details
  String uniqueId = String(config.hostname);
  std::replace(uniqueId.begin(), uniqueId.end(), '-', '_');

  for (unsigned short i = 0; i < uniqueId.length(); i++) {
    deviceId[i] = (byte)uniqueId[i];
  }

  strcpy(deviceName, uniqueId.c_str());

  device.setUniqueId(deviceId, uniqueId.length());
  device.setName(deviceName);
  device.setSoftwareVersion("1.0.0");
  device.setManufacturer("Shell M. Shrader");
  device.setModel("BME280");

  // configure sensors
  strcpy(tempSensorName, (uniqueId + "_temperature_sensor").c_str());
  strcpy(humidSensorName, (uniqueId + "_humdity_sensor").c_str());
  strcpy(presSensorName, (uniqueId + "_pressure_sensor").c_str());
  strcpy(altSensorName, (uniqueId + "_altitude_sensor").c_str());
  strcpy(rssiSensorName, (uniqueId + "_rssi_sensor").c_str());

  tempSensor = new HASensorNumber(tempSensorName, HASensorNumber::PrecisionP3);
  humidSensor = new HASensorNumber(humidSensorName, HASensorNumber::PrecisionP3);
  presSensor = new HASensorNumber(presSensorName, HASensorNumber::PrecisionP3);
  altSensor = new HASensorNumber(altSensorName, HASensorNumber::PrecisionP3);
  rssiSensor = new HASensorNumber(rssiSensorName, HASensorNumber::PrecisionP0);

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

  LOG.println("Rebuilding /setup.html");
  updateHtmlTemplate("/setup.template.html");

  // wire up http server and paths
  wireWebServerAndPaths();

  // fire up mqtt client if in station mode
  if (wifimode == WIFI_STA) {
    mqtt.begin(config.mqtt_server, config.mqtt_user, config.mqtt_pwd);
    LOG.println("MQTT started");
  }

  // setup done
  LOG.println("\nSystem Ready\n");
}

void loop() {
  // captive portal if in AP mode
  if (wifimode == WIFI_AP) {
    dnsServer.processNextRequest();
  } else {
    // handle MQTT
    mqtt.loop();
  }

  // check for OTA
  ArduinoOTA.handle();
  ElegantOTA.loop();

  // handle TelnetSpy
  SerialAndTelnet.handle();

  // rebuild setup.html on main thread
  if (setup_needs_update) {
    LOG.println("----- rebuilding /setup.html");
    updateHtmlTemplate("/setup.template.html");
    LOG.println("-----  /setup.html rebuilt");
    setup_needs_update = false;
  }

  const unsigned long sysmillis = millis();

  // collect a sample every (publish_interval / samples_per_publish) seconds
  if (sysmillis - samples.last_update >= config.publish_interval / config.samples_per_publish || samples.last_update == ULONG_MAX) {
    sensors_event_t temp_event, pressure_event, humidity_event;
    samples.sample_count++;

    bme_temp->getEvent(&temp_event);
    long currentTemp = round(temp_event.temperature * 1000);

    bme_pressure->getEvent(&pressure_event);
    long currentPres = round(pressure_event.pressure * 1000);
    long currentAlt = round(bme.readAltitude(SEALEVELPRESSURE_HPA) * 1000);

    bme_humidity->getEvent(&humidity_event);
    long currentHumid = round(humidity_event.relative_humidity * 1000);

    long currentRssi = abs(WiFi.RSSI());
    
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

    LOG.println("Gathered Sample #" + String(samples.sample_count));

    LOG.print(F("Temperature = "));
    LOG.print(currentTemp / 1000.0, 3);
    LOG.println(" *C");

    LOG.print(F("Humidity    = "));
    LOG.print(currentHumid / 1000.0, 3);
    LOG.println(" %");

    LOG.print(F("Altitude    = "));
    LOG.print(currentAlt / 1000.0, 3);
    LOG.println(" m");

    LOG.print(F("Pressure    = "));
    LOG.print(currentPres / 1000.0, 3);
    LOG.println(" hPa");

    LOG.print(F("rssi        = "));
    LOG.print(currentRssi * -1);
    LOG.println(" dB\n");

    if (samples.sample_count >= config.samples_per_publish) {
      // publish our normalized values 
        LOG.println("\nNormalized Result (Published)");

        // remove highest and lowest values (outliers)
        samples.temperature = samples.temperature - (samples.low_temperature + samples.high_temperature);
        samples.humidity = samples.humidity - (samples.low_humidity + samples.high_humidity);
        samples.altitude = samples.altitude - (samples.low_altitude + samples.high_altitude);
        samples.pressure = samples.pressure - (samples.low_pressure + samples.high_pressure);
        samples.rssi = samples.rssi - (samples.low_rssi + samples.high_rssi);

        // account for removed outliers
        samples.sample_count-=2;

        // use the average of what remains
        float finalTemp = samples.temperature / samples.sample_count / 1000.0 * 1.8 + 32;
        float finalHumid = samples.humidity / samples.sample_count / 1000.0;
        float finalAlt = samples.altitude / samples.sample_count / 1000.0;
        float finalPres = samples.pressure / samples.sample_count / 1000.0 * HPA_TO_INHG;
        short finalRssi = samples.rssi / samples.sample_count * -1;

        LOG.print(F("Temperature = "));
        LOG.print(finalTemp, 3);
        LOG.print(" *F (");
        LOG.print(samples.temperature / samples.sample_count / 1000.0, 3);
        LOG.println(" *C)");

        LOG.print(F("Humidity    = "));
        LOG.print(finalHumid, 3);
        LOG.println(" %");

        LOG.print(F("Altitude    = "));
        LOG.print(finalAlt, 3);
        LOG.println(" m");

        LOG.print(F("Pressure    = "));
        LOG.print(finalPres, 3);
        LOG.print(" inHg (");
        LOG.print(samples.pressure / samples.sample_count / 1000.0, 3);
        LOG.println(" hPa)");

        LOG.print(F("rssi        = "));
        LOG.print(finalRssi);
        LOG.println(" dB\n");

        if (isSampleValid(finalTemp)) tempSensor->setValue(finalTemp);
        if (isSampleValid(finalHumid)) humidSensor->setValue(finalHumid);
        if (isSampleValid(finalAlt)) altSensor->setValue(finalAlt);
        if (isSampleValid(finalPres)) presSensor->setValue(finalPres);
        if (isSampleValid(finalRssi)) rssiSensor->setValue(finalRssi);

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

        blink();
    }
    samples.last_update = sysmillis;
  }

  // it doesn't seem right to never sleep so we'll delay for 1ms
  delay(1);

  // reboot if in AP mode and no activity for 5 minutes
  if (wifimode == WIFI_AP && !ap_mode_activity && millis() >= 300000UL) {
    ota_needs_reboot = true;
  }

  if (ota_needs_reboot) {
    ElegantOTA.loop();
    delay(1000);
    LOG.println("\nOTA Reboot Triggered. . .\n");
    ESP.restart();
    while (1) {
    } // will never get here
  }
}
