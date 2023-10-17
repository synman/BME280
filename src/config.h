#define EEPROM_SIZE 256
#define HOSTNAME_LEN 32
#define WIFI_SSID_LEN 32
#define WIFI_PASSWD_LEN 64
#define MQTT_SERVER_LEN 16
#define MQTT_USER_LEN 16
#define MQTT_PASSWD_LEN 32

typedef struct config_type {
    unsigned char hostname_flag;
    char hostname[HOSTNAME_LEN];
    unsigned char ssid_flag;
    char ssid[WIFI_SSID_LEN];
    unsigned char ssid_pwd_flag;
    char ssid_pwd[WIFI_PASSWD_LEN];
    unsigned char mqtt_server_flag;
    char mqtt_server[MQTT_SERVER_LEN];
    unsigned char mqtt_user_flag;
    char mqtt_user[MQTT_USER_LEN];
    unsigned char mqtt_pwd_flag;
    char mqtt_pwd[MQTT_PASSWD_LEN];
} CONFIG_TYPE;

typedef struct samples_type {
    float temperature = 0;
    float humidity = 0;
    float altitude = 0;
    float pressure = 0;
    float rssi = 0;
    float high_temperature = std::numeric_limits<float>::min();
    float high_humidity = std::numeric_limits<float>::min();
    float high_altitude = std::numeric_limits<float>::min();
    float high_pressure = std::numeric_limits<float>::min();
    float high_rssi = std::numeric_limits<float>::min();
    float low_temperature = std::numeric_limits<float>::max();
    float low_humidity = std::numeric_limits<float>::max();
    float low_altitude = std::numeric_limits<float>::max();
    float low_pressure = std::numeric_limits<float>::max();
    float low_rssi = std::numeric_limits<float>::max();
    unsigned short sample_count = 0;
    unsigned long last_update = -5001;
} SAMPLES_TYPE;

CONFIG_TYPE config;
SAMPLES_TYPE samples;