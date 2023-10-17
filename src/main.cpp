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

DNSServer dnsServer;
const byte DNS_PORT = 53;

WiFiClient client;
HADevice device;
HAMqtt mqtt(client, device);

byte deviceId[40];
char deviceName[40];

char tempSensorName[80];
char humidSensorName[80];
char presSensorName[80];
char altSensorName[80];
char rssiSensorName[80];

HASensorNumber* tempSensor;
HASensorNumber* humidSensor;
HASensorNumber* presSensor;
HASensorNumber* altSensor;
HASensorNumber* rssiSensor;

unsigned long lastSensorUpdate = -60001;

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
  WiFi.hostname(hostname);
  WiFi.mode(wifimode);

  // WiFi.scanNetworks will return the number of networks found
  uint8_t *bestBssid;
  short bestRssi = std::numeric_limits<short>::min();
  int n = WiFi.scanNetworks();

  // arduino is too stupid to know which AP has the best signal
  // when connecting to an SSID with multiple BSSIDs (WAPs / Repeaters)
  // so we find the best one and tell it to use it
  if (n > 0 && ssid.length() > 0) {
    for (int i = 0; i < n; ++i) {
      if (WiFi.SSID(i).equals(ssid.c_str()) && WiFi.RSSI(i) > bestRssi) {
        bestRssi = WiFi.RSSI(i);
        bestBssid = WiFi.BSSID(i);
      }
    }
  }

  if (wifimode == WIFI_STA && bestRssi != std::numeric_limits<short>::min()) {
    LOG.print("Connecting to "); LOG.print(ssid);
    WiFi.begin(ssid.c_str(), ssid_pwd.c_str(), 0, bestBssid, true);
    for (unsigned char x = 0; x < 60 && WiFi.status() != WL_CONNECTED; x++) {
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
    WiFi.softAP(ap_name);
    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
    LOG.println("\nSoftAP [" + ap_name + "] started\n");
  }
      
  LOG.print("    Hostname: "); LOG.println(hostname);
  LOG.print("Connected to: "); LOG.println(wifimode == WIFI_STA ? ssid : ap_name);
  LOG.print("  IP address: "); LOG.println(wifimode == WIFI_STA ? WiFi.localIP().toString() : WiFi.softAPIP().toString());
  LOG.print("        RSSI: "); LOG.println(String(WiFi.RSSI()) + "\n");

  // enable mDNS via espota and enable ota
  wireArduinoOTA(hostname.c_str());

  // begin Elegant OTA
  ElegantOTA.begin(&server);
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);

  LOG.println("ElegantOTA started");

  if (!LittleFS.begin()) {
    LOG.println("An Error has occurred while mounting LittleFS");
  } else {
    LOG.println("LittleFS filesystem started");
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

  tempSensor = new HASensorNumber(tempSensorName, HASensorNumber::PrecisionP2);
  humidSensor = new HASensorNumber(humidSensorName, HASensorNumber::PrecisionP2);
  presSensor = new HASensorNumber(presSensorName, HASensorNumber::PrecisionP3);
  altSensor = new HASensorNumber(altSensorName, HASensorNumber::PrecisionP1);
  rssiSensor = new HASensorNumber(rssiSensorName, HASensorNumber::PrecisionP2);

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

  // wire up http server and paths
  wireWebServerAndPaths();

  // fire up mqtt client if in station mode
  if (wifimode == WIFI_STA) {
    mqtt.begin(mqtt_server.c_str(), mqtt_user.c_str(), mqtt_pwd.c_str());
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

  const unsigned long sysmillis = millis();

  // collect a sample every 5 seconds
  if (sysmillis - samples.last_update > 4999) {
    sensors_event_t temp_event, pressure_event, humidity_event;
    samples.sample_count++;

    bme_temp->getEvent(&temp_event);
    float currentTemp = 1.8f * temp_event.temperature + 32;

    bme_pressure->getEvent(&pressure_event);
    float currentPres = pressure_event.pressure * HPA_TO_INHG;
    float currentAlt = bme.readAltitude(SEALEVELPRESSURE_HPA);

    bme_humidity->getEvent(&humidity_event);
    float currentHumid = humidity_event.relative_humidity;

    float currentRssi = WiFi.RSSI();
    
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
    LOG.print(currentTemp);
    LOG.println(" *F");

    LOG.print(F("Humidity    = "));
    LOG.print(currentHumid);
    LOG.println(" %");

    LOG.print(F("Altitude    = "));
    LOG.print(currentAlt);
    LOG.println(" m");

    LOG.print(F("Pressure    = "));
    LOG.print(currentPres);
    LOG.println(" inHg");

    LOG.print(F("rssi        = "));
    LOG.print(currentRssi);
    LOG.println(" dB\n");

    samples.last_update = sysmillis;
    if (samples.sample_count == 1) lastSensorUpdate = sysmillis;
  }

  if (sysmillis - lastSensorUpdate > 59999) {
    String timestamp;

    if (!getLocalTime(&timeinfo)) {
      timestamp = "Failed to obtain time";
    } else {
      sprintf(buf, "%4d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
      timestamp = String(buf);
    }

    LOG.print(F("\nTimestamp   = "));
    LOG.println(timestamp);

    // remove highest and lowest values (outliers)
    samples.temperature = samples.temperature - (samples.low_temperature + samples.high_temperature);
    samples.humidity = samples.humidity - (samples.low_humidity + samples.high_humidity);
    samples.altitude = samples.altitude - (samples.low_altitude + samples.high_altitude);
    samples.pressure = samples.pressure - (samples.low_pressure + samples.high_pressure);
    samples.rssi = samples.rssi - (samples.low_rssi + samples.high_rssi);

    // account for removed outliers
    samples.sample_count-=2;

    // use the average of what remains
    float currentTemp = samples.temperature / samples.sample_count;
    float currentHumid = samples.humidity / samples.sample_count;
    float currentAlt = samples.altitude / samples.sample_count;
    float currentPres = samples.pressure / samples.sample_count;
    float currentRssi = samples.rssi / samples.sample_count;

    LOG.print(F("Temperature = "));
    LOG.print(currentTemp);
    LOG.println(" *F");

    LOG.print(F("Humidity    = "));
    LOG.print(currentHumid);
    LOG.println(" %");

    LOG.print(F("Altitude    = "));
    LOG.print(currentAlt);
    LOG.println(" m");

    LOG.print(F("Pressure    = "));
    LOG.print(currentPres);
    LOG.println(" inHg");

    LOG.print(F("rssi        = "));
    LOG.print(currentRssi);
    LOG.println(" dB\n");

    tempSensor->setValue(currentTemp);
    humidSensor->setValue(currentHumid);
    altSensor->setValue(currentAlt);
    presSensor->setValue(currentPres);
    rssiSensor->setValue(currentRssi);

    updateIndexTemplate(String(currentTemp), String(currentHumid), String(currentAlt), String(currentPres), String(currentRssi), timestamp);

    // reset our samples structure
    samples.temperature = 0;
    samples.humidity = 0;
    samples.altitude = 0;
    samples.pressure = 0;
    samples.rssi = 0;

    samples.high_temperature = std::numeric_limits<float>::min();
    samples.high_humidity = std::numeric_limits<float>::min();
    samples.high_altitude = std::numeric_limits<float>::min();
    samples.high_pressure = std::numeric_limits<float>::min();
    samples.high_rssi = std::numeric_limits<float>::min();

    samples.low_temperature = std::numeric_limits<float>::max();
    samples.low_humidity = std::numeric_limits<float>::max();
    samples.low_altitude = std::numeric_limits<float>::max();
    samples.low_pressure = std::numeric_limits<float>::max();
    samples.low_rssi = std::numeric_limits<float>::max();

    samples.sample_count = 0;
    lastSensorUpdate = sysmillis;

    blink();
  }

  // it doesn't seem right to never sleep so we'll delay for 1ms
  delay(1);

  if (ota_needs_reboot) {
    ElegantOTA.loop();
    delay(1000);
    LOG.println("\nOTA Reboot Triggered. . .\n");
    ESP.restart();
    while (1) {
    } // will never get here
  }
}