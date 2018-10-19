#include <Football.h>

// Use this line for a breakout with a SPI connection:
// Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);
static Adafruit_PN532 nfc(PN532_SS);

/* Assign a unique ID to this sensor at the same time */
static Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

static QueueList<String> queue;
static WiFiClient espClient;
static Ticker ticker;

std::shared_ptr<Adafruit_MQTT_Client> client = NULL;
std::shared_ptr<Adafruit_MQTT_Publish> topic = NULL;

static uint8_t last_uid[] = {0, 0, 0, 0, 0, 0, 0};
//static uint8_t prev_uid[] = {0, 0, 0, 0, 0, 0, 0};
static unsigned long last_uid_update = 0;
//static unsigned long prev_uid_update = 0;

static int state = STATE_UNCONFIGURED;

static DoubleResetDetector drd(DRD_TIMEOUT, DRD_ADDRESS);

/**
 * Setup the connections and chips
 */
void setup() {
  pinMode(LED_BUILTIN, OUTPUT); // Initialize the LED_BUILTIN pin as an output
  Serial.begin(115200);
  delay(2000);

  rst_info *rinfo;
  rinfo = ESP.getResetInfoPtr();

  // if we woke from deep sleep, do a full reset
  // this is needed because http gets from deep sleep are not properly returned.
  // test to see if this is fixed in future esp8266 libraries

  if ((*rinfo).reason == REASON_DEEP_SLEEP_AWAKE) {
    Serial.println("Woke from deep sleep, performing full reset");
    drd.stop();
    ESP.restart();
  }

  Serial.println("");
  Serial.println("");
  Serial.println("");
  Serial.println(
      "Oracle Infinity IoT Football Demo - 2018 <pete.crossley@oracle.com>");

  // Setup components
  setup_wifi();

  if (state == STATE_NORMAL) {
    setup_rfid();
    setup_sensors();

    digitalWrite(LED_BUILTIN, HIGH); // make sure its off during init.
  }

  // setup the queue
  queue.setPrinter(Serial);
  Serial.println("Setup complete, let's play!");
}

/**
 * Main process loop
 */
void loop() {
  if (state == STATE_ERROR) {
    if (!ticker.active()) {
      Serial.println("IoTFootball in ERROR state... please reconfigure.");
      ticker.attach(0.1, tick);
    }
  }

  if (state == STATE_YAWN) { // get ready for bed
    sleepSensors();
  }

  if (state == STATE_SLEEP) // deep sleep
  {
    Serial.println("Sleeping ESP8266 in 5 seconds.");
    delay(5000);
    digitalWrite(LED_BUILTIN, LOW); // make sure its off during sleep.
    ESP.deepSleep(0);
  }

  if (state == STATE_NORMAL) {
    digitalWrite(LED_BUILTIN, HIGH); // make sure its off during init.
    // handle rfid in the main loop
    sensorReadRFID();
  }
  if ((state == STATE_NORMAL) || (state == STATE_YAWN)) {
    // process the queue to ensure all the data is on the wire
    processQueue();
  }
  drd.loop();
}

/**
 * Ensure the RFID board is present.
 */
void setup_rfid() {
  Serial.println("Initializing PN53x...!");

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1) // halt
    {
    }
  }
  // Got ok data, print it out!
  Serial.print("Found chip PN5");
  Serial.println((versiondata >> 24) & 0xFF, HEX);
  Serial.print("Firmware ver. ");
  Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.');
  Serial.println((versiondata >> 8) & 0xFF, DEC);

  // configure board to read RFID tags
  nfc.SAMConfig();
}

/**
 * Configure WiFi to verify connectivity to the internet.
 */
void setup_wifi() {
  // Local intialization. Once its business is done, there is no need to keep it
  // around
  WiFiManager wifiManager;

  if (drd.detectDoubleReset()) {
    Serial.println("Reseting WiFi network and system settings...!");
    wifiManager.resetSettings();
    drd.stop();
  }
  Serial.println("Initializing WiFi network...!");
  // WiFiManager

  ticker.attach(0.6, tick);

  wifiManager.setAPCallback(configure);

  // id/name, placeholder/prompt, default, length
  WiFiManagerParameter custom_mqtt_server("mqtt.server", "MQTT server",
                                          settings.MQTT_SERVER, 50);
  wifiManager.addParameter(&custom_mqtt_server);

  WiFiManagerParameter custom_mqtt_port("mqtt.port", "MQTT port",
                                        settings.MQTT_PORT, 10);
  wifiManager.addParameter(&custom_mqtt_port);

  WiFiManagerParameter custom_mqtt_topic("mqtt.topic", "MQTT topic",
                                         settings.MQTT_TOPIC, 25);
  wifiManager.addParameter(&custom_mqtt_topic);

  WiFiManagerParameter custom_mqtt_username("mqtt.username", "MQTT username",
                                            settings.MQTT_USERNAME, 50);
  wifiManager.addParameter(&custom_mqtt_username);

  WiFiManagerParameter custom_mqtt_password("mqtt.password", "MQTT password",
                                            settings.MQTT_PASSWORD, 50);
  wifiManager.addParameter(&custom_mqtt_password);

  // reset saved settings
  // wifiManager.resetSettings();

  // fetches ssid and pass from eeprom and tries to connect
  // if it does not connect it starts an access point with the specified name
  // here  "AutoConnectAP"
  // and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect(settings.IOT_ID)) {
    Serial.println(
        "Failed to connect to Wifi and hit timeout, going to sleep.");
    // reset and try again, or maybe put it to deep sleep
    digitalWrite(LED_BUILTIN, LOW); // make sure its off during sleep.
    ESP.deepSleep(0);
  }
  // or use this for auto generated name ESP + ChipID
  // wifiManager.autoConnect();

  // if you get here you have connected to the WiFi
  Serial.println("connected... ");

  // make a MQTT connection
  strcpy(settings.MQTT_SERVER, custom_mqtt_server.getValue());
  strcpy(settings.MQTT_PORT, custom_mqtt_port.getValue());
  strcpy(settings.MQTT_USERNAME, custom_mqtt_username.getValue());
  strcpy(settings.MQTT_PASSWORD, custom_mqtt_password.getValue());
  strcpy(settings.MQTT_TOPIC, custom_mqtt_topic.getValue());


  client = std::shared_ptr<Adafruit_MQTT_Client>(new Adafruit_MQTT_Client(
      &espClient, settings.MQTT_SERVER, atoi(settings.MQTT_PORT), settings.MQTT_USERNAME, settings.MQTT_PASSWORD));
  topic = std::shared_ptr<Adafruit_MQTT_Publish>(
      new Adafruit_MQTT_Publish(&*client, settings.MQTT_TOPIC));

  randomSeed(micros());

  #ifdef IOTFOOTBALL_OTA

  Serial.println("OTA updates ENABLED.");
  ArduinoOTA.onStart([]() { Serial.println("Start updating flash via OTA."); });
  ArduinoOTA.onEnd([]() { Serial.println("\nEnd"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }

    ESP.restart();
  });
  ArduinoOTA.begin();

  #else
  Serial.println("OTA updates DISABLED.");
  #endif

  state = STATE_NORMAL;

  // wifi startup done
  ticker.detach();
  
  //test connections
  WiFiClient client;
  if(client.connect(TEST_NETWORK_HOST, TEST_NETWORK_HOST_PORT)) {
    Serial.printf("Network Test OK [%d]\n", client.status());
  } else {
    Serial.printf("Network Test FAILED [%d]\n", client.status());
    state = STATE_ERROR;
  }
  client.stop();
  
  if(client.connect(settings.MQTT_SERVER, atoi(settings.MQTT_PORT))) {
    Serial.printf("MQTT Connection Test OK [%d]\n", client.status());
  } else {
    Serial.printf("MQTT Connection Test FAILED [%d]\n", client.status());
    state = STATE_ERROR;
  }
  client.stop();

  digitalWrite(LED_BUILTIN, LOW);
}

void setup_sensors() {
  Serial.println("Accelerometer Initialization...!");
  Serial.println("");

  /* Initialise the sensor */
  if (!accel.begin()) {
    /* There was a problem detecting the ADXL345 ... check your connections */
    Serial.println("Ooops, no ADXL345 detected ... Check your wiring!");
    while (1) {
    }
  }


  //accel.writeRegister(ADXL345_REG_BW_RATE, ADXL345_DATARATE_0_10_HZ);
  accel.writeRegister(ADXL345_REG_OFSY, 0x07);
  accel.writeRegister(ADXL345_REG_OFSX, 0x08);
  accel.writeRegister(ADXL345_REG_OFSZ, 0x02);

  /* Set the range to whatever is appropriate for your project */
  accel.setRange(ADXL345_RANGE_4_G);
  // displaySetRange(ADXL345_RANGE_8_G);
  // displaySetRange(ADXL345_RANGE_4_G);
  // displaySetRange(ADXL345_RANGE_2_G);

  // disable the interrupt till we are configured
  accel.writeRegister(ADXL345_REG_INT_ENABLE, 0x00);
  accel.writeRegister(ADXL345_REG_INT_MAP, B10000011);

  // configure motion thresholds
  accel.writeRegister(ADXL345_REG_THRESH_ACT,
                      0x3C); // 4x62mg, default factor is 62mg/LSB
  accel.writeRegister(
      ADXL345_REG_THRESH_INACT,
      0x3C); // 4x62mg < of no motion under ADXL345_REG_THRESH_ACT
  accel.writeRegister(
      ADXL345_REG_TIME_INACT,
      0xF0); // 240 sec of no motion under ADXL345_REG_THRESH_ACT
  accel.writeRegister(ADXL345_REG_ACT_INACT_CTL,
                      0xFF); // enable activity and inactivity thesholds
  // accel.writeRegister(ADXL345_REG_POWER_CTL, B00010000);  //auto sleep after
  // inactivity thresholds

  // configure tap/knock thresholds
  accel.writeRegister(ADXL345_REG_TAP_AXES, B00001111); // enable tap/knock detection
  accel.writeRegister(ADXL345_REG_THRESH_TAP,
                      0x64); // 100mg, or greater = a tap/knock
  accel.writeRegister(
      ADXL345_REG_DUR,
      0x0F); // need to be greater than 625 μs/LSB to count as a knock
  // accel.writeRegister(ADXL345_REG_LATENT, 0x10); // min dealy between taps
  // 2ms accel.writeRegister(ADXL345_REG_WINDOW, 0x64); // allow up to 150ms for
  // multipul tap (aka double tap)

  // free fall
  accel.writeRegister(ADXL345_REG_THRESH_FF,
                      0x05); // 300mg, anything above will be FF
  accel.writeRegister(ADXL345_REG_TIME_FF,
                      0x14); // 200ms of FF till interrupt is notified

  // setup the FIFO queue
  accel.writeRegister(
      ADXL345_REG_FIFO_CTL,
      B10000101); // trigger when there are at least 5 samples in the FIFO

  // enable interrupts
  accel.writeRegister(ADXL345_REG_INT_ENABLE, B01011100);

  /* Display some basic information on this sensor */
  displaySensorDetails();

  /* Display additional settings (outside the scope of sensor_t) */
  displayDataRate();
  displayRange();

  // make sure it is low.
  digitalWrite(ADXL345_IRQ, LOW);

  // callback for the ADXL345 sensor.
  attachInterrupt(ADXL345_IRQ, sensorReadADXL, RISING);

  // displayAllSensorRegister();
  Serial.println("Accelerometer Enabled....!");
}

void configure(WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  // if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  // entered config mode, make led toggle faster
  ticker.attach(0.2, tick);
}

// *** Logic Funvtions *** //

/*
 * Sleep for an hour, pwr down.
 */
void sleepSensors() {
  Serial.println("Sleeping ADXL345 and clearing IRQ.");
  // disable the interrupt till we are configured
  accel.writeRegister(ADXL345_REG_INT_ENABLE, 0x00);
  accel.writeRegister(ADXL345_REG_INT_MAP, B10000011);

  digitalWrite(ADXL345_IRQ, LOW);

  // process the queue to ensure all the data is on the wire
  processQueue();

  Serial.println("Sleeping MQTT connections.");
  if (client->connected()) {
    client->disconnect();
  }
  Serial.println("Sleeping WiFi connection.");
  WiFi.disconnect(false);
  digitalWrite(LED_BUILTIN, LOW); // making it blink during read
  state = STATE_SLEEP;
}

/**
 * Read any RFID tags.
 */
void sensorReadRFID() {
  uint8_t success;
  uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0}; // Buffer to store the returned UID
  uint8_t uidLength; // Length of the UID (4 or 7 bytes depending on ISO14443A
                     // card type)

  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
  success =
      nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 100);

  if (success) {
    digitalWrite(LED_BUILTIN, LOW); // making it blink during read
    // Display some basic information about the card
    #ifdef IOTFOOTBALL_DEBUG
    Serial.println("Found an ISO14443A card");
    Serial.print("  UID Length: ");
    Serial.print(uidLength, DEC);
    Serial.println(" bytes");
    Serial.print("  UID Value: ");
    nfc.PrintHex(uid, uidLength);
    Serial.println("");
    #endif

    digitalWrite(LED_BUILTIN, HIGH); // making it blink during read
  }

  // do we need to update last tracked.
  if (success && ((sizeof(uid) != sizeof(last_uid)) ||
                  (!memcmp(uid, last_uid, sizeof(uid)) == 0))) {
    // store the old id
    // memcpy(prev_uid, last_uid, 7);
    // prev_uid_update = millis();

    // whats the current id
    memcpy(last_uid, uid, 7);
    last_uid_update = millis();
    Serial.println("Updating last read rfid_tag uid.");

    StaticJsonBuffer<512> jsonBuffer;
    JsonObject &data = jsonBuffer.createObject();

    String tag = hex_str(last_uid, sizeof(last_uid));
    Serial.print("Tag: ");
    Serial.println(tag);

    // set the dcsvid to the active rfid_tag hash
    data["dcsvid"] = hash_string(tag.c_str());
    // data["wt.vt_sid"] = settings.IOT_ID;
    data.set<String>("movement", "tag_read");
    data.set<String>("rfid_tag", tag);

    String data_buffer = "";
    data.printTo(data_buffer);

    queue.push(data_buffer);

  } else if (success) {
    // just update the last time it was read, tag did not change.
    last_uid_update = millis();
  }
}

/**
 * Read ADXL movement data
 */

void sensorReadADXL() {
  byte source = accel.readRegister(ADXL345_REG_INT_SOURCE);

  if (((source >> 6) & 1) || ((source >> 4) & 1) || ((source >> 3) & 1) ||
        ((source >> 2) & 1)) {
    digitalWrite(LED_BUILTIN, LOW); // making it blink during read
    StaticJsonBuffer<256> jsonBuffer;
    JsonObject &data = jsonBuffer.createObject();

    // set the dcsvid to the active rfid_tag hash
    String rfid_tag = hex_str(last_uid, sizeof(last_uid));
    data["dcsvid"] = hash_string(rfid_tag.c_str());
    // data["wt.vt_sid"] = settings.IOT_ID;

    if ((millis() - last_uid_update) <=
        30000) // if the read is within 30 sec add to event
    {
      data["rfid_tag"] = rfid_tag;
    }
    // if ((millis() - prev_uid_update) <= 5000)             //if the read is
    // within 30 sec add to event
    // {
    //      data["rfid_tag_source"] = hex_str(prev_uid, sizeof(prev_uid));
    // }

    // add the current millis to the generated event.
    long mtime = millis();
    data["m_time"] = mtime;

    // detected actions
    String movement = "";
    if ((source >> 6) & 1) // tap/knock
    {
      movement += (movement.length() == 0 ? "" : ";");
      movement += "knock";
    } else if ((source >> 2) & 1) // freefall
    {
      movement += (movement.length() == 0 ? "" : ";");
      movement += "freefall";
    } else if ((source >> 4) & 1) // activity
    {
      movement += (movement.length() == 0 ? "" : ";");
      movement += "activity";
    } else if ((source >> 3) & 1) // inactivity
    {
      movement += (movement.length() == 0 ? "" : ";");
      movement += "inactivity";
    }

    data["movement"] = movement;

    /* Get a new sensor event */
    sensors_event_t event;
    accel.getEvent(&event);

    // set the x,y,z
    int x = abs(event.acceleration.x);
    int y = abs(event.acceleration.y);
    int z = abs(event.acceleration.z);

    // data["movement_source"] = byte_str(source);
    data["m_x"] = x;
    data["m_y"] = y;
    data["m_z"] = z;

    #ifdef IOTFOOTBALL_TRACE
    Serial.printf("%lu,%d,%d,%d\n", mtime, x, y, z);
    #endif

    String data_buffer = "";
    data.printTo(data_buffer);

    queue.push(data_buffer);

    #ifdef IOTFOOTBALL_DEBUG
    Serial.print("Source: ");
    print_byte(source);
    Serial.print(" ");
    Serial.print("X: ");
    Serial.print(x);
    Serial.print(" ");
    Serial.print("Y: ");
    Serial.print(y);
    Serial.print(" ");
    Serial.print("Z: ");
    Serial.print(z);
    Serial.print(" ");
    Serial.println("m/s^2 ");
    #endif

    source = accel.readRegister(ADXL345_REG_INT_SOURCE);
    digitalWrite(LED_BUILTIN, HIGH); // making it blink during read
  }

  // displayAllSensorRegister();
  digitalWrite(ADXL345_IRQ, LOW);

  #ifdef IOTFOOTBALL_ALLOW_SLEEP
  if (inactive) {
    state = STATE_YAWN;
  }
  #endif
}

/**
 * Reconnect to the MQTT server.  We dont want to block the main queue.
 */
void reconnect() {
  // Loop until we're reconnected
  while (!client->connected() && state == STATE_NORMAL) {
    Serial.printf("Attempting MQTT connection... %s:%s", settings.MQTT_SERVER,
                  settings.MQTT_PORT);
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += ESP.getChipId();
    int8_t ret;
    // Attempt to connect
    if ((ret = client->connect(settings.MQTT_USERNAME, settings.MQTT_PASSWORD)) == 0) {
      Serial.println(".. connected");
      StaticJsonBuffer<512> jsonBuffer;
      JsonObject &data = jsonBuffer.createObject();
      // Once connected, publish an announcement...
      data["sensor_state"] = "reconnect";
      data["sensor_id"] = clientId;
      char data_buffer[512];
      data.printTo(data_buffer, sizeof(data_buffer));
      topic->publish(data_buffer);      
    } else {
      Serial.println(" ");
      Serial.println(client->connectErrorString(ret));
      client->disconnect();
      Serial.println(" ... will try again in in a bit.");
      // Wait 1 seconds before retrying
      delay(1000);
    }
  }
}

/**
 * Publish data to MQTT,
 */
void publish(const char *data) {
  if (!client->connected()) {
    reconnect();
  }
  if (data && client->connected() && (state == STATE_NORMAL || state == STATE_YAWN)) {
    
    #ifdef IOTFOOTBALL_INFO
    Serial.print("Publish Data: ");
    Serial.println(data);
    #endif
    
    digitalWrite(LED_BUILTIN,
                 LOW); // Turn the LED on (Note that LOW is the voltage level
    if (data) {
      char data_buffer[strlen(data)];
      strcpy(data_buffer, data);
      topic->publish(data_buffer);
    }
  }
}

/**
 * read from the Queue if we been offline, and publish
 */
void processQueue() {
  while (!queue.isEmpty()) {
    digitalWrite(LED_BUILTIN,
                 LOW); // Turn the LED on (Note that LOW is the voltage level
    String data = queue.pop();
    publish(data.c_str());
    digitalWrite(LED_BUILTIN, HIGH); // making it blink during read
  }
}

// *** Supporting Functions *** //

/**************************************************************************/

/*!
 *  @brief  Prints a hexadecimal value in plain characters
 *
 *  @param  data      Pointer to the byte data
 *  @param  numBytes  Data length in bytes
 */
/**************************************************************************/
String hex_str(const byte *data, const uint32_t numBytes) {
  uint32_t szPos;
  String hexStr = "";

  for (szPos = 0; szPos < numBytes; szPos++) {
    hexStr += F("0x");
    // Append leading 0 for small values
    if (data[szPos] <= 0xF) {
      hexStr += F("0");
    }
    hexStr += String(data[szPos] & 0xff, HEX);
    if ((numBytes > 1) && (szPos != numBytes - 1)) {
      hexStr += F(" ");
    }
  }
  return hexStr;
}

/********************************************************************/
/*                                                                  */
// Print Register Values to Serial Output =
// Can be used to Manually Check the Current Configuration of Device

void print_byte(byte val) {
  int i;

  Serial.print("B");
  for (i = 7; i >= 0; i--) {
    Serial.print(val >> i & 1, BIN);
  }
}

String byte_str(byte val) {
  int i;
  String value = F("B");

  for (i = 7; i >= 0; i--) {
    byte bit = (val >> i & 1);
    value += bit;
  }
  return value;
}

inline size_t hash_string(const char *__s) {
  unsigned long __h = 0;
  for (; *__s; ++__s)
    __h = 5 * __h + *__s;
  return size_t(__h);
}

void displayAllSensorRegister() {
  Serial.print("0x00: ");
  byte _b = accel.readRegister(0x00);
  print_byte(_b);
  Serial.println("");
  int i;
  for (i = 29; i <= 57; i++) {
    Serial.print("0x");
    Serial.print(i, HEX);
    Serial.print(": ");
    print_byte(accel.readRegister(i));
    Serial.println("");
  }
}

void merge(JsonObject &dest, JsonObject &src) {
  for (auto kvp : src) {
    dest[kvp.key] = kvp.value;
  }
}

void displaySensorDetails(void) {
  sensor_t sensor;

  accel.getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.print("Sensor:       ");
  Serial.println(sensor.name);
  Serial.print("Driver Ver:   ");
  Serial.println(sensor.version);
  Serial.print("Unique ID:    ");
  Serial.println(sensor.sensor_id);
  Serial.print("Max Value:    ");
  Serial.print(sensor.max_value);
  Serial.println(" m/s^2");
  Serial.print("Min Value:    ");
  Serial.print(sensor.min_value);
  Serial.println(" m/s^2");
  Serial.print("Resolution:   ");
  Serial.print(sensor.resolution);
  Serial.println(" m/s^2");
  Serial.println("------------------------------------");
  Serial.println("");
  delay(500);
}

void displayDataRate(void) {
  Serial.print("Data Rate:    ");

  switch (accel.getDataRate()) {
  case ADXL345_DATARATE_3200_HZ:
    Serial.print("3200 ");
    break;

  case ADXL345_DATARATE_1600_HZ:
    Serial.print("1600 ");
    break;

  case ADXL345_DATARATE_800_HZ:
    Serial.print("800 ");
    break;

  case ADXL345_DATARATE_400_HZ:
    Serial.print("400 ");
    break;

  case ADXL345_DATARATE_200_HZ:
    Serial.print("200 ");
    break;

  case ADXL345_DATARATE_100_HZ:
    Serial.print("100 ");
    break;

  case ADXL345_DATARATE_50_HZ:
    Serial.print("50 ");
    break;

  case ADXL345_DATARATE_25_HZ:
    Serial.print("25 ");
    break;

  case ADXL345_DATARATE_12_5_HZ:
    Serial.print("12.5 ");
    break;

  case ADXL345_DATARATE_6_25HZ:
    Serial.print("6.25 ");
    break;

  case ADXL345_DATARATE_3_13_HZ:
    Serial.print("3.13 ");
    break;

  case ADXL345_DATARATE_1_56_HZ:
    Serial.print("1.56 ");
    break;

  case ADXL345_DATARATE_0_78_HZ:
    Serial.print("0.78 ");
    break;

  case ADXL345_DATARATE_0_39_HZ:
    Serial.print("0.39 ");
    break;

  case ADXL345_DATARATE_0_20_HZ:
    Serial.print("0.20 ");
    break;

  case ADXL345_DATARATE_0_10_HZ:
    Serial.print("0.10 ");
    break;

  default:
    Serial.print("???? ");
    break;
  }
  Serial.println(" Hz");
}

void displayRange(void) {
  Serial.print("Range:         +/- ");

  switch (accel.getRange()) {
  case ADXL345_RANGE_16_G:
    Serial.print("16 ");
    break;

  case ADXL345_RANGE_8_G:
    Serial.print("8 ");
    break;

  case ADXL345_RANGE_4_G:
    Serial.print("4 ");
    break;

  case ADXL345_RANGE_2_G:
    Serial.print("2 ");
    break;

  default:
    Serial.print("?? ");
    break;
  }
  Serial.println(" g");
}

void tick() {
  // toggle state
  int state = digitalRead(LED_BUILTIN); // get the current state of GPIO1 pin
  digitalWrite(LED_BUILTIN, !state);    // set pin to the opposite state
}
