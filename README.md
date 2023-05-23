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
- Adruino IDE Version 2.1.0
- Sublime Text Editor
- Fritzing
- Wireshark
- MQTT X 

Hardware used for this project:
- ESP8266 NodeMCU 
- DC-DC Buck converter 
- DHT22 (temperature and humidity sensor)
- DSM501A (air quality sensor)
- SIM800l GSM Module V2
- INA219 (to monitor current and voltage)  ===> used for power monitoring
- USB Power Monitor (to monitor total mAh) ===> also, used for power monitoring

For cloud broker or server, I use EMQ X MQTT Broker as my MQTT broker. It's free, but can use any MQTT broker as long it can run MQTT version 3.x, you can try use HiveMQTT. Last testing, EMQ can use HTTP Endpoint to publish messages to the broker, but now somehow it can't do that anymore. 