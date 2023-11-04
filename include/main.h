/***************************************************************************
Copyright © 2023 Shell M. Shrader <shell at shellware dot com>
----------------------------------------------------------------------------
This work is free. You can redistribute it and/or modify it under the
terms of the Do What The Fuck You Want To Public License, Version 2,
as published by Sam Hocevar. See the COPYING file for more details.
****************************************************************************/
#include <Bootstrap.h>
#include <Adafruit_BME280.h>
#include <ArduinoHA.h>

#ifdef esp32
    #include <WiFiClientSecure.h>
    #include <ArduinoJson.h>
#else
    #include <ESP8266HTTPClient.h>
    #include <WiFiClientSecureBearSSL.h>
#endif

#if defined BME280_LOG_LEVEL_FULL and not defined BME280_LOG_LEVEL_BASIC
    #define BME280_LOG_LEVEL_BASIC
#endif

#ifdef BS_USE_TELNETSPY
    TelnetSpy SerialAndTelnet;
    Bootstrap bs = Bootstrap(PROJECT_NAME, &SerialAndTelnet, 1500000);
#else
    Bootstrap bs = Bootstrap(PROJECT_NAME);
#endif

#define MQTT_SERVER                    "mqtt_server"
#define MQTT_USER                      "mqtt_user"
#define MQTT_PWD                       "mqtt_pwd"
#define SAMPLES_PER_PUBLISH            "samples_per_publish"
#define PUBLISH_INTERVAL               "publish_interval"
#define PUBLISH_INTERVAL_IN_SECONDS    "publish_interval_in_seconds"
#define NWS_STATION                    "nws_station"

#define TEMPERATURE                    "temperature"
#define HUMIDITY                       "humidity"
#define ALTITUDE                       "altitude"
#define PRESSURE                       "pressure"
#define _RSSI                          "rssi"
#define SEA_LEVEL_ATMOSPHERIC_PRESSURE "sea_level_atmospheric_pressure"
#define IP_ADDRESS                     "ip_address"

#define MQTT_SERVER_LEN                16
#define MQTT_USER_LEN                  16
#define MQTT_PWD_LEN                   32
#define NWS_STATION_LEN                6

#define DEFAULT_SAMPLES_PER_PUBLISH    3
#define DEFAULT_PUBLISH_INTERVAL       60000

#define MIN_SAMPLES_PER_PUBLISH        3
#define MIN_PUBLISH_INTERVAL           1000

typedef struct bme280_config_type : config_type {
    tiny_int      mqtt_server_flag;
    char          mqtt_server[MQTT_SERVER_LEN];
    tiny_int      mqtt_user_flag;
    char          mqtt_user[MQTT_USER_LEN];
    tiny_int      mqtt_pwd_flag;
    char          mqtt_pwd[MQTT_PWD_LEN];
    tiny_int      samples_per_publish_flag;
    tiny_int      samples_per_publish;
    tiny_int      publish_interval_flag;
    unsigned long publish_interval;
    tiny_int      nws_station_flag;
    char          nws_station[NWS_STATION_LEN];
} BME280_CONFIG_TYPE;

typedef struct samples_type {
    long          temperature               = 0;
    long          humidity                  = 0;
    long          altitude                  = 0;
    long          pressure                  = 0;
    long          rssi                      = 0;
    long          high_temperature          = LONG_MIN;
    long          high_humidity             = LONG_MIN;
    long          high_altitude             = LONG_MIN;
    long          high_pressure             = LONG_MIN;
    long          high_rssi                 = LONG_MIN;
    long          low_temperature           = LONG_MAX;
    long          low_humidity              = LONG_MAX;
    long          low_altitude              = LONG_MAX;
    long          low_pressure              = LONG_MAX;
    long          low_rssi                  = LONG_MAX;
    short         sample_count              = 0;
    unsigned long last_update               = ULONG_MAX;
    unsigned long last_pressure_calibration = ULONG_MAX;
} SAMPLES_TYPE;

BME280_CONFIG_TYPE bme280_config;
SAMPLES_TYPE       samples;

const float  getSeaLevelPressure();
const bool   isNumeric(const String str);
const String toFloatStr(const float value, const short decimal_places);
const bool   isSampleValid(const float value);
const String escParam(const char *param_name);
void         printHeapStats();

Adafruit_BME280  bme; // use I2C interface
Adafruit_Sensor* bme_temp = bme.getTemperatureSensor();
Adafruit_Sensor* bme_pressure = bme.getPressureSensor();
Adafruit_Sensor* bme_humidity = bme.getHumiditySensor();

const float HPA_TO_INHG                  = 0.02952998057228486;
const float DEFAULT_SEALEVELPRESSURE_HPA = 1013.25;
const float INVALID_SEALEVELPRESSURE_HPA = SHRT_MIN;
float SEALEVELPRESSURE_HPA               = DEFAULT_SEALEVELPRESSURE_HPA;

HADevice device;
WiFiClient wifiClient;
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
