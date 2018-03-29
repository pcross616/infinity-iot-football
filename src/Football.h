#if defined(ARDUINO) && (ARDUINO >= 100)
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

#ifdef IOTFOOTBALL_OTA
#include <ArduinoOTA.h>
#endif

// WifiManager
#include <WiFiManager.h>

// Harware 
#include <SPI.h>
#include <Ticker.h>
#include <Wire.h>

// Supporting Classes
#include <ArduinoJson.h>
#include <QueueList.h>
#include <DoubleResetDetector.h>

// Sensors
#include <Adafruit_ADXL345_U.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include <Adafruit_PN532.h>
#include <Adafruit_Sensor.h>

extern "C" {
#include <user_interface.h>
}


// settings structure
struct StoreStruct {
  // The variables of your settings
  char MQTT_SERVER[50], MQTT_PORT[10], MQTT_USERNAME[50], MQTT_PASSWORD[50],
      MQTT_TOPIC[25], IOT_ID[25];
} settings = {"iotdemo.dev.infinitycloud.io",
              "1833",
              "",
              "",
              "IoTFootball",
              "IoTFootball"};

// If using the PN532 with SPI, define the pins for SPI communication.
#define PN532_SCK (14)
#define PN532_MOSI (13)
#define PN532_SS (2)
#define PN532_MISO (12)

// IRQ for ADXL345
#define ADXL345_IRQ (15)

// Football state
#define STATE_YAWN (-2)
#define STATE_SLEEP (-1)
#define STATE_UNCONFIGURED (0)
#define STATE_NORMAL (1)
#define STATE_ERROR (2)

// Number of seconds after reset during which a
// subseqent reset will be considered a double reset.
#define DRD_TIMEOUT (10)

// RTC Memory Address for the DoubleResetDetector to use
#define DRD_ADDRESS (0)

//#define IOTFOOTBALL_ALLOW_SLEEP

//comment out these lines to change debug levels
#ifdef IOTFOOTBALL_DEBUG
#define PN532DEBUG
#define MQTT_DEBUG
#define MIFAREDEBUG

#else

#undef PN532DEBUG
#undef MQTT_DEBUG
#undef MIFAREDEBUG

#endif

extern std::shared_ptr<Adafruit_MQTT_Client> client;
extern std::shared_ptr<Adafruit_MQTT_Publish> topic;

void setup_wifi();
void setup_rfid();
void setup_sensors();

void configure(WiFiManager *myWiFiManager);
void publish(const char *);
void sensorReadADXL();
void sensorReadRFID();
void sleepSensors();
void processQueue();

void tick();
void displaySensorDetails();
void displayDataRate();
void displayRange();

String hex_str(const byte *data, const uint32_t numBytes);
inline size_t hash_string(const char *__s);
void print_byte(byte val);