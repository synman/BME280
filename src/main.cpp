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

HASensorNumber* tempSensor;
HASensorNumber* humidSensor;
HASensorNumber* presSensor;
HASensorNumber* altSensor;

unsigned long lastSensorUpdate = -60001;

void setup() {
  INIT_LED;

  Serial.begin(115200);
  Serial.println("");
  Serial.println("");
  Serial.println("BME280 Sensor Event Publisher v1.0.0");

  while (!bme.begin()) {
    Serial.println(F("Could not find a valid BME280 sensor, check wiring!"));
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

  if (wifimode == WIFI_STA) {
    Serial.print("Connecting to ");
    Serial.print(ssid);

    WiFi.begin(ssid.c_str(), ssid_pwd.c_str());
    for (unsigned char x = 0; x < 60 && WiFi.status() != WL_CONNECTED; x++) {
      blink();
      Serial.print(".");
    }

    Serial.println("");

    if (WiFi.status() == WL_CONNECTED) {
      // initialize time
      configTime(0, 0, "pool.ntp.org");
      setenv("TZ", "EST+5EDT,M3.2.0/2,M11.1.0/2", 1);
      tzset();

      if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
      }
      sprintf(buf, "%4d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
      Serial.print("\nCurrent Time: ");
      Serial.println(buf);
    }
  }

  if (WiFi.status() != WL_CONNECTED || wifimode == WIFI_AP) {
    wifimode = WIFI_AP;
    WiFi.mode(wifimode);
    WiFi.softAP(ap_name);
    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
    Serial.println("\nSoftAP [" + ap_name + "] started\n");
  }

  Serial.print("    Hostname: ");
  Serial.println(hostname);
  Serial.print("Connected to: ");
  Serial.println(wifimode == WIFI_STA ? ssid : ap_name);
  Serial.print("  IP address: ");
  Serial.println(wifimode == WIFI_STA ? WiFi.localIP().toString() : WiFi.softAPIP().toString());
  Serial.print("        RSSI: ");
  Serial.println(String(WiFi.RSSI()) + "\n");

  // enable mDNS via espota and enable ota
  wireArduinoOTA(hostname.c_str());

  // wire up http server and paths
  wireWebServerAndPaths();

  // begin Elegant OTA
  ElegantOTA.begin(&server);
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);

  Serial.println("ElegantOTA started");

  if (!LittleFS.begin()) {
    Serial.println("An Error has occurred while mounting LittleFS");
  } else {
    Serial.println("LittleFS filesystem started");
  }

  // begin the web server
  server.begin();
  Serial.println("HTTP server started");

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

  tempSensor = new HASensorNumber(tempSensorName, HASensorNumber::PrecisionP2);
  humidSensor = new HASensorNumber(humidSensorName, HASensorNumber::PrecisionP2);
  presSensor = new HASensorNumber(presSensorName, HASensorNumber::PrecisionP3);
  altSensor = new HASensorNumber(altSensorName, HASensorNumber::PrecisionP1);

  tempSensor->setDeviceClass("temperature");
  tempSensor->setName("Temperature");
  tempSensor->setUnitOfMeasurement("F");

  presSensor->setDeviceClass("atmospheric_pressure");
  presSensor->setName("Barometer");
  presSensor->setUnitOfMeasurement("inHg");

  altSensor->setIcon("mdi:waves-arrow-up");
  altSensor->setName("Altitude");
  altSensor->setUnitOfMeasurement("M");

  humidSensor->setDeviceClass("humidity");
  humidSensor->setName(" Humidity");
  humidSensor->setUnitOfMeasurement("%");

  // fire up mqtt client if in station mode
  if (wifimode == WIFI_STA) {
    mqtt.begin(mqtt_server.c_str(), mqtt_user.c_str(), mqtt_pwd.c_str());
    Serial.println("MQTT started");
  }

  // setup done
  Serial.println("\nSystem Ready\n");
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

  if ((millis() - lastSensorUpdate) > 59999) {
    sensors_event_t temp_event, pressure_event, humidity_event;
    String timestamp;

    if (!getLocalTime(&timeinfo)) {
      timestamp = "Failed to obtain time";
    } else {
      sprintf(buf, "%4d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
      timestamp = String(buf);
    }

    bme_temp->getEvent(&temp_event);
    float currentTemp = 1.8f * temp_event.temperature + 32;

    bme_pressure->getEvent(&pressure_event);
    float currentPres = pressure_event.pressure * HPA_TO_INHG;
    float currentAlt = bme.readAltitude(SEALEVELPRESSURE_HPA);

    bme_humidity->getEvent(&humidity_event);
    float currentHumid = humidity_event.relative_humidity;

    Serial.print(F("\nTimestamp   = "));
    Serial.println(timestamp);

    Serial.print(F("Temperature = "));
    Serial.print(currentTemp);
    Serial.println(" *F");

    Serial.print(F("Humidity    = "));
    Serial.print(currentHumid);
    Serial.println(" %");

    Serial.print(F("Pressure    = "));
    Serial.print(currentPres);
    Serial.println(" inHg");

    Serial.print(F("Altitude    = "));
    Serial.print(currentAlt);
    Serial.println(" m\n");

    tempSensor->setValue(currentTemp);
    presSensor->setValue(currentPres);
    altSensor->setValue(currentAlt);
    humidSensor->setValue(currentHumid);

    updateIndexTemplate(String(currentTemp), String(currentHumid), String(currentAlt), String(currentPres), timestamp);

    lastSensorUpdate = millis();
    blink();
  }

  // it doesn't seem right to never sleep so we'll delay for 1ms
  delay(1);

  if (ota_needs_reboot) {
    ElegantOTA.loop();
    delay(1000);
    Serial.println("\nOTA Reboot Triggered. . .\n");
    ESP.restart();
    while (1) {
    } // will never get here
  }
}

/*********
  Rui Santos
  Complete project details at https://randomnerdtutorials.com
*********/

// #include <Wire.h>
// #include "Arduino.h"

// void setup() {
//   Wire.begin();
//   Serial.begin(115200);
//   Serial.println("\nI2C Scanner");
// }

// void loop() {
//   byte error, address;
//   int nDevices;
//   Serial.println("Scanning...");
//   nDevices = 0;
//   for(address = 1; address < 127; address++ ) {
//     Wire.beginTransmission(address);
//     error = Wire.endTransmission();
//     if (error == 0) {
//       Serial.print("I2C device found at address 0x");
//       if (address<16) {
//         Serial.print("0");
//       }
//       Serial.println(address,HEX);
//       nDevices++;
//     }
//     else if (error==4) {
//       Serial.print("Unknow error at address 0x");
//       if (address<16) {
//         Serial.print("0");
//       }
//       Serial.println(address,HEX);
//     }
//   }
//   if (nDevices == 0) {
//     Serial.println("No I2C devices found\n");
//   }
//   else {
//     Serial.println("done\n");
//   }
//   delay(5000);
// }