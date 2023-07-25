#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "DHT.h"

// wifi and broker credentials
#include "secret.h"


// setup for mqtt
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, server_name, server_port, server_user, server_pass);
Adafruit_MQTT_Publish msg_payload = Adafruit_MQTT_Publish(&mqtt, "/topic/msg_payload", MQTT_QOS_0);


// setup for dht sensor
#define DHTPIN D7
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

float temp = 0;
float humidity = 0;


// setup for dsm501a sensor
#define DSM501_PM10_PIN D5
#define DSM501_PM25_PIN D6

unsigned long lpo_pm10;
unsigned long lpo_pm25;
float ratio_pm10 = 0;
float ratio_pm25 = 0;
float concentration_pm10 = 0;
float concentration_pm25 = 0;


// set interval time
unsigned long start_time;
unsigned long interval_time = 2000; // 2000ms means 2s


// warming up hardware
void setup_hardware() {
	pinMode(DSM501_PM10_PIN, INPUT);
	pinMode(DSM501_PM25_PIN, INPUT);

	// wait 60s to warmup dsm501a
	for (int i = 0; i <= 60; i++) {
		delay(1000);
		Serial.print(i);
		Serial.println(" s (wait 60s for dsm501a to warm up.)");
	}

	dht.begin();
}


// connect to nearby wifi connection
void connect_to_wifi() {
  WiFi.begin(wifi_ssid, wifi_pass);
  Serial.println("[info] connecting to WiFi..");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".\n");
  }
  Serial.println("");
  Serial.print("[info] connected to WiFi! IP Address: ");
  Serial.println(WiFi.localIP());
}


// connect to broker/server/cloud
void connect_to_mqtt() {
  int8_t ret;

  if (mqtt.connected()) {
    return;
  }

  Serial.print("[info] connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("[warning] retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("[info] connected to mqtt broker!");
}


// calculate polution weight to mg/m^3
float get_particle_weight(float ratio) {
	/*
	 * from data sheet, regresion function is
	 * 		y=0.1776*x^3-2.24*x^2+ 94.003*x
	 *
	 * https://github.com/R2D2-2019/R2D2-2019/wiki/Is-the-given-formula-for-calculating-the-mg-m3-for-the-dust-sensor-dsm501a-correct%3F
	 */	

	float mgm3 = 0.001915 * pow(ratio , 2) + 0.09522 * ratio - 0.04884;
  return mgm3  < 0.0 ? 0.0 : mgm3;
}


// formatting payload into JSON format
void set_mqtt_payload(float concentration_pm10, float concentration_pm25, float weight_pm10, float weight_pm25, float temp, float humidity) {
  String payload = "{";
  payload += "\"cPM10\": " + String(concentration_pm10) + ",";
  payload += "\"cPM25\": " + String(concentration_pm25) + ",";
  payload += "\"wPM10\": " + String(weight_pm10) + ",";
  payload += "\"wPM25\": " + String(weight_pm25) + ",";
  payload += "\"temp\": " + String(temp) + ",";
  payload += "\"humidity\": " + String(humidity);
  payload += "}";

  /*
   * MQTT_Publish class need C-style string
   * so, we need to convert String payload
   * to C-style string, medokusai.
   */
  char converted_payload[payload.length() + 1];
  payload.toCharArray(converted_payload, sizeof(converted_payload));

  // publish payload
	if (msg_payload.publish(converted_payload)) {
    // uncomment code below for debugging only
    // Serial.println("[info] payload published!");
  } else {
    // Serial.println("[error] payload not published due error.");
  }
}


void setup() {
	Serial.begin(9600);

	// connect to wifi
  	int number_of_networks = WiFi.scanNetworks();
  	for (int i = 0; i<number_of_networks; i++) {
  		Serial.print("Network name: ");
      	Serial.println(WiFi.SSID(i));
      	Serial.print("Signal strength: ");
      	Serial.println(WiFi.RSSI(i));
      	Serial.println("-----------------------");
  	}
  	delay(100);
  	WiFi.mode(WIFI_STA);
  	connect_to_wifi();

  	delay(100);
  	setup_hardware();
}


void loop() {
	connect_to_mqtt();

	lpo_pm10 += pulseIn(DSM501_PM10_PIN, LOW);
	lpo_pm25 += pulseIn(DSM501_PM25_PIN, LOW);

	if ((millis() - start_time) > interval_time) {
		ratio_pm10 = lpo_pm10 / (interval_time * 10.0);
		concentration_pm10 = 1.1 * pow(ratio_pm10,3) - 3.8 * pow(ratio_pm10,2) + 520 * ratio_pm10 + 0.62;

		ratio_pm25 = lpo_pm25 / (interval_time * 10.0);
		concentration_pm25 = 1.1 * pow(ratio_pm25,3) - 3.8 * pow(ratio_pm25,2) + 520 * ratio_pm25 + 0.62;

		float weight_pm10 = get_particle_weight(ratio_pm10);
		float weight_pm25 = get_particle_weight(ratio_pm25);

		// get dht data
		temp = dht.readTemperature();
		humidity = dht.readHumidity();

		// for debugging only
		// Serial.println("========================================================");
		// Serial.print("PM10 Concentration: "); Serial.println(concentration_pm10);
    // Serial.print("PM10 Weight: "); Serial.println(weight_pm10);
    // Serial.print("PM2.5 Concentration: "); Serial.println(concentration_pm25);
    // Serial.print("PM2.5 Weight: "); Serial.println(weight_pm25);
    // Serial.print("Temperature: "); Serial.println(temp);
    // Serial.print("Humidity: "); Serial.println(humidity);

		// set and then send payload
		set_mqtt_payload(concentration_pm10, concentration_pm25, weight_pm10, weight_pm25, temp, humidity);

		lpo_pm10 = 0;
		lpo_pm25 = 0;
		start_time = millis();
	}
}