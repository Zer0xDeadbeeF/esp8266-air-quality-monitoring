// THIS CODE USED FOR HTTPS POST
// RUN FIRST BEFORE DOING ANY MODIFICATION
//
// If there no error after first run
// then you can write your code such as 
// reading data from sensor
//
// Much love,
// Created by Yoshi.
//

#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>

#include <WiFiClientSecureBearSSL.h>
// Cara dapetin fingerprint SSL (pake yang SHA-1)
// 1. Akses endpoint pake browser
// 2. Buka development menu (ctrl shift i)
// 3. Buka tab Security
// 4. Klik View Certificate
// 5. Tambahin 0x di tiap data fingerprint
//
// Contoh fingerprint original :
// 70 7C 82 07 F3 58 18 87 25 42 31 83 45 86 BD 17 86 71 4E 1F
const uint8_t fingerprint[20] = {0x70, 0x7C, 0x82, 0x07, 0xF3, 0x58, 0x18, 0x87, 0x25, 0x42, 0x31, 0x83, 0x45, 0x86, 0xBD, 0x17, 0x86, 0x71, 0x4E, 0x1F};

// detail wifi
const char* ssid = "";
const char* password = "";

// Detail server RESTAPI
// below is free RESTAPI server
//
//#define server_name  "https://httpbin.org/post"
#define server_name  "https://jsonplaceholder.typicode.com/posts"
//#define server_port 443
//#define server_user ""
//#define server_pass ""

// set delay
unsigned long previous_millis = 0;
unsigned long interval_time = 2000; // 2000ms means 2 second

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

  // list of wifi around
  int number_of_networks = WiFi.scanNetworks();
  for(int i =0; i<number_of_networks; i++){

      Serial.print("Network name: ");
      Serial.println(WiFi.SSID(i));
      Serial.print("Signal strength: ");
      Serial.println(WiFi.RSSI(i));
      Serial.println("-----------------------");
  }

  WiFi.mode(WIFI_STA);
  connect_to_wifi();
}

void loop() {
  // put your main code here, to run repeatedly:
  unsigned long current_millis = millis();
  
  if (current_millis - previous_millis > interval_time) {
    // check if connected to wifi
    if (WiFi.status() == WL_CONNECTED) {
      std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);

      // client->setFingerprint(fingerprint);
      // Or, if you happy to ignore the SSL certificate, then use the following line instead:
      client->setInsecure();

      HTTPClient https;

      https.begin(*client, server_name);
      // If you need Node-RED/server authentication, insert user and password below
      //https.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD");

      // If you need an HTTP request with a content type: application/json, use the following:
      https.addHeader("Content-Type", "application/json");

      // publish data
      String payload;
      StaticJsonDocument<128> doc;
      doc["api_key"] = "tPmAT5Abxxxxx";
      doc["PM10"] = "12";
      doc["PM2.5"] = "12";
      doc["Temp"] = "43.24";
      doc["Humidity"] = "43.24";
      serializeJson(doc, payload);
      
      int https_response_code = https.POST(payload);
      String payload_respone = https.getString();

      // if HTTP code is 200 (or any 200 code)
      // this means payload published.
      Serial.print("HTTP(s) Response code: ");
      Serial.println(https_response_code);
      Serial.println(payload_respone);

      https.end();
      
    } else {
      Serial.println("WiFi Disconnected");
      connect_to_wifi();
    }
    previous_millis = current_millis;
  }
}
