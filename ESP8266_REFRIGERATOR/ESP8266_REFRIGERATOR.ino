#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h>
#include <EEPROM.h>
#include "DHT.h"

#define DEVICE_ID         "DEVICE001"
#define TRIGGER_PIN       D1
#define DHT_PIN           D4
#define DHT_TYPE          DHT22

#define EEPROM_SIZE       250
#define EEPROM_SSID_START 0
#define EEPROM_SSID_END   19
#define EEPROM_PASS_START 20
#define EEPROM_PASS_END   39

#define MQTT_SERVER       "mqtt.flespi.io"
#define MQTT_USERNAME     "XYwy6gDl0Y76a9C5vl18YJtp0RxQzkYm8iJ3occc078Z6BUqKLmzkGM8l9OLiAVe"
#define MQTT_PORT         1883

#define MAX_TEMP          40
#define MIN_TEMP          25
#define MAX_HUMI          90
#define MIN_HUMI          50

ESP8266WebServer server(5000);
DHT dht(DHT_PIN, DHT_TYPE);
WiFiClient espClient;
PubSubClient client(espClient);
bool setupDone = false;

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

void handleRoot() {
  server.send(200, "text/plain", DEVICE_ID);
  setupDone = true;
}

bool longPress()
{
  static int lastPress = 0;
  int mili = millis();
  while (millis() - mili < 5000) {
    if (millis() - lastPress > 2000 && digitalRead(0) == 0) { // Nếu button không được nhấn và giữ đủ 3s thì
      return true;
    } else if (digitalRead(0) == 1) {                         
      lastPress = millis();
    }
  }
  return false;
}

bool isTriggered() {
  if (digitalRead(TRIGGER_PIN) == HIGH) {
    return true;
  }
  return false;
}

void smartWifiConfig() {
  // Mode wifi là station
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Chờ kết nối
  Serial.println("Đang chờ thiết đặt ESPTouch");
  WiFi.beginSmartConfig();

  while(1){
    delay(1000);
    //Kiểm tra kết nối thành công in thông báo
    if(WiFi.smartConfigDone()){
      Serial.println("- SmartConfig sử dụng ESPTouch thành công!");
      break;
    }
  }

  Serial.println("Đang chờ kết nối Wi-Fi");
  while(WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");

  // In ra chi tiết kết nối
  WiFi.printDiag(Serial);

  // Khởi tạo server từ ESP để gửi ID thiết bị và kết thúc sau khi gửi xong
  Serial.println(WiFi.localIP());
  Serial.println("Server start!");
  server.on("/", handleRoot);
  server.begin();
  while(!setupDone) {
    server.handleClient();
  }
  Serial.println("Server stop!");

  // Lưu giá trị SSID và Password vào EEPROM
  EEPROM_write(WiFi.SSID(), EEPROM_SSID_START, EEPROM_SSID_END);
  EEPROM_write(WiFi.psk(), EEPROM_PASS_START, EEPROM_PASS_END);  
}

void SendSensorsData(float temp, float humi) {
  client.setServer(MQTT_SERVER, MQTT_PORT);
  while (!client.connected()) {
    String client_id = DEVICE_ID;
    if (client.connect(client_id.c_str(), MQTT_USERNAME, "")) {
      char tempString[8];
      char humiString[8];
      dtostrf(temp, 4, 1, tempString);
      dtostrf(humi, 4, 1, humiString);
      
      char topic[18];
      strcpy(topic, DEVICE_ID);
      strcat(topic, "/SensorsData");
      Serial.print("\"");
      Serial.print(topic);
      Serial.println("\"");
      
      char data[13];
      strcpy(data, tempString);
      strcat(data, " ");
      strcat(data, humiString);
      Serial.println(data);
      
      client.publish("DEVICE001/SensorsData", data);
    } else {
      Serial.print("\nMQTT failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

void ConnectWiFi() {
  String ssid = EEPROM_read(EEPROM_SSID_START, EEPROM_SSID_END);
  String pass = EEPROM_read(EEPROM_PASS_START, EEPROM_PASS_END);
//  String ssid = "H2O";
//  String pass = "55555555";
  WiFi.begin(ssid, pass);
  Serial.println();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
}

void setup() {
  //Khởi tạo baud 115200
  Serial.begin(74880);
  
  
  pinMode(0, INPUT);
  digitalWrite(0, HIGH);

  dht.begin();
  if(isTriggered()) {
    Serial.println("\nChế độ thiết đặt");
    smartWifiConfig();
  }
  
  Serial.println("Chương trình chính");

  ConnectWiFi();
  float temp = dht.readTemperature();
  float humi = dht.readHumidity();
  SendSensorsData(temp, humi);
  Serial.print("\nNhiet do: ");
  Serial.println(String(temp));
  Serial.print("Do am: ");
  Serial.println(String(humi));
  
  // Check if abnormal temperature or humidity
  if (temp > MAX_TEMP || temp < MIN_TEMP || humi > MAX_HUMI || humi < MIN_HUMI) {
    Serial.println("Nhiet do hoac do am bat thuong");
  }
  else {
    Serial.println("Nhiet do va do am binh thuong");
  }
  
  delay(100);
  ESP.deepSleep(15e6);
}

void loop() {

}
