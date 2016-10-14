#include <Football.h>

// Use this line for a breakout with a SPI connection:
//Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);
Adafruit_PN532 nfc(PN532_SS);

/* Assign a unique ID to this sensor at the same time */
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

#if defined(ARDUINO_ARCH_SAMD)
// for Zero, output on USB Serial console, remove line below if using programming port to program the Zero!
// also change #define in Adafruit_PN532.cpp library file
#define Serial    SerialUSB
#endif

QueueList<String> queue;
WiFiClient        espClient;

std::shared_ptr<Adafruit_MQTT_Client>  client = NULL;
std::shared_ptr<Adafruit_MQTT_Publish> topic  = NULL;

uint8_t       last_uid[] = { 0, 0, 0, 0, 0, 0, 0 };
uint8_t       prev_uid[] = { 0, 0, 0, 0, 0, 0, 0 };
unsigned long last_uid_update = 0;
unsigned long prev_uid_update = 0;
unsigned long last_reconnect  = 0;

int state = STATE_UNCONFIGURED;


// *** Supporting Functions *** //

/**************************************************************************/

/*!
 *  @brief  Prints a hexadecimal value in plain characters
 *
 *  @param  data      Pointer to the byte data
 *  @param  numBytes  Data length in bytes
 */
/**************************************************************************/
String hex_str(const byte *data, const uint32_t numBytes)
{
   uint32_t szPos;
   String   hexStr = "";

   for (szPos = 0; szPos < numBytes; szPos++)
   {
      hexStr += F("0x");
      // Append leading 0 for small values
      if (data[szPos] <= 0xF)
      {
         hexStr += F("0");
      }
      hexStr += String(data[szPos] & 0xff, HEX);
      if ((numBytes > 1) && (szPos != numBytes - 1))
      {
         hexStr += F(" ");
      }
   }
   return hexStr;
}


/********************************************************************/
/*                                                                  */
// Print Register Values to Serial Output =
// Can be used to Manually Check the Current Configuration of Device

void print_byte(byte val)
{
   int i;

   Serial.print("B");
   for (i = 7; i >= 0; i--)
   {
      Serial.print(val >> i & 1, BIN);
   }
}


String byte_str(byte val)
{
   int    i;
   String value = F("B");

   for (i = 7; i >= 0; i--)
   {
      byte bit = (val >> i & 1);
      value += bit;
   }
   return value;
}


void displayAllSensorRegister()
{
   Serial.print("0x00: ");
   byte _b = accel.readRegister(0x00);
   print_byte(_b);
   Serial.println("");
   int i;
   for (i = 29; i <= 57; i++)
   {
      Serial.print("0x");
      Serial.print(i, HEX);
      Serial.print(": ");
      print_byte(accel.readRegister(i));
      Serial.println("");
   }
}


void merge(JsonObject& dest, JsonObject& src)
{
   for (auto kvp : src)
   {
      dest[kvp.key] = kvp.value;
   }
}


bool isSleepy()
{
   return state == STATE_SLEEP;
}


/*
 * Sleep for an hour, pwr down.
 */
void sleepNow()
{
   state = STATE_YAWN;

   Serial.println("Sleeping ADXL345 and clearing IRQ.");
   //disable the interrupt till we are configured
   accel.writeRegister(ADXL345_REG_INT_ENABLE, 0x00);
   accel.writeRegister(ADXL345_REG_INT_MAP, B10000011);

   digitalWrite(ADXL345_IRQ, LOW);

   Serial.print("Flush events in MQTT queue.");
   int trys = 0;
   while (!queue.isEmpty() && trys < 10)
   {
      Serial.print(".");
      delay(500);
      trys++;
   }

   Serial.println("");
   Serial.println("Sleeping MQTT connections.");
   if (client->connected())
   {
      client->disconnect();
   }
   Serial.println("Sleeping WiFi connection.");
   WiFi.disconnect(false);
   digitalWrite(BUILTIN_LED, LOW);      //making it blink during read
   state = STATE_SLEEP;
}


/**
 * Read any RFID tags.
 */
void readRFID()
{
   uint8_t success;
   uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 }; // Buffer to store the returned UID
   uint8_t uidLength;                       // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

   // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
   // 'uid' will be populated with the UID, and uidLength will indicate
   // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
   success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 100);

   if (success)
   {
      digitalWrite(BUILTIN_LED, LOW);           //making it blink during read
      // Display some basic information about the card
      Serial.println("Found an ISO14443A card");
      Serial.print("  UID Length: ");
      Serial.print(uidLength, DEC);
      Serial.println(" bytes");
      Serial.print("  UID Value: ");
      nfc.PrintHex(uid, uidLength);

      if (uidLength == 4)
      {
         // We probably have a Mifare Classic card ...
         uint32_t cardid = uid[0];
         cardid <<= 8;
         cardid  |= uid[1];
         cardid <<= 8;
         cardid  |= uid[2];
         cardid <<= 8;
         cardid  |= uid[3];
         Serial.print("Seems to be a Mifare Classic card #");
         Serial.println(cardid);
      }
      Serial.println("");

      digitalWrite(BUILTIN_LED, HIGH);           //making it blink during read
   }

   //do we need to update last tracked.
   if (success && ((sizeof(uid) != sizeof(last_uid)) || (!memcmp(uid, last_uid, sizeof(uid)) == 0)))
   {
     //store the old id
      //memcpy(prev_uid, last_uid, 7);
      //prev_uid_update = millis();

      //whats the current id
      memcpy(last_uid, uid, 7);
      last_uid_update = millis();
      Serial.println("Updating last read rfid_tag uid.");

      StaticJsonBuffer<512> jsonBuffer;
      JsonObject&           data = jsonBuffer.createObject();

      String tag = hex_str(last_uid, sizeof(last_uid));
      Serial.print("Tag: ");
      Serial.println(tag);
      data.set<String>("rfid_tag", tag);
      char data_buffer[512];
      //data.printTo(Serial);
      data.printTo(data_buffer, sizeof(data_buffer));
      queue.push(data_buffer);
   }
   else if (success)
   {
      //just update the last time it was read, tag did not change.
      last_uid_update = millis();
   }
}


void readAccel()
{
   bool inactive = false;
   byte source   = accel.readRegister(ADXL345_REG_INT_SOURCE);

   while (((source >> 6) & 1) || ((source >> 4) & 1) ||
          ((source >> 3) & 1) || ((source >> 2) & 1))
   {
      digitalWrite(BUILTIN_LED, LOW);           //making it blink during read
      StaticJsonBuffer<1024> jsonBuffer;
      JsonObject&            data = jsonBuffer.createObject();
      if ((millis() - last_uid_update) <= 30000)             //if the read is within 30 sec add to event
      {
         data["rfid_tag"] = hex_str(last_uid, sizeof(last_uid));
      }
      // if ((millis() - prev_uid_update) <= 5000)             //if the read is within 30 sec add to event
      // {
      //    data["rfid_tag_prev"] = hex_str(prev_uid, sizeof(prev_uid));
      // }

      //add the current millis to the generated event.
      data["movement_time"] = millis();

      //detected actions
      String movement = "";
      if ((source >> 6) & 1)             //tap/knock
      {
         movement += (movement.length() == 0 ? "" : ";");
         movement += "knock";
      }
      else if ((source >> 2) & 1)             //freefall
      {
         movement += (movement.length() == 0 ? "" : ";");
         movement += "freefall";
      }
      else if ((source >> 4) & 1)        //activity
      {
         movement += (movement.length() == 0 ? "" : ";");
         movement += "activity";
      }
      else if ((source >> 3) & 1)             //inactivity
      {
         movement += (movement.length() == 0 ? "" : ";");
         movement += "inactivity";
         inactive  = true;
      }

      data["movement"] = movement;

      /* Get a new sensor event */
      sensors_event_t event;
      accel.getEvent(&event);

      //set the x,y,z
      int x = abs(event.acceleration.x);
      int y = abs(event.acceleration.y);
      int z = abs(event.acceleration.z);

      //data["movement_source"] = byte_str(source);
      data["movement_x"] = x;
      data["movement_y"] = y;
      data["movement_z"] = z;

      //data.printTo(Serial);

      String data_buffer = "";
      data.printTo(data_buffer);

      queue.push(data_buffer);
      Serial.print("Data Buffer: ");
      Serial.println(data_buffer);
      Serial.print("Payload: ");
      data.printTo(Serial);
      Serial.println("");
      Serial.print("Source: ");
      print_byte(source);
      Serial.println("");
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

      source = accel.readRegister(ADXL345_REG_INT_SOURCE);
      digitalWrite(BUILTIN_LED, HIGH);           //making it blink during read
   }

   //displayAllSensorRegister();
   digitalWrite(ADXL345_IRQ, LOW);

   if (inactive)
   {
      sleepNow();
   }
}


/**
 * Reconnect to the MQTT server.  We dont want to block the main queue.
 */
void reconnect()
{
   // Loop until we're reconnected
   while (!client->connected() && !isSleepy())
   {
      Serial.printf("Attempting MQTT connection... %s:%d", settings.MQTT_SERVER, settings.MQTT_PORT);
      // Create a random client ID
      String clientId = "ESP8266Client-";
      clientId += String(random(0xffff), HEX);
      // Attempt to connect
      if (client->connect(settings.MQTT_USERNAME, settings.MQTT_PASSWORD) == 0)
      {
         Serial.println(".. connected");
         StaticJsonBuffer<512> jsonBuffer;
         JsonObject&           data = jsonBuffer.createObject();
         // Once connected, publish an announcement...
         data["sensor_state"] = "reconnect";
         data["sensor_id"]    = clientId;
         char data_buffer[512];
         data.printTo(data_buffer, sizeof(data_buffer));
         topic->publish(data_buffer);
      }
      else
      {
         Serial.println(" will try again in in a bit.");
         // Wait 5 seconds before retrying
         //delay(5000);
      }
   }
}


/**
 * Publish data to MQTT,
 */
void publish(char *data)
{
   if (!client->connected())
   {
      reconnect();
   }
   if (client->connected() && !isSleepy())
   {
      Serial.print("Publish Data: ");
      Serial.println(data);
      digitalWrite(BUILTIN_LED, LOW);           // Turn the LED on (Note that LOW is the voltage level
      topic->publish(data);
   }
}


/**
 * read from the Queue if we been offline, and publish
 */
void processQueue()
{
   while (!queue.isEmpty())
   {
      digitalWrite(BUILTIN_LED, LOW);           // Turn the LED on (Note that LOW is the voltage level
      String val = queue.pop();
      Serial.print("Process Queue: ");
      Serial.print(val);
      Serial.println("");
      char data[val.length()];
      val.toCharArray(data, val.length() + 1);
      publish(data);
      digitalWrite(BUILTIN_LED, HIGH);           //making it blink during read
   }
}


void displaySensorDetails(void)
{
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


void displayDataRate(void)
{
   Serial.print("Data Rate:    ");

   switch (accel.getDataRate())
   {
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


void displayRange(void)
{
   Serial.print("Range:         +/- ");

   switch (accel.getRange())
   {
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


/**
 * Ensure the RFID board is present.
 */
void setup_rfid()
{
   Serial.println("Initializing PN53x...!");

   nfc.begin();

   uint32_t versiondata = nfc.getFirmwareVersion();
   if (!versiondata)
   {
      Serial.print("Didn't find PN53x board");
      while (1)             // halt
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
 * Make sure all connecti ons to MQTT are correct.
 */
void setup_mqtt()
{
   client = std::shared_ptr<Adafruit_MQTT_Client> (new Adafruit_MQTT_Client(&espClient, settings.MQTT_SERVER,
                                                                            settings.MQTT_PORT, settings.MQTT_USERNAME, settings.MQTT_PASSWORD));
   topic = std::shared_ptr<Adafruit_MQTT_Publish>(new Adafruit_MQTT_Publish(&*client, settings.MQTT_TOPIC));
}


/**
 * Configure WiFi to verify connectivity to the internet.
 */
void setup_wifi()
{
   Serial.println("Initializing WiFi network...!");
   // We start by connecting to a WiFi network
   Serial.println();
   Serial.print("Connecting to ");
   Serial.print(settings.SSID);

   WiFi.begin(settings.SSID, settings.SSID_PASSWORD);
   int connect_trys = 0;
   while (WiFi.status() != WL_CONNECTED && connect_trys < 40) //trying to conect to wifi for 20 seconds
   {
      digitalWrite(BUILTIN_LED, HIGH);
      delay(500);
      Serial.print(".");
      digitalWrite(BUILTIN_LED, LOW);
      delay(500);
      connect_trys++;
   }

   if (WiFi.status() != WL_CONNECTED)
   {
      Serial.println("Network not found, entering configuration mode.");
      digitalWrite(BUILTIN_LED, LOW);
      state = STATE_ERROR;
      return;
   }


   randomSeed(micros());

   ArduinoOTA.onStart([]() {
      Serial.println("Start updating flash via OTA.");
   });
   ArduinoOTA.onEnd([]() {
      Serial.println("\nEnd");
   });
   ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
   });
   ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR)
      {
         Serial.println("Auth Failed");
      }
      else if (error == OTA_BEGIN_ERROR)
      {
         Serial.println("Begin Failed");
      }
      else if (error == OTA_CONNECT_ERROR)
      {
         Serial.println("Connect Failed");
      }
      else if (error == OTA_RECEIVE_ERROR)
      {
         Serial.println("Receive Failed");
      }
      else if (error == OTA_END_ERROR)
      {
         Serial.println("End Failed");
      }
   });
   ArduinoOTA.begin();

   Serial.println("");
   Serial.println("WiFi connected");
   Serial.println("IP address: ");
   Serial.println(WiFi.localIP());


   state = STATE_NORMAL;
}


void setup_sensors()
{
// #ifndef ESP8266
//    while (!Serial)        // for Leonardo/Micro/Zero
//    {
//    }
// #endif

   Serial.println("Accelerometer Initialization...!");
   Serial.println("");

   /* Initialise the sensor */
   if (!accel.begin())
   {
      /* There was a problem detecting the ADXL345 ... check your connections */
      Serial.println("Ooops, no ADXL345 detected ... Check your wiring!");
      while (1)
      {
      }
   }

   /* Set the range to whatever is appropriate for your project */
   accel.setRange(ADXL345_RANGE_16_G);
   // displaySetRange(ADXL345_RANGE_8_G);
   // displaySetRange(ADXL345_RANGE_4_G);
   // displaySetRange(ADXL345_RANGE_2_G);

   //disable the interrupt till we are configured
   accel.writeRegister(ADXL345_REG_INT_ENABLE, 0x00);
   accel.writeRegister(ADXL345_REG_INT_MAP, B10000011);

   //configure motion thresholds
   accel.writeRegister(ADXL345_REG_THRESH_ACT, 0x4B);      //4x62mg, default factor is 62mg/LSB
   accel.writeRegister(ADXL345_REG_THRESH_INACT, 0x4B);    //4x62mg < of no motion under ADXL345_REG_THRESH_ACT
   accel.writeRegister(ADXL345_REG_TIME_INACT, 0xF0);      //240 sec of no motion under ADXL345_REG_THRESH_ACT
   accel.writeRegister(ADXL345_REG_ACT_INACT_CTL, 0xFF);   //enable activity and inactivity thesholds
   //accel.writeRegister(ADXL345_REG_POWER_CTL, B00010000);  //auto sleep after inactivity thresholds

   //configure tap/knock thresholds
   accel.writeRegister(ADXL345_REG_TAP_AXES, 0x21);   //enable tap/knock detection
   accel.writeRegister(ADXL345_REG_THRESH_TAP, 0x32); //100mg, or greater = a tap/knock
   accel.writeRegister(ADXL345_REG_DUR, 0x0F);        //need to be greater than 625 μs/LSB to count as a knock
   //accel.writeRegister(ADXL345_REG_LATENT, 0x10); // min dealy between taps 2ms
   //accel.writeRegister(ADXL345_REG_WINDOW, 0x64); // allow up to 150ms for multipul tap (aka double tap)

   //free fall
   accel.writeRegister(ADXL345_REG_THRESH_FF, 0x05);    //300mg, anything above will be FF
   accel.writeRegister(ADXL345_REG_TIME_FF, 0x14);      //200ms of FF till interrupt is notified

   //setup the FIFO queue
   accel.writeRegister(ADXL345_REG_FIFO_CTL, B10000101);      //trigger when there are at least 5 samples in the FIFO

   //enable interrupts
   accel.writeRegister(ADXL345_REG_INT_ENABLE, B01011100);

   /* Display some basic information on this sensor */
   displaySensorDetails();

   /* Display additional settings (outside the scope of sensor_t) */
   displayDataRate();
   displayRange();

   //make sure it is low.
   digitalWrite(ADXL345_IRQ, LOW);

   //callback for the ADXL345 sensor.
   attachInterrupt(ADXL345_IRQ, readAccel, RISING);

   //displayAllSensorRegister();
   Serial.println("Accelerometer Enabled....!");
}


void saveEEPROM()
{
   Serial.print("Saving EEPROM configuration.");
   for (unsigned int t = 0; t < sizeof(settings); t++)
   {      // writes to EEPROM
      EEPROM.write(CONFIG_START + t, *((char *)&settings + t));
      // and verifies the data
      if (EEPROM.read(CONFIG_START + t) != *((char *)&settings + t))
      {
         Serial.print("X");
         // error writing to EEPROM
      }
      else
      {
         Serial.print(".");
      }
   }
   Serial.println("");
   EEPROM.commit();
}


bool loadEEPROM()
{
   EEPROM.begin(512);

   delay(100);
   Serial.print("Loading EEPROM configuration.");

   // To make sure there are settings, and they are YOURS!
   // If nothing is found it will use the default settings.
   if (     //EEPROM.read(CONFIG_START + sizeof(settings) - 1) == settings.version_of_program[3] // this is '\0'
      (EEPROM.read(CONFIG_START + sizeof(settings) - 2) == settings.version_of_program[2]) &&
      (EEPROM.read(CONFIG_START + sizeof(settings) - 3) == settings.version_of_program[1]) &&
      (EEPROM.read(CONFIG_START + sizeof(settings) - 4) == settings.version_of_program[0]))
   {      // reads settings from EEPROM
      for (unsigned int t = 0; t < sizeof(settings); t++)
      {
         Serial.print(".");
         *((char *)&settings + t) = EEPROM.read(CONFIG_START + t);
      }
   }
   else
   {
      Serial.println("..... Error loading settings from EEPROM, using defaults.");
      return false;
   }
   Serial.println("Done");
   return true;
}


void configure(int timeout = 5000)
{
   bool loaded = loadEEPROM();

   if (state == STATE_ERROR)
   {
      uint8_t success;
      uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 }; // Buffer to store the returned UID
      uint8_t uidLength;                       // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

      // Wait for an NTAG203 card.  When one is found 'uid' will be populated with
      // the UID, and uidLength will indicate the size of the UUID (normally 7)
      Serial.printf("Configuration sideload, looking for NFC tag. (will continue in %d seconds)", timeout / 1000);
      int timeout_count = 0;
      while (!success && timeout_count < timeout / 1000)
      {
         digitalWrite(BUILTIN_LED, HIGH); //make sure its off during init.
         success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 500);
         Serial.print(".");
         digitalWrite(BUILTIN_LED, LOW); //make sure its off during init.
         delay(500);
         timeout_count++;
      }
      digitalWrite(BUILTIN_LED, LOW); //make sure its off during init.
      Serial.println("");

      if (success)
      {
         // Display some basic information about the card
         Serial.println("Found an ISO14443A card, checking for configuration data.");
         Serial.print("  UID Length: ");
         Serial.print(uidLength, DEC);
         Serial.println(" bytes");
         Serial.print("  UID Value: ");
         nfc.PrintHex(uid, uidLength);
         Serial.println("");

         if (uidLength == 7)
         {
            uint8_t data[32];

            // We probably have an NTAG2xx card (though it could be Ultralight as well)
            Serial.println("Seems to be an NTAG2xx tag (7 byte UID)");

            // NTAG2x3 cards have 39*4 bytes of user pages (156 user bytes),
            // starting at page 4 ... larger cards just add pages to the end of
            // this range:

            // See: http://www.nxp.com/documents/short_data_sheet/NTAG203_SDS.pdf

            // TAG Type       PAGES   USER START    USER STOP
            // --------       -----   ----------    ---------
            // NTAG 203       42      4             39
            // NTAG 213       45      4             39
            // NTAG 215       135     4             129
            // NTAG 216       231     4             225

            for (uint8_t i = 0; i < 42; i++)
            {
               success = nfc.ntag2xx_ReadPage(i, data);

               // Display the current page number
               Serial.print("PAGE ");
               if (i < 10)
               {
                  Serial.print("0");
                  Serial.print(i);
               }
               else
               {
                  Serial.print(i);
               }
               Serial.print(": ");

               // Display the results, depending on 'success'
               if (success)
               {
                  // Dump the page data
                  nfc.PrintHexChar(data, 4);
                  Serial.print((char *)data);
               }
               else
               {
                  Serial.println("Unable to read the requested page!");
               }
            }
         }
         else
         {
            Serial.println("This doesn't seem to be an NTAG203 tag (UUID length != 7 bytes)!");
         }
      }
      else
      {
         if (!loaded)
         {
            //saveEEPROM();
         }

         Serial.println("Using current EEPROM configuration.");
      }
   }
   //close out the EEPROM
   EEPROM.end();
  }


/**
 * Setup the connections and chips
 */
void setup()
{
   pinMode(BUILTIN_LED, OUTPUT);      // Initialize the BUILTIN_LED pin as an output

   Serial.begin(115200);
   delay(500);
   Serial.println("");
   Serial.println("");
   Serial.println("");
   Serial.println("Webtrends IoT Football Demo - 2016 <peter.crossley@webtrends.com>");

   //get all configuration and allocations from EEPROM
   configure();

   //Setup components
   setup_wifi();

   if (state == STATE_NORMAL)
   {
      setup_rfid();
      setup_mqtt();
      setup_sensors();

      digitalWrite(BUILTIN_LED, HIGH);   //make sure its off during init.
   }

   //setup the queue
   queue.setPrinter(Serial);
}


/**
 * Main process loop
 */
void loop()
{
   if (state == STATE_SLEEP) //sleepy
   {
      Serial.println("Sleeping ESP8266 in 5 seconds.");
      delay(5000);
      digitalWrite(BUILTIN_LED, LOW);           //make sure its off during sleep.
      ESP.deepSleep(0);
   }

   if (state == STATE_ERROR)
   {
      setup_rfid();
      configure(20000);  //wait 20 sec
      ESP.restart();
   }

   if (state == STATE_NORMAL)
   {
      digitalWrite(BUILTIN_LED, HIGH);     //make sure its off during init.
      //handle rfid in the main loop
      readRFID();
   }
   if ((state == STATE_NORMAL) || (state == STATE_YAWN))
   {
   //process the queue to ensure all the data is on the wire
    processQueue();
   }

   //prevent lockups
   yield();
}
