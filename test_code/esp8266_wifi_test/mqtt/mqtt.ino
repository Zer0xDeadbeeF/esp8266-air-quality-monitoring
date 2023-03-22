// THIS CODE USED FOR MQTT PUBLISH
// RUN FIRST BEFORE DOING ANY MODIFICATION
//
// If there no error after first run
// then you can write your code such as 
// reading data from sensor
//
// Much love,
// Created by Yoshi.
//

#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

// detail wifi
const char* ssid = "";
const char* password = "";

// detail server mqtt
// di bawah ini pakai broker publik emqx
#define server_name  "broker.emqx.io"
#define server_port  1883
#define server_user  ""
#define server_pass  ""

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
Adafruit_MQTT_Publish contoh_publish = Adafruit_MQTT_Publish(&mqtt, "testtopic/contoh", MQTT_QOS_0); 


// setting delay pengiriman data
unsigned long previous_millis = 0;
unsigned long interval_time = 2000; // 2000ms berarti 2 detik

// dummy payload
char* payload = "Hello, World!";

// dari contoh kodingan, harus nambahin ini. Ada bug di IDE v1.6.6.
void MQTT_connect();

void connect_to_wifi() {
  WiFi.begin(ssid, password);
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
  Serial.begin(115200);
  // Serial.setDebugOutput(true);

  Serial.println();
  Serial.println();
  Serial.println();

  // liat wifi yang ada di sekitar
  int number_of_networks = WiFi.scanNetworks();
  for(int i =0; i<number_of_networks; i++){

      Serial.print("Network name: ");
      Serial.println(WiFi.SSID(i));
      Serial.print("Signal strength: ");
      Serial.println(WiFi.RSSI(i));
      Serial.println("-----------------------");
  }

  WiFi.mode(WIFI_STA); // ubah mode jadi client, bukan Acces Point (hotspot)
  connect_to_wifi();
}

void loop() {
  // put your main code here, to run repeatedly:
  MQTT_connect();
  
  unsigned long current_millis = millis();
  
  if (current_millis - previous_millis > interval_time) {
    if (WiFi.status() == WL_CONNECTED) {

      // kirim data
      Serial.println("Publishing data: ");
      Serial.println(payload);
      
      if (! contoh_publish.publish(payload)) {
        Serial.println("Failed!");
      } else {
        Serial.println("Published!");
      }

      // ping server biar koneksi mqtt nya tetap terhubung
      // kalo frekuensi publish datanya <=  KEEPALIVE, gak perlu
      //if (! mqtt.ping()){
      //  mqtt.disconnect();
      //}
    } else {
      Serial.println("WiFi disconnected!");
      connect_to_wifi();
    }
    previous_millis = current_millis;
  }
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
