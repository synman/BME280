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

#include "config.h"

// LED is connected to GPIO2 on this board
#define INIT_LED { pinMode(2, OUTPUT); digitalWrite(2, LOW); }
#define LED_ON   { digitalWrite(2, HIGH); }
#define LED_OFF  { digitalWrite(2, LOW); }

void blink() {
    LED_ON;
    delay(200);
    LED_OFF;
    delay(100);
    LED_ON;
    delay(200);
    LED_OFF;
}

Adafruit_BME280 bme; // use I2C interface
Adafruit_Sensor* bme_temp = bme.getTemperatureSensor();
Adafruit_Sensor* bme_pressure = bme.getPressureSensor();
Adafruit_Sensor* bme_humidity = bme.getHumiditySensor();

#define SEALEVELPRESSURE_HPA (1013.25)
#define HPA_TO_INHG (0.02952998057228486)

String hostname = "bme280-env-sensor";
String ssid = "";
String ssid_pwd = "";
String mqtt_server = "";
String mqtt_user = "";
String mqtt_pwd = "";

String ap_name = hostname;
WiFiMode_t wifimode = WIFI_AP;
char buf[255];

bool ota_needs_reboot = false;
unsigned long ota_progress_millis = 0;
struct tm timeinfo;

// Set web server port number to 80
AsyncWebServer server(80);

void wireConfig() {
    // configuration storage
    EEPROM.begin(EEPROM_SIZE);
    uint8_t* p = (uint8_t*)(&config);
    for (unsigned int i = 0; i < sizeof(config); i++) {
        *(p + i) = EEPROM.read(i);
    }
    EEPROM.commit();
    EEPROM.end();

    if (config.hostname_flag == 9) {
        sprintf(buf, "config host: %s", config.hostname);
        hostname = config.hostname;
    } else {
        strcpy(config.hostname, hostname.c_str());
    }

    if (config.ssid_flag == 9) {
        sprintf(buf, "config ssid: %s", config.ssid);
        ssid = config.ssid;
        wifimode = WIFI_STA;
    } else {
        strcpy(config.ssid, ssid.c_str());
    }

    if (config.ssid_pwd_flag == 9) {
        sprintf(buf, "config ssid pwd: %s", config.ssid_pwd);
        ssid_pwd = config.ssid_pwd;
    } else {
        strcpy(config.ssid_pwd, ssid_pwd.c_str());
    }

    if (config.mqtt_server_flag == 9) {
        sprintf(buf, "config mqtt server: %s", config.mqtt_server);
        mqtt_server = config.mqtt_server;
    } else {
        strcpy(config.mqtt_server, mqtt_server.c_str());
    }

    if (config.mqtt_user_flag == 9) {
        sprintf(buf, "config mqtt user: %s", config.mqtt_user);
        mqtt_user = config.mqtt_user;
    } else {
        strcpy(config.mqtt_user, mqtt_user.c_str());
    }

    if (config.mqtt_pwd_flag == 9) {
        sprintf(buf, "config mqtt pwd: %s", config.mqtt_pwd);
        mqtt_pwd = config.mqtt_pwd;
    } else {
        strcpy(config.mqtt_pwd, mqtt_pwd.c_str());
    }

    Serial.println("       config host: [" + String(config.hostname) + "] stored: " + (config.hostname_flag == 9 ? "true" : "false"));
    Serial.println("       config ssid: [" + String(config.ssid) + "] stored: " + (config.ssid_flag == 9 ? "true" : "false"));
    Serial.println("   config ssid pwd: [" + String(config.ssid_pwd) + "] stored: " + (config.ssid_pwd_flag == 9 ? "true\n" : "false\n"));
    Serial.println("config mqtt server: [" + String(config.mqtt_server) + "] stored: " + (config.mqtt_server_flag == 9 ? "true" : "false"));
    Serial.println("  config mqtt user: [" + String(config.mqtt_user) + "] stored: " + (config.mqtt_user_flag == 9 ? "true" : "false"));
    Serial.println("   config mqtt pwd: [" + String(config.mqtt_pwd) + "] stored: " + (config.mqtt_pwd_flag == 9 ? "true\n" : "false\n"));
}

void onOTAStart() {
    // Log when OTA has started
    Serial.println("OTA update started!");
    // <Add your own code here>
}

void onOTAProgress(size_t current, size_t final) {
    // Log every 1 second
    if (millis() - ota_progress_millis > 1000) {
        ota_progress_millis = millis();
        Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
    }
}

void onOTAEnd(bool success) {
    // Log when OTA has finished
    if (success) {
        Serial.println("OTA update finished successfully!");
        ota_needs_reboot = true;
    } else {
        Serial.println("There was an error during OTA update!");
    }
}

String template_processor(const String& var) {
    char result[255];

    if (var == "hostname")
        return config.hostname;
    if (var == "ssid")
        return config.ssid;
    if (var == "ssid_pwd")
        return config.ssid_pwd;
    if (var == "mqtt_server")
        return config.mqtt_server;
    if (var == "mqtt_user")
        return config.mqtt_user;
    if (var == "mqtt_pwd")
        return config.mqtt_pwd;

    if (var == "timestamp") {
        if (getLocalTime(&timeinfo)) {
            sprintf(result, "\n%4d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        } else {
            return "Failed to obtain time";
        }
    }

    return result;
}

void updateIndexTemplate(String temperature, String humidity, String altitude, String pressure, String timestamp) {
    // create new index based on active config
    File _template = LittleFS.open("/index.template.html", "r");

    if (_template) {
        String html = _template.readString();
        _template.close();

        while (html.indexOf("{temperature}", 0) != -1) {
            html.replace("{temperature}", temperature);
        }

        while (html.indexOf("{humidity}", 0) != -1) {
            html.replace("{humidity}", humidity);
        }

        while (html.indexOf("{altitude}", 0) != -1) {
            html.replace("{altitude}", altitude);
        }

        while (html.indexOf("{pressure}", 0) != -1) {
            html.replace("{pressure}", pressure);
        }

        while (html.indexOf("{timestamp}", 0) != -1) {
            html.replace("{timestamp}", timestamp);
        }

        if (LittleFS.exists("/index.html.new"))
            LittleFS.remove("/index.html.new");

        File _index = LittleFS.open("/index.html.new", "w");
        _index.print(html.c_str());
        _index.close();

        if (LittleFS.exists("/index.html"))
            LittleFS.remove("/index.html");

        LittleFS.rename("/index.html.new", "/index.html");
    }
}

void wireArduinoOTA(const char* hostname) {
    ArduinoOTA.setHostname(hostname);
    ArduinoOTA
        .onStart([]()
            {
                String type;
                if (ArduinoOTA.getCommand() == U_FLASH)
                    type = "sketch";
                else // U_SPIFFS
                    type = "filesystem";

                // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
                Serial.println("OTA triggered for updating " + type);
            })
        .onEnd([]()
            {
                Serial.println("\nEnd");
            })
                .onProgress([](unsigned int progress, unsigned int total)
                    {
                        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
                    })
                .onError([](ota_error_t error)
                    {
                        Serial.printf("Error[%u]: ", error);
                        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
                        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
                        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
                        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
                        else if (error == OTA_END_ERROR) Serial.println("End Failed");
                    });
                    ArduinoOTA.begin();
                    Serial.println("ArduinoOTA started");
}

void wireWebServerAndPaths() {
    // define default document
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request)
        {
            request->redirect("/index.html");
            Serial.println(request->url() + " handled");
        });

    // define setup document
    server.on("/setup", HTTP_GET, [](AsyncWebServerRequest* request)
        {
            request->send(LittleFS, "/setup.template.html", String(), false, template_processor);
            Serial.println(request->url() + " handled");
        });

    // captive portal
    server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest* request)
        {
            request->send(LittleFS, "/index.html", "text/html");
            Serial.println(request->url() + " handled");
        });
    server.on("/library/test/success.html", HTTP_GET, [](AsyncWebServerRequest* request)
        {
            request->send(LittleFS, "/index.html", "text/html");
            Serial.println(request->url() + " handled");
        });
    server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest* request)
        {
            request->send(LittleFS, "/index.html", "text/html");
            Serial.println(request->url() + " handled");
        });
    server.on("/gen_204", HTTP_GET, [](AsyncWebServerRequest* request)
        {
            request->send(LittleFS, "/index.html", "text/html");
            Serial.println(request->url() + " handled");
        });
    server.on("/ncsi.txt", HTTP_GET, [](AsyncWebServerRequest* request)
        {
            request->send(LittleFS, "/index.html", "text/html");
            Serial.println(request->url() + " handled");
        });
    server.on("/check_network_status.txt", HTTP_GET, [](AsyncWebServerRequest* request)
        {
            request->send(LittleFS, "/index.html", "text/html");
            Serial.println(request->url() + " handled");
        });

    // request reboot
    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest* request)
        {
            request->redirect("/setup");
            Serial.println(request->url() + " handled");
            ota_needs_reboot = true;
        });

    // save config
    server.on("/save", HTTP_GET, [](AsyncWebServerRequest* request)
        {
            if (request->hasParam("ssid")) {
                ssid = request->getParam("ssid")->value();
                memset(config.ssid, '\0', WIFI_SSID_LEN);
                ssid.toCharArray(config.ssid, WIFI_SSID_LEN);
                config.ssid_flag = 9;
            }
            if (request->hasParam("hostname")) {
                hostname = request->getParam("hostname")->value();
                memset(config.hostname, '\0', HOSTNAME_LEN);
                hostname.toCharArray(config.hostname, HOSTNAME_LEN);
                config.hostname_flag = 9;
            }
            if (request->hasParam("ssid_pwd")) {
                ssid_pwd = request->getParam("ssid_pwd")->value();
                memset(config.ssid_pwd, '\0', WIFI_PASSWD_LEN);
                ssid_pwd.toCharArray(config.ssid_pwd, WIFI_PASSWD_LEN);
                config.ssid_pwd_flag = 9;
            }
            if (request->hasParam("mqtt_server")) {
                mqtt_server = request->getParam("mqtt_server")->value();
                memset(config.mqtt_server, '\0', MQTT_SERVER_LEN);
                mqtt_server.toCharArray(config.mqtt_server, MQTT_SERVER_LEN);
                config.mqtt_server_flag = 9;
            }
            if (request->hasParam("mqtt_user")) {
                mqtt_user = request->getParam("mqtt_user")->value();
                memset(config.mqtt_user, '\0', MQTT_USER_LEN);
                mqtt_user.toCharArray(config.mqtt_user, MQTT_USER_LEN);
                config.mqtt_user_flag = 9;
            }
            if (request->hasParam("mqtt_pwd")) {
                mqtt_pwd = request->getParam("mqtt_pwd")->value();
                memset(config.mqtt_pwd, '\0', MQTT_PASSWD_LEN);
                mqtt_pwd.toCharArray(config.mqtt_pwd, MQTT_PASSWD_LEN);
                config.mqtt_pwd_flag = 9;
            }
            if (config.hostname_flag == 9 || config.ssid_flag == 9 || config.ssid_pwd_flag == 9 ||
                config.mqtt_server_flag == 9 || config.mqtt_user_flag == 9 || config.mqtt_pwd_flag == 9) {
                EEPROM.begin(EEPROM_SIZE);
                uint8_t* p = (uint8_t*)(&config);
                for (unsigned char i = 0; i < sizeof(config); i++) {
                    EEPROM.write(i, *(p + i));
                }
                EEPROM.commit();
                EEPROM.end();
            }
            request->redirect("/setup");
            Serial.println(request->url() + " handled");
        });

    // wipe config
    server.on("/wipe", HTTP_GET, [](AsyncWebServerRequest* request)
        {
            config.ssid_flag = 0;
            memset(config.ssid, '\0', WIFI_SSID_LEN);
            config.hostname_flag = 0;
            memset(config.hostname, '\0', HOSTNAME_LEN);
            config.ssid_pwd_flag = 0;
            memset(config.ssid_pwd, '\0', WIFI_PASSWD_LEN);
            config.mqtt_server_flag = 0;
            memset(config.mqtt_server, '\0', MQTT_SERVER_LEN);
            config.mqtt_user_flag = 0;
            memset(config.mqtt_user, '\0', MQTT_USER_LEN);
            config.mqtt_pwd_flag = 0;
            memset(config.mqtt_pwd, '\0', MQTT_PASSWD_LEN);

            EEPROM.begin(EEPROM_SIZE);
            uint8_t* p = (uint8_t*)(&config);
            for (unsigned char i = 0; i < sizeof(config); i++) {
                EEPROM.write(i, *(p + i));
            }
            EEPROM.commit();
            EEPROM.end();

            request->send(200, "text/plain", "Settings Wiped!");
            Serial.println(request->url() + " handled");
        });

    // 404 (includes file handling)
    server.onNotFound([](AsyncWebServerRequest* request)
        {
            if (LittleFS.exists(request->url())) {
                AsyncWebServerResponse* response = request->beginResponse(LittleFS, request->url(), String());
                String url = request->url(); url.toLowerCase();
                // only chache digital assets
                if (url.indexOf(".png") != -1 || url.indexOf(".jpg") != -1 || url.indexOf(".ico") != -1 || url.indexOf(".svg") != -1) {
                    response->addHeader("Cache-Control", "max-age=604800");
                } else {
                    response->addHeader("Cache-Control", "no-store");
                }
                request->send(response);
                Serial.println(request->url() + " handled");
            } else {
                request->send(404, "text/plain", request->url() + " Not found!");
                Serial.println(request->url() + " Not found!");
            }
        });
}