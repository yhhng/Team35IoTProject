#include <Boho.h>
#include "M5StickCPlus.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include "secrets.h"

#define IR_SENSOR_PIN 26  // IR sensor pin
#define SIZE 50

WiFiClientSecure espClient;
PubSubClient client(espClient);
Boho boho;

char msg[50];
int lastSensorValue = -1;
char plainData[SIZE];
uint8_t encData[SIZE + MetaSize_ENC_PACK]; // Encoded data pack size will be increased
char decData[SIZE];

// Function Prototypes
void setupWifi();
void callback(char* topic, byte* payload, unsigned int length);
void reConnect();

// Encrypt function with Boho
String encryptMessage(String msg) {
  // Debug logging
  Serial.println("\nEncryption Debug:");
  Serial.print("Input message: ");
  Serial.println(msg);

  // Clear encryption buffer
  memset(encData, 0, SIZE + MetaSize_ENC_PACK);
  
  // Copy message to plain data buffer
  strncpy(plainData, msg.c_str(), SIZE - 1);
  plainData[SIZE - 1] = '\0'; // Ensure null termination
  
  int len = strlen(plainData);
  
  // Debug plain data
  Serial.print("Plain data: ");
  Serial.println(plainData);
  
  // Encrypt with Boho
  int packSize = boho.encryptPack(encData, plainData, len);
  
  // Debug encrypted size
  Serial.print("Encrypted pack size: ");
  Serial.println(packSize);
  
  // Debug encrypted bytes
  Serial.print("Encrypted bytes: ");
  for (int i = 0; i < packSize; i++) {
    Serial.print((int)encData[i]);
    Serial.print(" ");
  }
  Serial.println();
  
  // Convert binary data to string for MQTT transmission
  // We need to use a special encoding to preserve binary data
  String result = "";
  for (int i = 0; i < packSize; i++) {
    // Convert each byte to a hex string
    char hex[3];
    sprintf(hex, "%02X", encData[i]);
    result += hex;
  }
  
  Serial.print("Encoded for MQTT: ");
  Serial.println(result);
  
  return result;
}

void setup() {
  Serial.begin(115200);
  delay(100);

  M5.begin();
  M5.Lcd.setRotation(3);
  pinMode(IR_SENSOR_PIN, INPUT_PULLUP);
  
  // Initialize Boho with a key
  boho.set_key("12345678"); // Must use same key in CentralCommand.ino

  setupWifi();
  espClient.setPreSharedKey(pskIdent, psKey);

  Serial.println("\nStarting connection to server...");
  client.setServer(mqtt_server, 8883);
  client.setCallback(callback);

  M5.Lcd.println("Node C (Kitchen) Ready");
}

void loop() {
  if (!client.connected()) {
    reConnect();
  }
  client.loop();

  // Read IR sensor value
  int sensorValue = digitalRead(IR_SENSOR_PIN) ? 0 : 1;

  // Publish only if changed
  if (sensorValue != lastSensorValue) {
    String plainText = String(sensorValue);
    String encryptedMsg = encryptMessage(plainText);
    client.publish(topicKitchen, encryptedMsg.c_str());

    // LCD update
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.print("Kitchen's Door IR Sensor: ");
    M5.Lcd.println(sensorValue);

    lastSensorValue = sensorValue;
  }

  delay(100);
}

void setupWifi() {
  delay(10);
  M5.Lcd.printf("Connecting to %s", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    M5.Lcd.print(".");
  }
  M5.Lcd.println("\nConnected");
}

void callback(char* topic, byte* payload, unsigned int length) {
  String received = "";
  for (int i = 0; i < length; i++) {
    received += (char)payload[i];
  }

  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.print("Received MQTT Msg\nTopic: ");
  M5.Lcd.println(topic);
  M5.Lcd.print("Message: ");
  M5.Lcd.println(received);
}

void reConnect() {
  while (!client.connected()) {
    M5.Lcd.print("Attempting MQTT connection...");
    String clientId = "Sensor1-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str(), mqtt_username, mqtt_password)) {
      M5.Lcd.println("\nConnected");
      client.subscribe("home/kitchen/door/cmd");
    } else {
      M5.Lcd.print("Failed, rc=");
      M5.Lcd.println(client.state());
      delay(5000);
    }
  }
}
