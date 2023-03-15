/*******************************************************************************************************
 * Ini kode untuk alat mikrokontroler mengirimkan data dari sensor menggunakan WiFi. 
 * Kode diperuntukan pada mikrokontroler esp8266.
 * 
 * Sensor yang digunakan : DSM501A dan DHT22
 * 
 * Credit : Yoshikuni
 * *****************************************************************************************************/

#include <ESP8266WiFi.h>
#include "DHT.h"
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

/****************** WiFi Access Point *******************************************************************/

#define WLAN_SSID  "Nama WiFi kamu"
#define WLAN_PASS   "Password WiFi kamu"

/****************** MQTT Broker Detail ******************************************************************/

#define MQTT_SERVER "url/ip address server"
#define MQTT_PORT 1883  // default server biasanya 1883
#define MQTT_UNAME  "username"
#define MQTT_PASS "key/password"

/***** Global State (bagian ini ga perlu diubah) ********************************************************/

// Ngebuat ESP8266 WiFiClient class buat konek ke broker mqtt cloudnya
WiFiClient client;
// atau.... pake WiFiClientSecure buat SSL
//WiFiClientSecure client;

// Setup client MQTT class
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, MQTT_PORT, MQTT_UNAME, MQTT_PASS);

/************************ Initial Setup Sensor and MQTT client ******************************************/
// Define tipe DHT (sensor temperatur dan kelembaban)
#define DHTPIN D7
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// detail DSM501
#define DSM501_PM1 D5
#define DSM501_PM25 D6

Adafruit_MQTT_Publish sensor_dht_temp = Adafruit_MQTT_Publish(&mqtt, MQTT_UNAME "/topic/sensor_dht_temp");
Adafruit_MQTT_Publish sensor_dht_humidity = Adafruit_MQTT_Publish(&mqtt, MQTT_UNAME "/topic/sensor_dht_humidity");
Adafruit_MQTT_Publish sensor_dsm_pm10 = Adafruit_MQTT_Publish(&mqtt, MQTT_UNAME "/topic/sensor_dsm_pm10");
Adafruit_MQTT_Publish sensor_dsm_pm25 = Adafruit_MQTT_Publish(&mqtt, MQTT_UNAME "/topic/sensor_dsm_pm25");

/******************** Main Code *************************************************************************/

unsigned long previous_millis = 0;
unsigned long interval_time = 2000; // 2000ms berarti 2 detik

// variabel untuk pembacaan sensor dsm501
byte buff[2];
unsigned long durationPM1;
unsigned long durationPM25;
unsigned long lowpulseoccupancyPM1 = 0;
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

  // Kalo udah connected, yaudah hehe
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT...");

  uint8_t retries = 3; // Maksimal percobaan connect
  while ((ret = mqtt.connect()) != 0) {
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 sec.");
    mqtt.disconnect();
    delay(5000);
    retries--;
    if (retries == 0) {
      // basically die and wait for WDT to reset me
      while(1);
    }
  }
  Serial.println("MQTT Connected!");
}

void setup() {
  dht.begin();
  Serial.begin(115200);
  pinMode(DSM501_PM1,INPUT);
  pinMode(DSM501_PM25,INPUT);
  delay(10);

  // Connect ke wifi
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin();
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();

  Serial.println("WiFi connected!");
  Serial.println("IP Address : ");
  Serial.println(WiFi.localIP());

}

void loop() {
  // cuma memastikan kalo masih connected atau ngga
  // kalo ngga (disconnected), function bakal melakukan reconnect
  MQTT_connect();

  unsigned long current_millis = millis();
  if (current_millis - previous_millis > interval_time) {
    /* Kode di dalam sarang "if" ini akan dieksekusi
    * tiap interval_time ms.
    * interval_time = 2000 = 2 detik.
    */

    // baca data dari sensor
    // pembacaan sensor dsm sekitar 2000ms
    durationPM1 = pulseIn(DSM501_PM1, LOW);
    durationPM25 = pulseIn(DSM501_PM25, LOW);

    lowpulseoccupancyPM1 += durationPM1;
    lowpulseoccupancyPM25 += durationPM25;

    //calculateConcentration(lowpulseoccupancy,duration (on second))
    float conPM1 = calculateConcentration(lowpulseoccupancyPM1,3);
    float conPM25 = calculateConcentration(lowpulseoccupancyPM25,3);
    Serial.print("PM1 ");
    Serial.print(conPM1);
    Serial.print("  PM2.5 ");
    Serial.println(conPM25);
    lowpulseoccupancyPM1 = 0;
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

    // Publish Data
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

    // update previous_milis
    previous_millis = current_millis;
  }

  // ping ke server biar koneksi tetep alive
    // gak perlu pake kalo interval ngirim data <= KEEPALIVE server
    /*
    if(! mqtt.ping()) {
      mqtt.disconnect();
    }
    */
}
