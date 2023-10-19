#include "config.h"

void blink() {
    LED_ON;
    delay(200);
    LED_OFF;
    delay(100);
    LED_ON;
    delay(200);
    LED_OFF;
}

void wireConfig() {
    // configuration storage
    EEPROM.begin(EEPROM_SIZE);
    uint8_t* p = (uint8_t*)(&config);
    for (unsigned int i = 0; i < sizeof(config); i++) {
        *(p + i) = EEPROM.read(i);
    }
    EEPROM.commit();
    EEPROM.end();

    if (String(config.hostname).length() < 1) {
        strcpy(config.hostname, DEFAULT_HOSTNAME);
    }

    if (config.ssid_flag == 9) {
        if (String(config.ssid).length() > 0) wifimode = WIFI_STA;
    } else {
        strcpy(config.ssid, "");
    }

    if (config.ssid_pwd_flag != 9) strcpy(config.ssid_pwd, "");
    if (config.mqtt_server_flag != 9) strcpy(config.mqtt_server, "");
    if (config.mqtt_user_flag != 9) strcpy(config.mqtt_user, "");
    if (config.mqtt_pwd_flag != 9) strcpy(config.mqtt_pwd, "");
    if (config.samples_per_publish_flag != 9) config.samples_per_publish = 3;
    if (config.publish_interval_flag != 9) config.publish_interval = 60000;
    
    LOG.println("        EEPROM size: [" + String(EEPROM_SIZE)+ "]");
    LOG.println("        config size: [" + String(sizeof(config))+ "]\n");
    LOG.println("        config host: [" + String(config.hostname) + "] stored: " + (config.hostname_flag == 9 ? "true" : "false"));
    LOG.println("        config ssid: [" + String(config.ssid) + "] stored: " + (config.ssid_flag == 9 ? "true" : "false"));
    LOG.println("    config ssid pwd: [" + String(config.ssid_pwd) + "] stored: " + (config.ssid_pwd_flag == 9 ? "true\n" : "false\n"));
    LOG.println(" config mqtt server: [" + String(config.mqtt_server) + "] stored: " + (config.mqtt_server_flag == 9 ? "true" : "false"));
    LOG.println("   config mqtt user: [" + String(config.mqtt_user) + "] stored: " + (config.mqtt_user_flag == 9 ? "true" : "false"));
    LOG.println("    config mqtt pwd: [" + String(config.mqtt_pwd) + "] stored: " + (config.mqtt_pwd_flag == 9 ? "true\n" : "false\n"));
    LOG.println(" config num samples: [" + String(config.samples_per_publish) + "] stored: " + (config.samples_per_publish_flag == 9 ? "true" : "false"));
    LOG.println("config pub interval: [" + String(config.publish_interval) + "] stored: " + (config.publish_interval_flag == 9 ? "true\n" : "false\n"));
}

void onOTAStart() {
    // Log when OTA has started
    LOG.println("OTA update started!");
    // <Add your own code here>
}

void onOTAProgress(size_t current, size_t final) {
    // Log every 1 second
    if (millis() - ota_progress_millis > 1000) {
        ota_progress_millis = millis();
        LOG.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
    }
}

void onOTAEnd(bool success) {
    // Log when OTA has finished
    if (success) {
        LOG.println("OTA update finished successfully!");
        ota_needs_reboot = true;
    } else {
        LOG.println("There was an error during OTA update!");
    }
}

String template_processor(const String& var) {
    char result[255];

    if (var == "hostname") return config.hostname;
    if (var == "ssid") return config.ssid;
    if (var == "ssid_pwd") return config.ssid_pwd;
    if (var == "mqtt_server") return config.mqtt_server;
    if (var == "mqtt_user") return config.mqtt_user;
    if (var == "mqtt_pwd") return config.mqtt_pwd;
    if (var == "samples_per_publish") return String(config.samples_per_publish);
    if (var == "publish_interval") return String(config.publish_interval);

    if (var == "timestamp") {
        if (getLocalTime(&timeinfo)) {
            sprintf(result, "\n%4d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        } else {
            return "Failed to obtain time";
        }
    }
    return result;
}

void updateIndexTemplate(String temperature, String humidity, String altitude, String pressure, String rssi, String timestamp) {
    // create new index based on active config
    File _template = LittleFS.open("/index.template.html", "r");

    if (_template) {
        String html = _template.readString();
        _template.close();

        while (html.indexOf("{publish_interval}", 0) != -1) {
            html.replace("{publish_interval}", String(config.publish_interval / 1000));
        }

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

        while (html.indexOf("{rssi}", 0) != -1) {
            html.replace("{rssi}", rssi);
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
                LOG.println("OTA triggered for updating " + type);
            })
        .onEnd([]()
            {
                LOG.println("\nEnd");
            })
                .onProgress([](unsigned int progress, unsigned int total)
                    {
                        LOG.printf("Progress: %u%%\r", (progress / (total / 100)));
                    })
                .onError([](ota_error_t error)
                    {
                        LOG.printf("Error[%u]: ", error);
                        if (error == OTA_AUTH_ERROR) LOG.println("Auth Failed");
                        else if (error == OTA_BEGIN_ERROR) LOG.println("Begin Failed");
                        else if (error == OTA_CONNECT_ERROR) LOG.println("Connect Failed");
                        else if (error == OTA_RECEIVE_ERROR) LOG.println("Receive Failed");
                        else if (error == OTA_END_ERROR) LOG.println("End Failed");
                    });
                    ArduinoOTA.begin();
                    LOG.println("ArduinoOTA started");
}

void wireWebServerAndPaths() {
    // define default document
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request)
        {
            request->redirect("/index.html");
            LOG.println(request->url() + " handled");
        });

    // define setup document
    server.on("/setup", HTTP_GET, [](AsyncWebServerRequest* request)
        {
            request->send(LittleFS, "/setup.template.html", String(), false, template_processor);
            LOG.println(request->url() + " handled");
        });

    // captive portal
    server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest* request)
        {
            request->send(LittleFS, "/index.html", "text/html");
            LOG.println(request->url() + " handled");
        });
    server.on("/library/test/success.html", HTTP_GET, [](AsyncWebServerRequest* request)
        {
            request->send(LittleFS, "/index.html", "text/html");
            LOG.println(request->url() + " handled");
        });
    server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest* request)
        {
            request->send(LittleFS, "/index.html", "text/html");
            LOG.println(request->url() + " handled");
        });
    server.on("/gen_204", HTTP_GET, [](AsyncWebServerRequest* request)
        {
            request->send(LittleFS, "/index.html", "text/html");
            LOG.println(request->url() + " handled");
        });
    server.on("/ncsi.txt", HTTP_GET, [](AsyncWebServerRequest* request)
        {
            request->send(LittleFS, "/index.html", "text/html");
            LOG.println(request->url() + " handled");
        });
    server.on("/check_network_status.txt", HTTP_GET, [](AsyncWebServerRequest* request)
        {
            request->send(LittleFS, "/index.html", "text/html");
            LOG.println(request->url() + " handled");
        });

    // request reboot
    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest* request)
        {
            request->redirect("/setup");
            LOG.println(request->url() + " handled");
            ota_needs_reboot = true;
        });

    // save config
    server.on("/save", HTTP_GET, [](AsyncWebServerRequest* request)
        {
            if (request->hasParam("hostname")) {
                String hostname = request->getParam("hostname")->value();
                if (hostname.length() < 1) hostname = DEFAULT_HOSTNAME;
                memset(config.hostname, '\0', HOSTNAME_LEN);
                hostname.toCharArray(config.hostname, HOSTNAME_LEN);
                config.hostname_flag = 9;
            }
            if (request->hasParam("ssid")) {
                const String ssid = request->getParam("ssid")->value();
                memset(config.ssid, '\0', WIFI_SSID_LEN);
                ssid.toCharArray(config.ssid, WIFI_SSID_LEN);
                config.ssid_flag = 9;
            }
            if (request->hasParam("ssid_pwd")) {
                const String ssid_pwd = request->getParam("ssid_pwd")->value();
                memset(config.ssid_pwd, '\0', WIFI_PASSWD_LEN);
                ssid_pwd.toCharArray(config.ssid_pwd, WIFI_PASSWD_LEN);
                config.ssid_pwd_flag = 9;
            }
            if (request->hasParam("mqtt_server")) {
                const String mqtt_server = request->getParam("mqtt_server")->value();
                memset(config.mqtt_server, '\0', MQTT_SERVER_LEN);
                mqtt_server.toCharArray(config.mqtt_server, MQTT_SERVER_LEN);
                config.mqtt_server_flag = 9;
            }
            if (request->hasParam("mqtt_user")) {
                const String mqtt_user = request->getParam("mqtt_user")->value();
                memset(config.mqtt_user, '\0', MQTT_USER_LEN);
                mqtt_user.toCharArray(config.mqtt_user, MQTT_USER_LEN);
                config.mqtt_user_flag = 9;
            }
            if (request->hasParam("mqtt_pwd")) {
                const String mqtt_pwd = request->getParam("mqtt_pwd")->value();
                memset(config.mqtt_pwd, '\0', MQTT_PASSWD_LEN);
                mqtt_pwd.toCharArray(config.mqtt_pwd, MQTT_PASSWD_LEN);
                config.mqtt_pwd_flag = 9;
            }

            if (request->hasParam("samples_per_publish")) {
                const int samples = request->getParam("samples_per_publish")->value().toInt();
                if (samples >= MIN_SAMPLES_PER_PUBLISH) {
                    config.samples_per_publish = samples;
                    config.samples_per_publish_flag = 9;
                }
            }

            if (request->hasParam("publish_interval")) {
                const unsigned long interval = strtoul(request->getParam("publish_interval")->value().c_str(), NULL, 10);
                if (interval >= MIN_PUBLISH_INTERVAL) {
                    config.publish_interval = interval;
                    config.publish_interval_flag = 9;
                }
            }

            EEPROM.begin(EEPROM_SIZE);
            uint8_t* p = (uint8_t*)(&config);
            for (int i = 0; i < sizeof(config); i++) {
                EEPROM.write(i, *(p + i));
            }
            EEPROM.commit();
            EEPROM.end();

            request->redirect("/setup");
            LOG.println(request->url() + " handled");
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

            config.samples_per_publish_flag = 0;
            config.samples_per_publish = 3;
            config.publish_interval_flag = 0;
            config.publish_interval = 60000;

            EEPROM.begin(EEPROM_SIZE);
            uint8_t* p = (uint8_t*)(&config);
            for (int i = 0; i < sizeof(config); i++) {
                EEPROM.write(i, *(p + i));
            }
            EEPROM.commit();
            EEPROM.end();

            request->send(200, "text/plain", "Settings Wiped!");
            LOG.println(request->url() + " handled");
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
                LOG.println(request->url() + " handled");
            } else {
                request->send(404, "text/plain", request->url() + " Not found!");
                LOG.println(request->url() + " Not found!");
            }
        });

    // begin the web server
    server.begin();
    LOG.println("HTTP server started");
}

bool isSampleValid(float value) {
    return value < SHRT_MAX && value > SHRT_MIN;
}

String toFloatStr(float value, short decimal_places) {
    char buf[20];
    String fmt;

    fmt = "%." + String(decimal_places) + "f";
    sprintf(buf, fmt.c_str(), value);

    return String(buf);
}
