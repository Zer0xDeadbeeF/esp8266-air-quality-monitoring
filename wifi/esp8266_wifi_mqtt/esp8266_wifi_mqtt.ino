/*******************************************************************************************************
 * Ini kode untuk alat mikrokontroler mengirimkan data dari sensor menggunakan WiFi. 
 * Kode diperuntukan pada mikrokontroler esp8266.
 * 
 * Sensor yang digunakan : DSM501A dan DHT22
 * 
 * Credit : Yoshikuni
 * *****************************************************************************************************/
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "DHT.h"
#include "DHT_U.h"

// detail wifi dan server edit di file secret.h
#include "secret.h"

// konfigurasi mau pakai apa connect ke servernya
WiFiClient client; // defaultnya ini
// atau pakai WiFiClientSecure untuk SSL
//WiFiClientSecure client;

// setup mqtt
// MQTT_QOS_0 untuk publish data dengan QoS 0
// MQTT_QOS_1 untuk publish data dengan QoS 1
// 
// QoS 2 belum diimplementasikan dalam librarynya,
// jadi ini keterbatasan dari library.
// baca: https://learn.adafruit.com/mqtt-adafruit-io-and-you/qos-and-wills
//
Adafruit_MQTT_Client mqtt(&client, server_name, server_port, server_user, server_pass);

Adafruit_MQTT_Publish sensor_dht_temp = Adafruit_MQTT_Publish(&mqtt, "/topic/sensor_dht_temp", MQTT_QOS_0);
Adafruit_MQTT_Publish sensor_dht_humidity = Adafruit_MQTT_Publish(&mqtt, "/topic/sensor_dht_humidity", MQTT_QOS_0);
Adafruit_MQTT_Publish sensor_dsm_pm10 = Adafruit_MQTT_Publish(&mqtt, "/topic/sensor_dsm_pm10", MQTT_QOS_0);
Adafruit_MQTT_Publish sensor_dsm_pm25 = Adafruit_MQTT_Publish(&mqtt, "/topic/sensor_dsm_pm25", MQTT_QOS_0);


// Define tipe DHT (sensor temperatur dan kelembaban)
#define DHTPIN D7
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// detail DSM501
#define DSM501_PM10 D5
#define DSM501_PM25 D6

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

// dari contoh kodingan, harus nambahin ini. Ada bug di IDE v1.6.6.
void MQTT_connect();

void connect_to_wifi() {
  WiFi.begin(wifi_ssid, wifi_pass);
  Serial.println("Connecting to WiFi..");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".\n");
  }
  Serial.println("");
  Serial.print("Connected to WiFi! IP Address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  // Serial.setDebugOutput(true);

  Serial.println();
  Serial.println();
  Serial.println();

  // initialise sensor
  dht.begin();
  pinMode(DSM501_PM10,INPUT);
  pinMode(DSM501_PM25,INPUT);

  // liat wifi yang ada di sekitar
  int number_of_networks = WiFi.scanNetworks();
  for(int i =0; i<number_of_networks; i++){

      Serial.print("Network name: ");
      Serial.println(WiFi.SSID(i));
      Serial.print("Signal strength: ");
      Serial.println(WiFi.RSSI(i));
      Serial.println("-----------------------");
  }

  delay(10);
  WiFi.mode(WIFI_STA); // ubah mode jadi client, bukan Acces Point (hotspot)
  connect_to_wifi();
}

void loop() {
  // cuma memastikan kalo masih connected atau ngga
  // kalo ngga (disconnected), function bakal melakukan reconnect
  MQTT_connect();

  unsigned long current_millis = millis();
  if (current_millis - previous_millis > interval_time) {

    // baca data dari sensor
    // pembacaan sensor dsm sekitar 2000ms
    durationPM10 = pulseIn(DSM501_PM10, LOW);
    durationPM25 = pulseIn(DSM501_PM25, LOW);

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
      // data yang akan dipublish berupa "0.0"
      temp = 0.0;
      humidity = 0.0;
    }

    // Publish Data
    if (WiFi.status() == WL_CONNECTED) {
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
    } else{
      Serial.println("WiFi Disconnected");
      connect_to_wifi();
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

void MQTT_connect() {
  int8_t ret;

  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}
