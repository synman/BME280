/***************************************************************************
 Copyright © 2023 Shell M. Shrader <shell at shellware dot com
 ---------------------------------------------------------------------------
This work is free. You can redistribute it and/or modify it under the
terms of the Do What The Fuck You Want To Public License, Version 2,
as published by Sam Hocevar. See the COPYING file for more details.
****************************************************************************/
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

    if (config.hostname_flag != CFG_SET) {
        strcpy(config.hostname, DEFAULT_HOSTNAME);
    }

    if (config.ssid_flag == CFG_SET) {
        if (String(config.ssid).length() > 0) wifimode = WIFI_STA;
    } else {
        memset(config.ssid, CFG_NOT_SET, WIFI_SSID_LEN);
    }

    if (config.ssid_pwd_flag != CFG_SET) memset(config.ssid_pwd, CFG_NOT_SET, WIFI_PASSWD_LEN);
    if (config.mqtt_server_flag != CFG_SET) memset(config.mqtt_server, CFG_NOT_SET, MQTT_SERVER_LEN);
    if (config.mqtt_user_flag != CFG_SET) memset(config.mqtt_user, CFG_NOT_SET, MQTT_USER_LEN);
    if (config.mqtt_pwd_flag != CFG_SET) memset(config.mqtt_pwd, CFG_NOT_SET, MQTT_PASSWD_LEN);
    if (config.samples_per_publish_flag != CFG_SET) config.samples_per_publish = DEFAULT_SAMPLES_PER_PUBLISH;
    if (config.publish_interval_flag != CFG_SET) config.publish_interval = DEFAULT_PUBLISH_INTERVAL;
    
    LOG.println("        EEPROM size: [" + String(EEPROM_SIZE)+ "]");
    LOG.println("        config size: [" + String(sizeof(config))+ "]\n");
    LOG.println("        config host: [" + String(config.hostname) + "] stored: " + (config.hostname_flag == CFG_SET ? "true" : "false"));
    LOG.println("        config ssid: [" + String(config.ssid) + "] stored: " + (config.ssid_flag == CFG_SET ? "true" : "false"));
    LOG.println("    config ssid pwd: [" + String(config.ssid_pwd) + "] stored: " + (config.ssid_pwd_flag == CFG_SET ? "true\n" : "false\n"));
    LOG.println(" config mqtt server: [" + String(config.mqtt_server) + "] stored: " + (config.mqtt_server_flag == CFG_SET ? "true" : "false"));
    LOG.println("   config mqtt user: [" + String(config.mqtt_user) + "] stored: " + (config.mqtt_user_flag == CFG_SET ? "true" : "false"));
    LOG.println("    config mqtt pwd: [" + String(config.mqtt_pwd) + "] stored: " + (config.mqtt_pwd_flag == CFG_SET ? "true\n" : "false\n"));
    LOG.println(" config num samples: [" + String(config.samples_per_publish) + "] stored: " + (config.samples_per_publish_flag == CFG_SET ? "true" : "false"));
    LOG.println("config pub interval: [" + String(config.publish_interval) + "] stored: " + (config.publish_interval_flag == CFG_SET ? "true\n" : "false\n"));
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

// String template_processor(const String& var) {
//     char ts[255];
//     const String empty = "";
//     String res;
// 
//     if (var == "hostname") res = (config.hostname_flag == 9 ? config.hostname : String(DEFAULT_HOSTNAME));
//     if (var == "ssid") res = config.ssid_flag == 9 ? config.ssid : empty; 
//     if (var == "ssid_pwd") res = config.ssid_pwd_flag == 9 ? config.ssid_pwd : empty;
//     if (var == "mqtt_server") res = config.mqtt_server_flag == 9 ? config.mqtt_server : empty;
//     if (var == "mqtt_user") res = config.mqtt_user_flag == 9 ? config.mqtt_user : empty;
//     if (var == "mqtt_pwd") res = config.mqtt_pwd_flag == 9 ? config.mqtt_pwd : empty;
//     if (var == "samples_per_publish") res = config.samples_per_publish_flag == 9 ? String(config.samples_per_publish) : String(DEFAULT_SAMPLES_PER_PUBLISH);
//     if (var == "publish_interval") res = config.publish_interval_flag == 9 ? String(config.publish_interval) : String(DEFAULT_PUBLISH_INTERVAL);
// 
//     if (var == "timestamp") {
//         if (getLocalTime(&timeinfo)) {
//             sprintf(ts, "\n%4d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
//             res = ts;
//         } else {
//             res = "Failed to obtain time";
//         }
//     }
// 
//     LOG.println("template for [" + var + "] = [" + res + "]");
//     return res;
// }

void updateHtmlTemplate(String template_filename, String temperature = "", String humidity = "", String altitude = "", String pressure = "", String rssi = "") {
    String output_filename = template_filename;
    output_filename.replace(".template", "");

    File _template = LittleFS.open(template_filename, FILE_READ);

    if (_template) {
        String html = _template.readString();
        _template.close();

        while (html.indexOf("{hostname}", 0) != -1) {
            html.replace("{hostname}", String(config.hostname));
        }

        while (html.indexOf("{ssid}", 0) != -1) {
            html.replace("{ssid}", String(config.ssid));
        }

        while (html.indexOf("{ssid_pwd}", 0) != -1) {
            html.replace("{ssid_pwd}", String(config.ssid_pwd));
        }

        while (html.indexOf("{mqtt_server}", 0) != -1) {
            html.replace("{mqtt_server}", String(config.mqtt_server));
        }

        while (html.indexOf("{mqtt_user}", 0) != -1) {
            html.replace("{mqtt_user}", String(config.mqtt_user));
        }

        while (html.indexOf("{mqtt_pwd}", 0) != -1) {
            html.replace("{mqtt_pwd}", String(config.mqtt_pwd));
        }

        while (html.indexOf("{samples_per_publish}", 0) != -1) {
            html.replace("{samples_per_publish}", String(config.samples_per_publish));
        }

        while (html.indexOf("{publish_interval}", 0) != -1) {
            html.replace("{publish_interval}", String(config.publish_interval));
        }

        while (html.indexOf("{publish_interval_in_seconds}", 0) != -1) {
            html.replace("{publish_interval_in_seconds}", String(config.publish_interval / 1000));
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

        if (html.indexOf("{timestamp}", 0) != 1 ) {
            String timestamp;
            char timebuf[255];

            if (!getLocalTime(&timeinfo)) {
                timestamp = "Failed to obtain time";
            } else {
                sprintf(timebuf, "%4d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
                timestamp = String(timebuf);
            }

            while (html.indexOf("{timestamp}", 0) != -1) {
                html.replace("{timestamp}", timestamp);
            }
        }

        File _index = LittleFS.open(output_filename + ".new", FILE_WRITE);
        _index.print(html.c_str());
        _index.close();

        LittleFS.remove(output_filename);
        LittleFS.rename(output_filename + ".new", output_filename);
    }
}

void wireArduinoOTA(const char* hostname) {
    ArduinoOTA.setHostname(hostname);

    ArduinoOTA.onStart([]()
    {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
            type = "sketch";
        else // U_SPIFFS
            type = "filesystem";

        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        LOG.println("OTA triggered for updating " + type);
    });

    ArduinoOTA.onEnd([]()
    {
        LOG.println("\nEnd");
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
    {
        LOG.printf("Progress: %u%%\r", (progress / (total / 100)));
    });

    ArduinoOTA.onError([](ota_error_t error)
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
            ap_mode_activity = true;
            request->redirect("/index.html");
            LOG.println(request->url() + " handled");
        });

    // define setup document
    server.on("/setup", HTTP_GET, [](AsyncWebServerRequest* request)
        {
            request->send(LittleFS, "/setup.html", "text/html");
            LOG.println(request->url() + " handled");
        });

    // captive portal
    server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest* request)
        {
            ap_mode_activity = true;
            request->send(LittleFS, "/index.html", "text/html");
            LOG.println(request->url() + " handled");
        });
    server.on("/library/test/success.html", HTTP_GET, [](AsyncWebServerRequest* request)
        {
            ap_mode_activity = true;
            request->send(LittleFS, "/index.html", "text/html");
            LOG.println(request->url() + " handled");
        });
    server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest* request)
        {
            ap_mode_activity = true;
            request->send(LittleFS, "/index.html", "text/html");
            LOG.println(request->url() + " handled");
        });
    server.on("/gen_204", HTTP_GET, [](AsyncWebServerRequest* request)
        {
            ap_mode_activity = true;
            request->send(LittleFS, "/index.html", "text/html");
            LOG.println(request->url() + " handled");
        });
    server.on("/ncsi.txt", HTTP_GET, [](AsyncWebServerRequest* request)
        {
            ap_mode_activity = true;
            request->send(LittleFS, "/index.html", "text/html");
            LOG.println(request->url() + " handled");
        });
    server.on("/check_network_status.txt", HTTP_GET, [](AsyncWebServerRequest* request)
        {
            ap_mode_activity = true;
            request->send(LittleFS, "/index.html", "text/html");
            LOG.println(request->url() + " handled");
        });

    // request reboot
    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest* request)
        {
            request->redirect("/index.html");
            LOG.println(request->url() + " handled");
            ota_needs_reboot = true;
        });

    // save config
    server.on("/save", HTTP_GET, [](AsyncWebServerRequest* request)
        {
            if (request->hasParam("hostname")) {
                memset(config.hostname, CFG_NOT_SET, HOSTNAME_LEN);
                String hostname = request->getParam("hostname")->value();
                if (hostname.length() > 0) {                    
                    config.hostname_flag = CFG_SET;
                } else {
                    config.hostname_flag = CFG_NOT_SET;
                    hostname = DEFAULT_HOSTNAME;
                }
                hostname.toCharArray(config.hostname, HOSTNAME_LEN);
            }

            if (request->hasParam("ssid")) {
                const String ssid = request->getParam("ssid")->value();
                memset(config.ssid, CFG_NOT_SET, WIFI_SSID_LEN);
                if (ssid.length() > 0) {
                    ssid.toCharArray(config.ssid, WIFI_SSID_LEN);
                    config.ssid_flag = CFG_SET;
                } else {
                    config.ssid_flag = CFG_NOT_SET;
                }
            }

            if (request->hasParam("ssid_pwd")) {
                const String ssid_pwd = request->getParam("ssid_pwd")->value();
                memset(config.ssid_pwd, CFG_NOT_SET, WIFI_PASSWD_LEN);
                if (ssid_pwd.length() > 0) {
                    ssid_pwd.toCharArray(config.ssid_pwd, WIFI_PASSWD_LEN);
                    config.ssid_pwd_flag = CFG_SET;
                } else {
                    config.ssid_pwd_flag = CFG_NOT_SET;
                }
            }

            if (request->hasParam("mqtt_server")) {
                const String mqtt_server = request->getParam("mqtt_server")->value();
                memset(config.mqtt_server, CFG_NOT_SET, MQTT_SERVER_LEN);
                if (mqtt_server.length() > 0) {
                    mqtt_server.toCharArray(config.mqtt_server, MQTT_SERVER_LEN);
                    config.mqtt_server_flag = CFG_SET;
                } else {
                    config.mqtt_server_flag = CFG_NOT_SET;
                }
            }

            if (request->hasParam("mqtt_user")) {
                const String mqtt_user = request->getParam("mqtt_user")->value();
                memset(config.mqtt_user, CFG_NOT_SET, MQTT_USER_LEN);
                if (mqtt_user.length() > 0) {
                    mqtt_user.toCharArray(config.mqtt_user, MQTT_USER_LEN);
                    config.mqtt_user_flag = CFG_SET;
                } else {
                    config.mqtt_user_flag = CFG_NOT_SET;
                }
            }

            if (request->hasParam("mqtt_pwd")) {
                const String mqtt_pwd = request->getParam("mqtt_pwd")->value();
                memset(config.mqtt_pwd, CFG_NOT_SET, MQTT_PASSWD_LEN);
                if (mqtt_pwd.length() > 0) {
                    mqtt_pwd.toCharArray(config.mqtt_pwd, MQTT_PASSWD_LEN);
                    config.mqtt_pwd_flag = CFG_SET;
                } else {
                    config.mqtt_pwd_flag = CFG_NOT_SET;
                }
            }

            if (request->hasParam("samples_per_publish")) {
                const int samples = request->getParam("samples_per_publish")->value().toInt();
                if (samples > MIN_SAMPLES_PER_PUBLISH) {
                    config.samples_per_publish = samples;
                    config.samples_per_publish_flag = CFG_SET;
                } else {
                    config.samples_per_publish_flag = CFG_NOT_SET;
                    config.samples_per_publish = DEFAULT_SAMPLES_PER_PUBLISH;
                }
            }

            if (request->hasParam("publish_interval")) {
                const unsigned long interval = strtoul(request->getParam("publish_interval")->value().c_str(), NULL, 10);
                if (interval > MIN_PUBLISH_INTERVAL && interval != DEFAULT_PUBLISH_INTERVAL) {
                    config.publish_interval = interval;
                    config.publish_interval_flag = CFG_SET;
                } else {
                    config.publish_interval_flag = CFG_NOT_SET;
                    config.publish_interval = DEFAULT_PUBLISH_INTERVAL;
                }
            }

            EEPROM.begin(EEPROM_SIZE);
            uint8_t* p = (uint8_t*)(&config);
            for (unsigned long i = 0; i < sizeof(config); i++) {
                EEPROM.write(i, *(p + i));
            }
            EEPROM.commit();
            EEPROM.end();

            setup_needs_update = true;
            request->redirect("/index.html");
            LOG.println(request->url() + " handled");
        });

    // load config
    server.on("/load", HTTP_GET, [](AsyncWebServerRequest* request)
        {
            LOG.println();
            wireConfig();
            setup_needs_update = true;
            request->redirect("/index.html");
            LOG.println(request->url() + " handled");
        });

    // wipe config
    server.on("/wipe", HTTP_GET, [](AsyncWebServerRequest* request)
        {
            const boolean reboot = !request->hasParam("noreboot");

            config.hostname_flag = CFG_NOT_SET;
            strcpy(config.hostname, DEFAULT_HOSTNAME);
            config.ssid_flag = CFG_NOT_SET;
            memset(config.ssid, CFG_NOT_SET, WIFI_SSID_LEN);
            config.ssid_pwd_flag = CFG_NOT_SET;
            memset(config.ssid_pwd, CFG_NOT_SET, WIFI_PASSWD_LEN);
            config.mqtt_server_flag = CFG_NOT_SET;
            memset(config.mqtt_server, CFG_NOT_SET, MQTT_SERVER_LEN);
            config.mqtt_user_flag = CFG_NOT_SET;
            memset(config.mqtt_user, CFG_NOT_SET, MQTT_USER_LEN);
            config.mqtt_pwd_flag = CFG_NOT_SET;
            memset(config.mqtt_pwd, CFG_NOT_SET, MQTT_PASSWD_LEN);

            config.samples_per_publish_flag = CFG_NOT_SET;
            config.samples_per_publish = DEFAULT_SAMPLES_PER_PUBLISH;
            config.publish_interval_flag = CFG_NOT_SET;
            config.publish_interval = DEFAULT_PUBLISH_INTERVAL;

            EEPROM.begin(EEPROM_SIZE);
            uint8_t* p = (uint8_t*)(&config);
            for (unsigned long i = 0; i < sizeof(config); i++) {
                EEPROM.write(i, *(p + i));
            }
            EEPROM.commit();
            EEPROM.end();

            request->redirect("/index.html");
            LOG.println(request->url() + " handled");

            // trigger a reboot
            if (reboot) ota_needs_reboot = true;
        });

    // 404 (includes file handling)
    server.onNotFound([](AsyncWebServerRequest* request)
        {
            ap_mode_activity = true;

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
