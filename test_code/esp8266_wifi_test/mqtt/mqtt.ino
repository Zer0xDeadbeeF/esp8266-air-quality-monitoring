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
// below using emqx public mqtt broker
#define server_name  "broker.emqx.io"
#define server_port  1883
#define server_user  ""
#define server_pass  ""

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client; 
// or use WiFiClientSecure for SSL
//WiFiClientSecure client;

// setup mqtt
// MQTT_QOS_0 for publishing data using QoS 0
// MQTT_QOS_1 for publishing data using QoS 1
// 
// There's also QoS 2, This is a bit more complex because you need to 
// start tracking packet IDs so we'll leave that for a later time.
// read: https://learn.adafruit.com/mqtt-adafruit-io-and-you/qos-and-wills
//

Adafruit_MQTT_Client mqtt(&client, server_name, server_port, server_user, server_pass);
Adafruit_MQTT_Publish example_publish = Adafruit_MQTT_Publish(&mqtt, "testtopic/example", MQTT_QOS_0); 


// set delay
unsigned long previous_millis = 0;
unsigned long interval_time = 2000; // 2000ms means 2 second

// dummy payload
char* payload = "Hello, World!";

// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).
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
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();
  
  unsigned long current_millis = millis();
  
  if (current_millis - previous_millis > interval_time) {
    if (WiFi.status() == WL_CONNECTED) {

      // publish data
      Serial.println("Publishing data: ");
      Serial.println(payload);
      
      if (! example_publish.publish(payload)) {
        Serial.println("Failed!");
      } else {
        Serial.println("Published!");
      }

      // ping the server to keep the mqtt connection alive
      // NOT required if you are publishing once every KEEPALIVE seconds
      /*
      if(! mqtt.ping()) {
        mqtt.disconnect();
      }
      */
    } else {
      Serial.println("WiFi disconnected!");
      connect_to_wifi();
    }
    previous_millis = current_millis;
  }
}


// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
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
