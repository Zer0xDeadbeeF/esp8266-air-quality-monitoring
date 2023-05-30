/***************************************************
  Adafruit MQTT Library FONA Example

  Designed specifically to work with the Adafruit FONA
  ----> http://www.adafruit.com/products/1946
  ----> http://www.adafruit.com/products/1963
  ----> http://www.adafruit.com/products/2468
  ----> http://www.adafruit.com/products/2542

  These cellular modules use TTL Serial to communicate, 2 pins are
  required to interface.

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/

// Modified by Yoshikuni
// For educational purpose only

#include <Adafruit_Sensor.h>
#include <Adafruit_SleepyDog.h>
#include <SoftwareSerial.h>
#include "Adafruit_FONA.h"
//#include "Adafruit_MQTT.h"
//#include "Adafruit_MQTT_FONA.h"
#include <DHT.h>
#include <DHT_U.h>

#define FONA_TX  0 // D3
#define FONA_RX  2 // D4 
#define FONA_RST 4

SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);

// Beda kartu provider, beda lagi APN
#define FONA_APN       "internet"
#define FONA_USERNAME  "3gprs"
#define FONA_PASSWORD  "3gprs"

#define MQTT_SERVER      "url/ip address server"
#define MQTT_SERVERPORT  1883 // Default MQTT PORT
#define MQTT_USERNAME    "username"
#define MQTT_KEY         "root"

#define DHTPIN D7
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#define DSM_PM10 D5 // Perlu pin PWM
#define DSM_PM25 D6 // Perlu pin PWM

// You don't need to change anything below this line!
#define halt(s) { Serial.println(F( s )); while(1);  }

// FONAconnect is a helper function that sets up the FONA and connects to
// the GPRS network. See the fonahelper.cpp tab above for the source!
boolean FONAconnect(const __FlashStringHelper *apn, const __FlashStringHelper *username, const __FlashStringHelper *password);

// How many transmission failures in a row we're willing to be ok with before reset
unsigned long previous_millis = 0;
unsigned long interval_time = 2000; // 2000ms berarti 2 detik

// variabel untuk pembacaan sensor dsm501
byte buff[2];
unsigned long durationPM10;
unsigned long durationPM25;
unsigned long lowpulseoccupancyPM10 = 0;
unsigned long lowpulseoccupancyPM25 = 0;

float calculateConcentration(long lowpulseInMicroSeconds, long durationinSeconds){
  float ratio = (lowpulseInMicroSeconds/1000000.0)/30.0*100.0; // kalkulasi rasio
  float concentration = 0.001915 * pow(ratio,2) + 0.09522 * ratio - 0.04884;// kalkulasi ke mg/m^3
  Serial.print("lowpulseoccupancy:");
  Serial.print(lowpulseInMicroSeconds);
  Serial.print("    ratio:");
  Serial.print(ratio);
  Serial.print("    Concentration:");
  Serial.println(concentration);
  return concentration;
}

void setup() {
  while (!Serial);

  // Watchdog is optional!
  // Watchdog.enable(8000);
  dht.begin();
  Serial.begin(9600);
  pinMode(DSM_PM10, INPUT);
  pinMode(DSM_PM25, INPUT);
  delay(10);

  Serial.println(F("Adafruit FONA HTTP."));

  Watchdog.reset();
  delay(5000);  // wait a few seconds to stabilize connection
  Watchdog.reset();
  
  // Initialise the FONA module
  while (! FONAconnect(F(FONA_APN), F(FONA_USERNAME), F(FONA_PASSWORD))) {
    Serial.println("Retrying FONA");
  }

  Serial.println(F("Connected to Cellular!"));

  Watchdog.reset();
  delay(5000);  // wait a few seconds to stabilize connection
  Watchdog.reset();
}

void loop() {
  // Make sure to reset watchdog every loop iteration!
  Watchdog.reset();

  // now we can publish stuff!
  unsigned long current_millis = millis();
  if (current_millis - previous_millis > interval_time) {
    durationPM10 = pulseIn(DSM_PM10, LOW);
    durationPM25 = pulseIn(DSM_PM25, LOW);

    lowpulseoccupancyPM10 += durationPM10;
    lowpulseoccupancyPM25 += durationPM25;

    //calculateConcentration(lowpulseoccupancy,duration (on second))
    float conPM10 = calculateConcentration(lowpulseoccupancyPM10,3);
    float conPM25 = calculateConcentration(lowpulseoccupancyPM25,3);
    Serial.print("PM10 ");
    Serial.print(conPM10);
    Serial.print("  PM2.5 ");
    Serial.println(conPM25);
    lowpulseoccupancyPM10 = 0;
    lowpulseoccupancyPM25 = 0;

    // pembacaan sensor suhu atau kelembaban butuh sekitar 250ms
    float temp = dht.readTemperature();
    float humidity = dht.readHumidity();

    // cek apakah sensor berhasil membaca suhu/kelembaban
    if (isnan(humidity) || isnan(temp)) {
      Serial.println(F("Gagal membaca dari sensor DHT!")); 
      // data yang akan dipublish berupa "nan"
      return;
    }

    // post data
    uint16_t statuscode;
    int16_t length;
    char url[80] = "api/v4/mqtt/publish"; // host + "api/v4/mqtt/publish"; 
    char data[80] = "{\"topic\":\"topic_name\",\"payload\":\"some_data\"}";

    Serial.println(F("NOTE: in beta! Use simple websites to post!"));
    Serial.println(F("URL to post (e.g. httpbin.org/post):"));
    Serial.print(F("http://"));
    Serial.println(url);
    Serial.println(F("Data to post (e.g. \"foo\" or \"{\"simple\":\"json\"}\"):"));
    Serial.println(data);

    Serial.println(F("****"));
    if (!fona.HTTP_POST_start(url, F("application/json"), (uint8_t *) data, strlen(data), &statuscode, (uint16_t *)&length)) {
      Serial.println("Failed!");
    }

    while (length > 0) {
      while (fona.available()) {
        char c = fona.read();
                Serial.write(c);
                length--;
                if (! length) break;
      }
    }
    Serial.println(F("\n****"));
    fona.HTTP_POST_end();

    previous_millis = current_millis;
    }

    Watchdog.reset();
}
