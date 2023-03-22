/*******************************************************************************************************
 * Ini kode untuk alat mikrokontroler mengirimkan data dari sensor menggunakan WiFi. 
 * Kode diperuntukan pada mikrokontroler esp8266.
 * 
 * Sensor yang digunakan : DSM501A dan DHT22
 * 
 * Credit : Yoshikuni
 * *****************************************************************************************************/
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>

#include <DHT.h>
#include <DHT_U.h>

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

// Detail jaringan WiFi yang akan dihubungkan
const char* ssid = "";
const char* password = "";

// Detail server RESTAPI
// Di bawah ini contoh server restapi gratis
//
//#define server_name  "https://httpbin.org/post"
#define server_name  "https://jsonplaceholder.typicode.com/posts"
//#define server_port 443
//#define server_user ""
//#define server_pass ""

// setting delay pengiriman data
unsigned long previous_millis = 0;
unsigned long interval_time = 2000; // 2000ms berarti 2 detik


// Define tipe DHT (sensor temperatur dan kelembaban)
#define DHTPIN D7
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// detail DSM501
#define DSM501_PM10 D5
#define DSM501_PM25 D6

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

long lastMsg = 0;

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
  Serial.begin(115200);
  // Serial.setDebugOutput(true);

  Serial.println();
  Serial.println();
  Serial.println();

  // initialise sensor
  dht.begin();
  pinMode(DSM501_PM10, INPUT);
  pinMode(DSM501_PM25, INPUT);

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
  // put your main code here, to run repeatedly:
  unsigned long current_millis = millis();

  if (current_millis - previous_millis > interval_time) {
    durationPM10 = pulseIn(DSM501_PM10, LOW);
    durationPM25 = pulseIn(DSM501_PM25, LOW);

    lowpulseoccupancyPM10 += durationPM10;
    lowpulseoccupancyPM25 += durationPM25;

    //calculateConcentration(lowpulseoccupancy,duration (on second))
    float conPM10 = calculateConcentration(lowpulseoccupancyPM10, 3);
    float conPM25 = calculateConcentration(lowpulseoccupancyPM25, 3);
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

    // publish data
    if (WiFi.status() == WL_CONNECTED) {
      std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);

      //client->setFingerprint(fingerprint);
      // Atau, kalo mau ignore SSL nya, pake kode dibawah ini
      client->setInsecure();

      HTTPClient https;

      https.begin(*client, server_name);
      // If you need Node-RED/server authentication, insert user and password below
      //https.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD");

      // If you need an HTTP request with a content type: application/json, use the following:
      https.addHeader("Content-Type", "application/json");

      // kirim data
      String payload;
      StaticJsonDocument<128> doc;
      doc["api_key"] = "tPmAT5Abxxxxx";
      doc["PM10"] = conPM10;
      doc["PM2.5"] = conPM25;
      doc["Temp"] = temp;
      doc["Humidity"] = humidity;
      serializeJson(doc, payload);
      
      int https_response_code = https.POST(payload);
      String payload_respone = https.getString();

      // jika kode http menunjukan 200,
      // berarti data terkirim
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
