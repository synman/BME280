#include <Arduino.h>
#include <ArduinoHA.h>
#include <ArduinoOTA.h>
#include "time.h"

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BME280.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <EEPROM.h>
#include "LittleFS.h"

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>

#include <TelnetSpy.h>

// LED is connected to GPIO2 on this board
#define INIT_LED { pinMode(2, OUTPUT); digitalWrite(2, LOW); }
#define LED_ON   { digitalWrite(2, HIGH); }
#define LED_OFF  { digitalWrite(2, LOW); }

#define LOG SerialAndTelnet

#define EEPROM_SIZE 256
#define HOSTNAME_LEN 32
#define WIFI_SSID_LEN 32
#define WIFI_PASSWD_LEN 64
#define MQTT_SERVER_LEN 16
#define MQTT_USER_LEN 16
#define MQTT_PASSWD_LEN 32

#define DEFAULT_HOSTNAME            "bme280-env-sensor"
#define DEFAULT_SAMPLES_PER_PUBLISH 3
#define DEFAULT_PUBLISH_INTERVAL    60000

#define MIN_SAMPLES_PER_PUBLISH     3
#define MIN_PUBLISH_INTERVAL        1000

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
} SAMPLES_TYPE;

CONFIG_TYPE config;
SAMPLES_TYPE samples;

TelnetSpy SerialAndTelnet;

Adafruit_BME280 bme; // use I2C interface
Adafruit_Sensor* bme_temp = bme.getTemperatureSensor();
Adafruit_Sensor* bme_pressure = bme.getPressureSensor();
Adafruit_Sensor* bme_humidity = bme.getHumiditySensor();

#define SEALEVELPRESSURE_HPA (1013.25)
#define HPA_TO_INHG          (0.02952998057228486)

WiFiMode_t wifimode = WIFI_AP;
char buf[255];

bool ota_needs_reboot = false;
unsigned long ota_progress_millis = 0;
struct tm timeinfo;

// Set web server port number to 80
AsyncWebServer server(80);

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
