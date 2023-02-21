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

#include <Adafruit_SleepyDog.h>
#include <SoftwareSerial.h>
#include "Adafruit_FONA.h"
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_FONA.h"
#include "DHT.h"

// Default pins for Feather 32u4 FONA
#define FONA_RX  9 
#define FONA_TX  8 
#define FONA_RST 4

SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);

#define FONA_APN       ""
#define FONA_USERNAME  ""
#define FONA_PASSWORD  ""

#define MQTT_SERVER      "server_address.or.ip"
#define MQTT_SERVERPORT  1883
#define MQTT_USERNAME    "server_username"
#define MQTT_KEY         "server_key.or.password"

#define DHTPIN 7
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#define DSM_PM10 5 // Perlu pin PWM
#define DSM_PM25 6 // Perlu pin PWM

/************ Global State (you don't need to change this!) ******************/

Adafruit_MQTT_FONA mqtt(&fona, MQTT_SERVER, MQTT_SERVERPORT, MQTT_USERNAME, MQTT_KEY);

// You don't need to change anything below this line!
#define halt(s) { Serial.println(F( s )); while(1);  }

// FONAconnect is a helper function that sets up the FONA and connects to
// the GPRS network. See the fonahelper.cpp tab above for the source!
boolean FONAconnect(const __FlashStringHelper *apn, const __FlashStringHelper *username, const __FlashStringHelper *password);

Adafruit_MQTT_Publish sensor_dht_temp = Adafruit_MQTT_Publish(&mqtt, MQTT_USERNAME "/topic/sensor_dht_temp");
Adafruit_MQTT_Publish sensor_dht_humidity = Adafruit_MQTT_Publish(&mqtt, MQTT_USERNAME "/topic/sensor_dht_humidity");
Adafruit_MQTT_Publish sensor_dsm_pm10 = Adafruit_MQTT_Publish(&mqtt, MQTT_USERNAME "/topic/sensor_dsm_pm10");
Adafruit_MQTT_Publish sensor_dsm_pm25 = Adafruit_MQTT_Publish(&mqtt, MQTT_USERNAME "/topic/sensor_dsm_pm25");


/*************************** Sketch Code ************************************/

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

void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000);  // wait 5 seconds
  }
  Serial.println("MQTT Connected!");
}

void setup() {
  while (!Serial);

  // Watchdog is optional!
  // Watchdog.enable(8000);
  dht.begin();
  Serial.begin(115200);
  pinMode(DSM_PM10, INPUT);
  pinMode(DSM_PM25, INPUT);
  delay(10);

  Serial.println(F("Adafruit FONA MQTT."));

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

  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

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

    Serial.print(F("\nSending data..."));
    if (! sensor_dht_temp.publish(temp)) {
      Serial.println(F("Publish temp failed."));
    } else {
      Serial.println(F("Data temp OK!"));
    }

    if (! sensor_dht_humidity.publish(humidity)) {
      Serial.println(F("Publish humidity failed."));
    } else {
      Serial.println(F("Data humidirty OK!"));
    }

    if (! sensor_dsm_pm10.publish(conPM10)) {
      Serial.println(F("Publish pm10 failed."));
    } else {
      Serial.println(F("Data pm10 OK!"));
    }

    if (! sensor_dsm_pm25.publish(conPM25)) {
      Serial.println(F("Publish pm25 failed."));
    } else {
      Serial.println(F("Data pm2.5 OK!"));
    }

    previous_millis = current_millis;
  }

  Watchdog.reset();

  // ping the server to keep the mqtt connection alive, only needed if we're not publishing
  //if(! mqtt.ping()) {
  //  Serial.println(F("MQTT Ping failed."));
  //}
}
