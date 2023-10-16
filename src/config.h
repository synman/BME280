#define EEPROM_SIZE 256
#define HOSTNAME_LEN 32
#define WIFI_SSID_LEN 32
#define WIFI_PASSWD_LEN 64
#define MQTT_SERVER_LEN 16
#define MQTT_USER_LEN 16
#define MQTT_PASSWD_LEN 32

typedef struct config_type
{
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

CONFIG_TYPE config;