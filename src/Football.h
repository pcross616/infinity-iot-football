#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Wire.h>
#include <SPI.h>

#include <ArduinoJson.h>
#include <QueueList.h>
#include <EEPROM.h>

#include <Adafruit_PN532.h>
#include <Adafruit_ADXL345_U.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>

// ID of the settings block
#define CONFIG_VERSION "0049"

// Tell it where to store your config data in EEPROM
#define CONFIG_START 0

// settings structure
struct StoreStruct {
        // The variables of your settings
        char SSID[50], SSID_PASSWORD[50],
             MQTT_SERVER[50];
        int MQTT_PORT;
        char MQTT_USERNAME[50],MQTT_PASSWORD[50], MQTT_TOPIC[25], IOT_ID[25];
        // This is for mere detection if they are your settings
        char version_of_program[5]; // it is the last variable of the struct
        // so when settings are saved, they will only be validated if
        // they are stored completely.
} settings = {
        // The default values
        //"IoTDemo", "IoTFootball",
        "XLEY", "pcross616",
        "52.35.108.40", 1883, "", "", "sensors",
        "",
        CONFIG_VERSION
};


// If using the PN532 with SPI, define the pins for SPI communication.
#define PN532_SCK  (14)
#define PN532_MOSI (13)
#define PN532_SS   (2)
#define PN532_MISO (12)

// IRQ for ADXL345
#define ADXL345_IRQ (15)

#define STATE_YAWN (-2)
#define STATE_SLEEP (-1)
#define STATE_UNCONFIGURED (0)
#define STATE_NORMAL (1)
#define STATE_ERROR (2)

extern std::shared_ptr <Adafruit_MQTT_Client> client;
extern std::shared_ptr <Adafruit_MQTT_Publish> topic;
