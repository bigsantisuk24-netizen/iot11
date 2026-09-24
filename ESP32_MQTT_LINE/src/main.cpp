#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// ================= กำหนดค่าเครือข่าย =================
const char* ssid = "Santisuk";
const char* password = "123456789";

// ================= กำหนดค่า MQTT =================
const char* mqtt_server = "172.20.10.4";
const int mqtt_port = 1883;
const char* mqtt_topic = "sensor/room1";

// ================= กำหนดค่า LINE Messaging API =================
const String line_channel_token = "1hvHN3fRjqb/3LO78h46TjO3kFYY2Yoa31WFsMVo0K2DVjcwPEDrvZoBOtD1+8oDLtEXZ889pkhDeWlwtFc8Ey3V2BhpytsTPmAnF29X8Vz2f3khE4Wuc0bhoDDAExwESni4ltEV5WHnjzzuuwvHQwdB04t89/1O/w1cDnyilFU="; 
const String line_user_id = "U4c6389a45c51e7b9a5b1ac5ad37b9816";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// ประกาศตัวแปร secureClient ไว้ด้านนอก เพื่อลดภาระการกินหน่วยความจำ (RAM)
WiFiClientSecure secureClient;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
}

void reconnect_mqtt() {
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// ฟังก์ชันส่งข้อความผ่าน LINE Messaging API (LINE Bot)
void sendLineBot(String message) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.setTimeout(15000); 
    
    // เรียกใช้ secureClient ที่เราตั้งค่าไว้ตอน setup()
    http.begin(secureClient, "https://api.line.me/v2/bot/message/push");
    
    // กำหนด Headers ตามมาตรฐาน LINE API
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + line_channel_token);
    
    // สร้าง Payload แบบ JSON
    String jsonPayload = "{\"to\":\"" + line_user_id + "\",\"messages\":[{\"type\":\"text\",\"text\":\"" + message + "\"}]}";
    
    int httpCode = http.POST(jsonPayload);
    
    if (httpCode == 200) {
      Serial.println("LINE Bot: ส่งข้อความสำเร็จ!");
    } else {
      Serial.printf("LINE Bot: ส่งล้มเหลว HTTP Code %d\n", httpCode);
      Serial.println(http.getString()); 
    }
    http.end();
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  
  // ตั้งค่า Insecure ไว้ที่ setup ครั้งเดียวจบ
  secureClient.setInsecure(); 
  
  mqttClient.setServer(mqtt_server, mqtt_port);
  randomSeed(analogRead(34)); 
}

void loop() {
  if (!mqttClient.connected()) {
    reconnect_mqtt();
  }
  mqttClient.loop();

  // สุ่มค่าอุณหภูมิและความชื้น
  float temp = random(200, 400) / 10.0;
  float hum = random(400, 900) / 10.0;
  
  Serial.printf("Temp: %.1f C, Hum: %.1f %%\n", temp, hum);

  // 1. ส่งข้อมูลเข้า MQTT
  String payload = "{\"temperature\":" + String(temp, 1) + ", \"humidity\":" + String(hum, 1) + "}";
  mqttClient.publish(mqtt_topic, payload.c_str());
  Serial.println("ส่งข้อมูลเข้า MQTT แล้ว");

  // 2. สร้างข้อความสำหรับส่งเข้า LINE
  String lineMsg = "อุณหภูมิ: " + String(temp, 1) + " °C\\nความชื้น: " + String(hum, 1) + " %";
  sendLineBot(lineMsg);

  // หน่วงเวลา 10 วินาทีก่อนส่งรอบถัดไป
  delay(10000); 
}