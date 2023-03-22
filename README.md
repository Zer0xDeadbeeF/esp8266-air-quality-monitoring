# Air Quality Monitoring Device with ESP8266

ToDo:
- [x] gather sensor device and others tools required
- [x] make code base
- [x] make Fritzing model
- [ ] make container for the device
- [ ] make server using mosquitto(server) and NodeRED(frontend)

Libraries used for this project:
- Adafruit Sensor 	: [adafruit/Adafruit_Sensor](https://github.com/adafruit/Adafruit_Sensor)
- DHT Sensor 		: [adafruit/DHT-sensor-library](https://github.com/adafruit/DHT-sensor-library)
- FONA library 		: [adafruit/Adafruit_FONA](https://github.com/adafruit/Adafruit_FONA/)
- MQTT library 		: [adafruit/Adafruit_MQTT_library](https://github.com/adafruit/Adafruit_MQTT_Library)

Software used for this project:
- Adruino IDE Version 1.8.19
- Sublime Text Editor (cuz Arduino IDE doesn't have dark mode)
- Fritzing
- Wireshark
- MQTT X 

Hardware used for this project:
- ESP8266 NodeMCU 
- DC-DC Buck converter 
- DHT22 (temperatur and humidity sensor)
- DSM501A (air quality sensor)
- SIM800l GSM Module V2
- INA219 (to monitor current and voltage)  ===> used for power monitoring
- USB Power Monitor (to monitor total mAh) ===> also, used for power monitoring