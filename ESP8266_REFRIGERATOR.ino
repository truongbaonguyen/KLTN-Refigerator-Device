#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <EEPROM.h>
#include "DHT.h"

#define EEPROM_SIZE       250
#define EEPROM_SSID_START 0
#define EEPROM_SSID_END   19
#define EEPROM_PASS_START 20
#define EEPROM_PASS_END   39

#define TRIGGER_PIN       D1
#define DHT_PIN           D4
#define DHT_TYPE          DHT22

#define MQTT_SERVER       "mqtt.flespi.io"
#define MQTT_USERNAME     "XYwy6gDl0Y76a9C5vl18YJtp0RxQzkYm8iJ3occc078Z6BUqKLmzkGM8l9OLiAVe"
#define MQTT_PORT         1883

#define MAX_TEMP          40
#define MIN_TEMP          25
#define MAX_HUMI          90
#define MIN_HUMI          50


DHT dht(DHT_PIN, DHT_TYPE);
WiFiClient espClient;
PubSubClient client(espClient);

String EEPROM_read(int begin, int end) {
  String data;
  char c;
  for (int i = begin; i <= end; ++i) {
    c = (char) EEPROM.read(i);
    if (c != 0) {
      data += c;
    }
  }
  return data;
}

void EEPROM_clear(int start, int end) {
  for (int i = start; i <= end; ++i) {
    EEPROM.write(i, 0);
  }
}

void EEPROM_write(const String &data, int begin, int endMax) {
  int end = data.length() + begin;
  if (end - 1 > endMax) {
    Serial.print("[EEPRomService][write] Size too large");
    return;
  }
  EEPROM_clear(begin, endMax);

  for (int i = begin; i < end; i++) {
    EEPROM.write(i, data[i - begin]);
  }
  EEPROM.commit();
}

void ConnectWiFi() {
  String ssid = EEPROM_read(EEPROM_SSID_START, EEPROM_SSID_END);
  String pass = EEPROM_read(EEPROM_PASS_START, EEPROM_PASS_END);
  WiFi.begin("NOR", "vietgangZ");
  Serial.println();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
}

void SendSensorsData(float temp, float humi) {
  client.setServer(MQTT_SERVER, MQTT_PORT);
  while (!client.connected()) {
    String client_id = "esp01";
    if (client.connect(client_id.c_str(), MQTT_USERNAME, "")) {
      char tempString[8];
      char humiString[8];
      dtostrf(temp, 4, 1, tempString);
      dtostrf(humi, 4, 1, humiString);
      client.publish("ESP01/temp", tempString);
      client.publish("ESP01/humi", humiString);
    } else {
      Serial.print("\nMQTT failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  dht.begin();
  EEPROM.begin(EEPROM_SIZE);
  pinMode(TRIGGER_PIN, INPUT); 
    
  if(digitalRead(TRIGGER_PIN) == HIGH) {
    WiFiManager wifiManager;
    wifiManager.resetSettings();
    wifiManager.autoConnect("SMART REFRIGERATOR WiFi Manager");
    Serial.println("Wifi connected :)");
    EEPROM_write(wifiManager.getWiFiSSID(), EEPROM_SSID_START, EEPROM_SSID_END);
    EEPROM_write(wifiManager.getWiFiPass(), EEPROM_PASS_START, EEPROM_PASS_END);
  }
  else {
    // Read sensors data
    float temp = dht.readTemperature();
    float humi = dht.readHumidity();

    // Check if abnormal temperature or humidity
    if (temp > MAX_TEMP || temp < MIN_TEMP || humi > MAX_HUMI || humi < MIN_HUMI) {
      ConnectWiFi();
      SendSensorsData(temp, humi);
      Serial.print("\nNhiet do: ");
      Serial.println(String(temp));
      Serial.print("Do am: ");
      Serial.println(String(humi));
      Serial.println("Nhiet do hoac do am bat thuong");
    }
    else {
      Serial.print("\nNhiet do: ");
      Serial.println(String(temp));
      Serial.print("Do am: ");
      Serial.println(String(humi));
      Serial.println("Nhiet do va do am binh thuong");
    }

    // Store temp, humi into EEPROM

    delay(100);
    ESP.deepSleep(15e6);
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      delay(500);
      Serial.println("MQTT connected");
      // Subscribe
//      client.subscribe("esp32/output");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() { 
  
}
