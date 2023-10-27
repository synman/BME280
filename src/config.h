/***************************************************************************
Copyright © 2023 Shell M. Shrader <shell at shellware dot com>
----------------------------------------------------------------------------
This work is free. You can redistribute it and/or modify it under the
terms of the Do What The Fuck You Want To Public License, Version 2,
as published by Sam Hocevar. See the COPYING file for more details.
****************************************************************************/
#include <TelnetSpy.h>
TelnetSpy SerialAndTelnet;

#ifdef BME280_DEBUG
    #define LOG_BEGIN(baudrate)  SerialAndTelnet.begin(baudrate)
    #define LOG_PRINT(...)       SerialAndTelnet.print(__VA_ARGS__)
    #define LOG_PRINTLN(...)     SerialAndTelnet.println(__VA_ARGS__)
    #define LOG_PRINTF(...)      SerialAndTelnet.printf(__VA_ARGS__)
    #define LOG_HANDLE()         SerialAndTelnet.handle() ; checkForRemoteCommand()
    #define LOG_FLUSH()          SerialAndTelnet.flush()
    #define LOG_WELCOME_MSG(msg) SerialAndTelnet.setWelcomeMsg(msg)
#else
    #define LOG_BEGIN(baudrate)
    #define LOG_PRINT(...) 
    #define LOG_PRINTLN(...)
    #define LOG_PRINTF(...) 
    #define LOG_HANDLE()
    #define LOG_FLUSH()
    #define LOG_WELCOME_MSG(msg)
#endif

#define WATCHDOG_TIMEOUT_S 15
volatile bool timer_pinged;

#include <Arduino.h>
#include <ArduinoHA.h>
#include <ArduinoOTA.h>
#include "time.h"

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BME280.h>

#ifdef esp32
    #include <WiFi.h>
    #include <AsyncTCP.h>
    #include <WiFiClientSecure.h>

    #define WIFI_DISCONNECTED WIFI_EVENT_STA_DISCONNECTED

    hw_timer_t * watchDogTimer = NULL;

    void IRAM_ATTR watchDogInterrupt() {
        LOG_PRINTLN("watchdog triggered reboot");
        LOG_FLUSH();
        ESP.restart();
    }
#else
    #define USING_TIM_DIV256 true
    #define WIFI_DISCONNECTED WIFI_EVENT_STAMODE_DISCONNECTED
    #define FILE_READ "r"
    #define FILE_WRITE "w"

    #include <ESP8266Wifi.h>
    // #include <ESP8266WiFiGeneric.h>
    #include <ESPAsyncTCP.h>
    #include <ESP8266HTTPClient.h>
    #include <WiFiClientSecureBearSSL.h>
    #include "ESP8266TimerInterrupt.h"

    ESP8266Timer ITimer;

    void IRAM_ATTR TimerHandler() {
        if (timer_pinged) {
            LOG_PRINTLN("watchdog triggered reboot");
            LOG_FLUSH();
            ESP.restart();
        } else {
            timer_pinged = true;
            LOG_PRINTLN("\nPING");
        }
    }
#endif

#include <ArduinoJson.h>

#include <DNSServer.h>
#include <EEPROM.h>
#include "LittleFS.h"

#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>

// LED is connected to GPIO2 on these boards
#ifdef esp32
    #define INIT_LED { pinMode(2, OUTPUT); digitalWrite(2, LOW); }
    #define LED_ON   { digitalWrite(2, HIGH); }
    #define LED_OFF  { digitalWrite(2, LOW); }
#else
    #define INIT_LED { pinMode(2, OUTPUT); digitalWrite(2, HIGH); }
    #define LED_ON   { digitalWrite(2, LOW); }
    #define LED_OFF  { digitalWrite(2, HIGH); }
#endif

#define EEPROM_SIZE 256
#define HOSTNAME_LEN 32
#define WIFI_SSID_LEN 32
#define WIFI_PASSWD_LEN 64
#define MQTT_SERVER_LEN 16
#define MQTT_USER_LEN 16
#define MQTT_PASSWD_LEN 32
#define NWS_STATION_LEN 6

#define DEFAULT_HOSTNAME            "bme280-env-sensor"

#define DEFAULT_SAMPLES_PER_PUBLISH 3
#define DEFAULT_PUBLISH_INTERVAL    60000

#define MIN_SAMPLES_PER_PUBLISH     3
#define MIN_PUBLISH_INTERVAL        1000

#define CFG_NOT_SET                 0x0
#define CFG_SET                     0x9

typedef unsigned char tiny_int;

typedef struct config_type {
    tiny_int hostname_flag;
    char hostname[HOSTNAME_LEN];
    tiny_int ssid_flag;
    char ssid[WIFI_SSID_LEN];
    tiny_int ssid_pwd_flag;
    char ssid_pwd[WIFI_PASSWD_LEN];
    tiny_int mqtt_server_flag;
    char mqtt_server[MQTT_SERVER_LEN];
    tiny_int mqtt_user_flag;
    char mqtt_user[MQTT_USER_LEN];
    tiny_int mqtt_pwd_flag;
    char mqtt_pwd[MQTT_PASSWD_LEN];
    tiny_int samples_per_publish_flag;
    tiny_int samples_per_publish;
    tiny_int publish_interval_flag;
    unsigned long publish_interval;
    tiny_int nws_station_flag;
    char nws_station[NWS_STATION_LEN];
} CONFIG_TYPE;

typedef struct samples_type {
    long temperature = 0;
    long humidity = 0;
    long altitude = 0;
    long pressure = 0;
    long rssi = 0;
    long high_temperature = LONG_MIN;
    long high_humidity = LONG_MIN;
    long high_altitude = LONG_MIN;
    long high_pressure = LONG_MIN;
    long high_rssi = LONG_MIN;
    long low_temperature = LONG_MAX;
    long low_humidity = LONG_MAX;
    long low_altitude = LONG_MAX;
    long low_pressure = LONG_MAX;
    long low_rssi = LONG_MAX;
    short sample_count = 0;
    unsigned long last_update = ULONG_MAX;
    unsigned long last_pressure_calibration = ULONG_MAX;
} SAMPLES_TYPE;

CONFIG_TYPE config;
SAMPLES_TYPE samples;

void watchDogRefresh();

void saveConfig(String hostname, 
                String ssid, 
                String ssid_pwd, 
                String mqtt_server, 
                String mqtt_user, 
                String mqtt_pwd, 
                tiny_int samples_per_publish, 
                unsigned long publish_interval, 
                String nws_station);

void wipeConfig();
float getSeaLevelPressure();
boolean isNumeric(String str);
void printHeapStats();

Adafruit_BME280 bme; // use I2C interface
Adafruit_Sensor* bme_temp = bme.getTemperatureSensor();
Adafruit_Sensor* bme_pressure = bme.getPressureSensor();
Adafruit_Sensor* bme_humidity = bme.getHumiditySensor();

float HPA_TO_INHG = 0.02952998057228486;
float DEFAULT_SEALEVELPRESSURE_HPA = 1013.25;
float INVALID_SEALEVELPRESSURE_HPA = SHRT_MIN;
float SEALEVELPRESSURE_HPA = DEFAULT_SEALEVELPRESSURE_HPA;

WiFiMode_t wifimode = WIFI_AP;

bool esp_reboot_requested = false;
unsigned long ota_progress_millis = 0;

bool setup_needs_update = false;
bool ap_mode_activity = false;

// Set web server port number to 80
AsyncWebServer server(80);

DNSServer dnsServer;
const byte DNS_PORT = 53;

WiFiClient wifiClient;
int wifiState = WIFI_EVENT_MAX;

HADevice device;
HAMqtt mqtt(wifiClient, device, 10);

byte deviceId[40];
char deviceName[40];

char tempSensorName[80];
char humidSensorName[80];
char presSensorName[80];
char altSensorName[80];
char rssiSensorName[80];
char seaLevelPresSensorName[80];
char ipAddressSensorName[80];

HASensorNumber* tempSensor;
HASensorNumber* humidSensor;
HASensorNumber* presSensor;
HASensorNumber* altSensor;
HASensorNumber* rssiSensor;
HASensorNumber* seaLevelPresSensor;
HASensor* ipAddressSensor;
