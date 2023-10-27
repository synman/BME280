/***************************************************************************
Copyright © 2023 Shell M. Shrader <shell at shellware dot com>
----------------------------------------------------------------------------
This work is free. You can redistribute it and/or modify it under the
terms of the Do What The Fuck You Want To Public License, Version 2,
as published by Sam Hocevar. See the COPYING file for more details.
****************************************************************************/
#include "config.h"


void watchDogRefresh() {
  #ifdef esp32
    timerWrite(watchDogTimer, 0); 
  #else
    if (timer_pinged) {
      timer_pinged = false;
      LOG_PRINTLN("PONG");
      LOG_FLUSH();
    }
  #endif
}

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
    EEPROM.get(0, config);
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

    if (config.nws_station_flag != CFG_SET) memset(config.nws_station, CFG_NOT_SET, NWS_STATION_LEN);
    
    LOG_PRINTLN();
    LOG_PRINTLN("        EEPROM size: [" + String(EEPROM_SIZE)+ "]");
    LOG_PRINTLN("        config size: [" + String(sizeof(config))+ "]\n");
    LOG_PRINTLN("        config host: [" + String(config.hostname) + "] stored: " + (config.hostname_flag == CFG_SET ? "true" : "false"));
    LOG_PRINTLN("        config ssid: [" + String(config.ssid) + "] stored: " + (config.ssid_flag == CFG_SET ? "true" : "false"));
    LOG_PRINTLN("    config ssid pwd: [" + String(config.ssid_pwd) + "] stored: " + (config.ssid_pwd_flag == CFG_SET ? "true\n" : "false\n"));
    LOG_PRINTLN(" config mqtt server: [" + String(config.mqtt_server) + "] stored: " + (config.mqtt_server_flag == CFG_SET ? "true" : "false"));
    LOG_PRINTLN("   config mqtt user: [" + String(config.mqtt_user) + "] stored: " + (config.mqtt_user_flag == CFG_SET ? "true" : "false"));
    LOG_PRINTLN("    config mqtt pwd: [" + String(config.mqtt_pwd) + "] stored: " + (config.mqtt_pwd_flag == CFG_SET ? "true\n" : "false\n"));
    LOG_PRINTLN(" config num samples: [" + String(config.samples_per_publish) + "] stored: " + (config.samples_per_publish_flag == CFG_SET ? "true" : "false"));
    LOG_PRINTLN("config pub interval: [" + String(config.publish_interval) + "] stored: " + (config.publish_interval_flag == CFG_SET ? "true\n" : "false\n"));
    LOG_PRINTLN(" config nws station: [" + String(config.nws_station) + "] stored: " + (config.nws_station_flag == CFG_SET ? "true" : "false"));
}

void onOTAStart() {
    // Log when OTA has started
    LOG_PRINTLN("\nOTA update started!");
    // <Add your own code here>
}

void onOTAProgress(size_t current, size_t final) {
    // Log every 1 second
    if (millis() - ota_progress_millis > 1000) {
        watchDogRefresh();
        ota_progress_millis = millis();
        LOG_PRINTF("OTA Progress Current: %u bytes, Final: %u bytes\r", current, final);
        LOG_FLUSH();
    }
}

void onOTAEnd(bool success) {
    // Log when OTA has finished
    if (success) {
        LOG_PRINTLN("\nOTA update finished successfully!");
        esp_reboot_requested = true;
    } else {
        LOG_PRINTLN("\nThere was an error during OTA update!");
    }
    LOG_FLUSH();
}

void updateHtmlTemplate(String template_filename, 
                        String temperature = "", 
                        String humidity = "", 
                        String altitude = "", 
                        String pressure = "", 
                        String rssi = "",
                        bool showTime = true) {

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

        while (html.indexOf("{nws_station}", 0) != -1) {
            html.replace("{nws_station}", String(config.nws_station));
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

        while (html.indexOf("{sea_level_atmospheric_pressure}", 0) != -1) {
            html.replace("{sea_level_atmospheric_pressure}", String(SEALEVELPRESSURE_HPA * HPA_TO_INHG));
        }

        if (html.indexOf("{timestamp}", 0) != 1 ) {
            String timestamp;
            struct tm timeinfo;
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

            if (showTime) 
                LOG_PRINTLN("Timestamp   = " + timestamp);
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
        LOG_PRINTLN("\nOTA triggered for updating " + type);
    });

    ArduinoOTA.onEnd([]()
    {
        LOG_PRINTLN("\nOTA End");
        LOG_FLUSH();
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
    {
        watchDogRefresh();
        LOG_PRINTF("Progress: %u%%\r", (progress / (total / 100)));
        LOG_FLUSH();
    });

    ArduinoOTA.onError([](ota_error_t error)
    {
        LOG_PRINTF("\nError[%u]: ", error);
        if (error == OTA_AUTH_ERROR) LOG_PRINTLN("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) LOG_PRINTLN("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) LOG_PRINTLN("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) LOG_PRINTLN("Receive Failed");
        else if (error == OTA_END_ERROR) LOG_PRINTLN("End Failed");
        LOG_FLUSH();
    });

    ArduinoOTA.begin();
    LOG_PRINTLN("\nArduinoOTA started");
}

void wireWebServerAndPaths() {
    // define default document
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request)
        {
            ap_mode_activity = true;
            request->redirect("/index.html");
            LOG_PRINTLN("\n" + request->url() + " handled");
        });

    // define setup document
    server.on("/setup", HTTP_GET, [](AsyncWebServerRequest* request)
        {
            request->send(LittleFS, "/setup.html", "text/html");
            LOG_PRINTLN("\n" + request->url() + " handled");
        });

    // captive portal
    server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest* request)
        {
            ap_mode_activity = true;
            request->send(LittleFS, "/index.html", "text/html");
            LOG_PRINTLN("\n" + request->url() + " handled");
        });
    server.on("/library/test/success.html", HTTP_GET, [](AsyncWebServerRequest* request)
        {
            ap_mode_activity = true;
            request->send(LittleFS, "/index.html", "text/html");
            LOG_PRINTLN("\n" + request->url() + " handled");
        });
    server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest* request)
        {
            ap_mode_activity = true;
            request->send(LittleFS, "/index.html", "text/html");
            LOG_PRINTLN("\n" + request->url() + " handled");
        });
    server.on("/gen_204", HTTP_GET, [](AsyncWebServerRequest* request)
        {
            ap_mode_activity = true;
            request->send(LittleFS, "/index.html", "text/html");
            LOG_PRINTLN("\n" + request->url() + " handled");
        });
    server.on("/ncsi.txt", HTTP_GET, [](AsyncWebServerRequest* request)
        {
            ap_mode_activity = true;
            request->send(LittleFS, "/index.html", "text/html");
            LOG_PRINTLN("\n" + request->url() + " handled");
        });
    server.on("/check_network_status.txt", HTTP_GET, [](AsyncWebServerRequest* request)
        {
            ap_mode_activity = true;
            request->send(LittleFS, "/index.html", "text/html");
            LOG_PRINTLN("\n" + request->url() + " handled");
        });

    // request reboot
    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest* request)
        {
            request->redirect("/index.html");
            LOG_PRINTLN("\n" + request->url() + " handled");
            esp_reboot_requested = true;
        });

    // save config
    server.on("/save", HTTP_GET, [](AsyncWebServerRequest* request)
        {
            tiny_int samples_per_publish = DEFAULT_SAMPLES_PER_PUBLISH;
            unsigned long publish_interval = DEFAULT_PUBLISH_INTERVAL;

            if (isNumeric(request->getParam("samples_per_publish")->value())) {
                samples_per_publish = request->getParam("samples_per_publish")->value().toInt();
            }

            String interval = request->getParam("publish_interval")->value();
            if (isNumeric(interval)) {
                publish_interval = strtoul(interval.c_str(), 0, 10);
            }

            saveConfig(request->getParam("hostname")->value(),
                       request->getParam("ssid")->value(),
                       request->getParam("ssid_pwd")->value(),
                       request->getParam("mqtt_server")->value(),
                       request->getParam("mqtt_user")->value(),
                       request->getParam("mqtt_pwd")->value(),
                       samples_per_publish,
                       publish_interval,
                       request->getParam("nws_station")->value());

            request->redirect("/index.html");
            LOG_PRINTLN("\n" + request->url() + " handled");
        });

    // load config
    server.on("/load", HTTP_GET, [](AsyncWebServerRequest* request)
        {
            LOG_PRINTLN();
            wireConfig();
            setup_needs_update = true;
            request->redirect("/index.html");
            LOG_PRINTLN("\n" + request->url() + " handled");
        });

    // wipe config
    server.on("/wipe", HTTP_GET, [](AsyncWebServerRequest* request)
        {
            const boolean reboot = !request->hasParam("noreboot");

            wipeConfig();

            request->redirect("/index.html");
            LOG_PRINTLN("\n" + request->url() + " handled");

            // trigger a reboot
            if (reboot) esp_reboot_requested = true;
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
                LOG_PRINTLN("\n" + request->url() + " handled");
            } else {
                request->send(404, "text/plain", request->url() + " Not found!");
                LOG_PRINTLN("\n" + request->url() + " Not found!");
            }
        });

    // begin the web server
    server.begin();
    LOG_PRINTLN("HTTP server started");
}

void saveConfig(String hostname, 
                String ssid, 
                String ssid_pwd, 
                String mqtt_server, 
                String mqtt_user, 
                String mqtt_pwd, 
                tiny_int samples_per_publish, 
                unsigned long publish_interval, 
                String nws_station) {

    memset(config.hostname, CFG_NOT_SET, HOSTNAME_LEN);
    if (hostname.length() > 0) {                    
        config.hostname_flag = CFG_SET;
    } else {
        config.hostname_flag = CFG_NOT_SET;
        hostname = DEFAULT_HOSTNAME;
    }
    hostname.toCharArray(config.hostname, HOSTNAME_LEN);

    memset(config.ssid, CFG_NOT_SET, WIFI_SSID_LEN);
    if (ssid.length() > 0) {
        ssid.toCharArray(config.ssid, WIFI_SSID_LEN);
        config.ssid_flag = CFG_SET;
    } else {
        config.ssid_flag = CFG_NOT_SET;
    }

    memset(config.ssid_pwd, CFG_NOT_SET, WIFI_PASSWD_LEN);
    if (ssid_pwd.length() > 0) {
        ssid_pwd.toCharArray(config.ssid_pwd, WIFI_PASSWD_LEN);
        config.ssid_pwd_flag = CFG_SET;
    } else {
        config.ssid_pwd_flag = CFG_NOT_SET;
    }

    memset(config.mqtt_server, CFG_NOT_SET, MQTT_SERVER_LEN);
    if (mqtt_server.length() > 0) {
        mqtt_server.toCharArray(config.mqtt_server, MQTT_SERVER_LEN);
        config.mqtt_server_flag = CFG_SET;
    } else {
        config.mqtt_server_flag = CFG_NOT_SET;
    }

    memset(config.mqtt_user, CFG_NOT_SET, MQTT_USER_LEN);
    if (mqtt_user.length() > 0) {
        mqtt_user.toCharArray(config.mqtt_user, MQTT_USER_LEN);
        config.mqtt_user_flag = CFG_SET;
    } else {
        config.mqtt_user_flag = CFG_NOT_SET;
    }

    memset(config.mqtt_pwd, CFG_NOT_SET, MQTT_PASSWD_LEN);
    if (mqtt_pwd.length() > 0) {
        mqtt_pwd.toCharArray(config.mqtt_pwd, MQTT_PASSWD_LEN);
        config.mqtt_pwd_flag = CFG_SET;
    } else {
        config.mqtt_pwd_flag = CFG_NOT_SET;
    }

    if (samples_per_publish > MIN_SAMPLES_PER_PUBLISH) {
        config.samples_per_publish = samples_per_publish;
        config.samples_per_publish_flag = CFG_SET;
    } else {
        config.samples_per_publish_flag = CFG_NOT_SET;
        config.samples_per_publish = DEFAULT_SAMPLES_PER_PUBLISH;
    }

    if (publish_interval > MIN_PUBLISH_INTERVAL && publish_interval != DEFAULT_PUBLISH_INTERVAL) {
        config.publish_interval = publish_interval;
        config.publish_interval_flag = CFG_SET;
    } else {
        config.publish_interval_flag = CFG_NOT_SET;
        config.publish_interval = DEFAULT_PUBLISH_INTERVAL;
    }

    memset(config.nws_station, CFG_NOT_SET, NWS_STATION_LEN);
    if (nws_station.length() > 0) {
        nws_station.toCharArray(config.nws_station, NWS_STATION_LEN);
        config.nws_station_flag = CFG_SET;
    } else {
        config.nws_station_flag = CFG_NOT_SET;
    }

    EEPROM.begin(EEPROM_SIZE);
    EEPROM.put(0, config);
    EEPROM.commit();
    EEPROM.end();

    setup_needs_update = true;
}

void wipeConfig() {
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

    config.nws_station_flag = CFG_NOT_SET;
    memset(config.nws_station, CFG_NOT_SET, NWS_STATION_LEN);

    EEPROM.begin(EEPROM_SIZE);
    EEPROM.put(0,config);
    EEPROM.commit();
    EEPROM.end();

    LOG_PRINTLN("\nConfig wiped");
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

#ifdef BME280_DEBUG
void checkForRemoteCommand() {
    if (SerialAndTelnet.available() > 0) {
		char c = SerialAndTelnet.read();
		switch (c) {
		case '\r':
			LOG_PRINT("\r");
			break;
		case '\n':
			LOG_PRINT("\n");
			break;
		case 'D':
			LOG_PRINTLN("\nDisconnecting Wi-Fi. . .");
            LOG_FLUSH();
            WiFi.disconnect();
			break;
        case 'P':
            SEALEVELPRESSURE_HPA = getSeaLevelPressure();
            LOG_PRINTLN("\nSea Level Pressure: [" + String(SEALEVELPRESSURE_HPA) + "]\n");        
            break;
        case 'F':
            {
                #ifdef esp32
                    const size_t fs_size = LittleFS.totalBytes() / 1000;
                    const size_t fs_used = LittleFS.usedBytes() / 1000;
                #else
                    FSInfo fs_info;
                    LittleFS.info(fs_info);
                    const size_t fs_size = fs_info.totalBytes / 1000;
                    const size_t fs_used = fs_info.usedBytes / 1000;
                #endif
                LOG_PRINTLN("\n    Filesystem size: [" + String(fs_size) + "] KB");
                LOG_PRINTLN("         Free space: [" + String(fs_size - fs_used) + "] KB\n");
            }
            break;
		case 'S':
            {
                LOG_PRINTLN("\nType SSID and press <ENTER>");
                LOG_FLUSH();

                String ssid;
                do {
                    if (SerialAndTelnet.available() > 0) {
                        c = SerialAndTelnet.read();
                        if (c != 10 && c != 13) {
                            LOG_PRINT(c);
                            LOG_FLUSH();
                            ssid = ssid + String(c);
                        }
                    }
                    watchDogRefresh();
                } while (c != 13);

                LOG_PRINTLN("\nType PASSWORD and press <ENTER>");
                LOG_FLUSH();
                String ssid_pwd;
                do {
                    if (SerialAndTelnet.available() > 0) {
                        c = SerialAndTelnet.read();
                        if (c != 10 && c != 13) {
                            LOG_PRINT(c);
                            LOG_FLUSH();
                            ssid_pwd = ssid_pwd + String(c);
                        }
                    }
                    watchDogRefresh();
                } while (c != 13);

                LOG_PRINTLN("\n\nSSID=[" + ssid + "] PWD=[" + ssid_pwd + "]\n");
                LOG_FLUSH();

                memset(config.ssid, CFG_NOT_SET, WIFI_SSID_LEN);
                if (ssid.length() > 0) {
                    ssid.toCharArray(config.ssid, WIFI_SSID_LEN);
                    config.ssid_flag = CFG_SET;
                } else {
                    config.ssid_flag = CFG_NOT_SET;
                }

                memset(config.ssid_pwd, CFG_NOT_SET, WIFI_PASSWD_LEN);
                if (ssid_pwd.length() > 0) {
                    ssid_pwd.toCharArray(config.ssid_pwd, WIFI_PASSWD_LEN);
                    config.ssid_pwd_flag = CFG_SET;
                } else {
                    config.ssid_pwd_flag = CFG_NOT_SET;
                }

                EEPROM.begin(EEPROM_SIZE);
                EEPROM.put(0, config);
                EEPROM.commit();
                EEPROM.end();

                LOG_PRINTLN("SSID and Password saved - reload config or reboot\n");
                LOG_FLUSH();
            }
			break;
		case 'L':
            wireConfig();
            setup_needs_update = true;
			break;
		case 'W':
            wipeConfig();
			break;
		case 'X':
			LOG_PRINTLN(F("\r\nClosing session..."));
			SerialAndTelnet.disconnectClient();
			break;
		case 'R':
			LOG_PRINTLN(F("\r\nsubmitting reboot request..."));
			esp_reboot_requested = true;
			break;
        case ' ':
            // do nothing -- just a simple echo
            break;
		default:
			LOG_PRINT("\n\nCommands:\n\nD = Disconnect WiFi\nP = Sea Level Pressure\nF = Filesystem Info\nS - Set SSID / Password\nL = Reload Config\nW = Wipe Config\nX = Close Session\nR = Reboot ESP\n\n");
			break;
		}
		SerialAndTelnet.flush();
	}
}
#endif

float getSeaLevelPressure() {
    if (config.nws_station_flag == CFG_NOT_SET) {
        LOG_PRINTLN("\nNWS Station is not set - using default sea level pressure");
        return DEFAULT_SEALEVELPRESSURE_HPA;
    }

    if (wifimode == WIFI_AP) {
        LOG_PRINTLN("\nIn AP mode - altitude will be ignored");
        return INVALID_SEALEVELPRESSURE_HPA;
    }

    const String observationsUrl = "https://api.weather.gov/stations/" + String(config.nws_station) + "/observations/latest";
    DynamicJsonDocument doc(8192);

#ifdef esp32
    WiFiClientSecure httpsClient;
    
    httpsClient.setInsecure();
    httpsClient.setTimeout(5000);

    if (httpsClient.connect("api.weather.gov", 443)) {
        httpsClient.println("GET " + observationsUrl + " HTTP/1.0");
        httpsClient.println("Host: api.weather.gov");
        httpsClient.println("User-Agent: curl/8.1.2");
        httpsClient.println("Accept: application/json");
        httpsClient.println("Connection: close");
        httpsClient.println();
    } else {
        LOG_PRINTLN("\nUnable to connect to NWS - altitude will be ignored");
        return INVALID_SEALEVELPRESSURE_HPA;
    }

    // read headers
    while (httpsClient.connected()) {
        String line = httpsClient.readStringUntil('\n');
        if (line == "\r") break;
    }

    if (!httpsClient.connected()) {
        LOG_PRINTLN("\nNWS dropped connnection - altitude will be ignored");
        return INVALID_SEALEVELPRESSURE_HPA;
    }

    deserializeJson(doc, httpsClient.readString());

    httpsClient.stop();
#else
    HTTPClient httpClient;
    std::unique_ptr<BearSSL::WiFiClientSecure>httpsClient(new BearSSL::WiFiClientSecure);

    httpsClient->setInsecure();
    httpsClient->setTimeout(5000);
    httpsClient->setBufferSizes(4096,255);

    if (httpClient.begin(*httpsClient, observationsUrl)) {
        httpClient.addHeader("Accept", "application/json");
        if (httpClient.GET() == HTTP_CODE_OK) {
               deserializeJson(doc, httpClient.getString());
        } else {
            LOG_PRINTLN("\nBad HTTP Response Code from NWS - altitude will be ignored");
            return INVALID_SEALEVELPRESSURE_HPA;
        }
    } else {
        LOG_PRINTLN("\nUnable to connect to NWS - altitude will be ignored");
        return INVALID_SEALEVELPRESSURE_HPA;
    }
#endif

    String properties = doc["properties"];
    deserializeJson(doc, properties);
    String seaLevelPressure = doc["seaLevelPressure"];
    deserializeJson(doc, seaLevelPressure);

    String value = doc["value"];

    if (isNumeric(value)) {
        return value.toInt() / 100.0;
    } 

    LOG_PRINTLN("\nInvalid response from NWS - altitude will be ignored");
    return INVALID_SEALEVELPRESSURE_HPA;
}

boolean isNumeric(String str) {
    unsigned int stringLength = str.length();
 
    if (stringLength == 0) {
        return false;
    }
 
    boolean seenDecimal = false;
 
    for(unsigned int i = 0; i < stringLength; ++i) {
        if (isDigit(str.charAt(i))) {
            continue;
        }
 
        if (str.charAt(i) == '.') {
            if (seenDecimal) {
                return false;
            }
            seenDecimal = true;
            continue;
        }
        return false;
    }
    return true;
}

void printHeapStats() {
    #ifdef BME280_DEBUG
        uint32_t myfree;
        uint32_t mymax;
        uint8_t myfrag;

        #ifdef esp32 
        const uint32_t size = ESP.getHeapSize();
        const uint32_t free = ESP.getFreeHeap();
        const uint32_t max = ESP.getMaxAllocHeap();
        const uint32_t min = ESP.getMinFreeHeap();
        LOG_PRINTF("\n(%ld) -> size: %5d - free: %5d - max: %5d - min: %5d <-\n", millis(), size, free, max, min);
        #else
        ESP.getHeapStats(&myfree, &mymax, &myfrag);
        LOG_PRINTF("\n(%ld) -> free: %5d - max: %5d - frag: %3d%% <-\n", millis(), myfree, mymax, myfrag);
        #endif
    #endif

    return;
}