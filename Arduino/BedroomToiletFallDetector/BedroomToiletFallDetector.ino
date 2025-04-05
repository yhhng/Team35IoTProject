#include <Boho.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include "secrets.h"

#define HOME_BUTTON_PIN 3  // Home Button Pin
bool currentPinState = false;
bool previousPinState = false;  // Track previous state for change detection

// Flags for debounce
unsigned long lastButtonPressTime = 0;
unsigned long buttonDebounceDelay = 1000;  // 1 sec debounce

WiFiClientSecure espClient;
PubSubClient client(espClient);
Boho boho;

#define SIZE 50
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

  pinMode(HOME_BUTTON_PIN, INPUT_PULLUP);

  // Initialize Boho with the same key as the other devices
  boho.set_key("12345678");

  setupWifi();

  espClient.setPreSharedKey(pskIdent, psKey);

  Serial.println("\nStarting connection to server...");
  client.setServer(mqtt_server, 8883);
  client.setCallback(callback);

  // Set Keep Alive to the maximum value (65535 seconds)
  client.setKeepAlive(65535);

  Serial.println("Bedroom's Toilet Rug Node Ready");
}

void loop() {
  if (!client.connected()) {
    reConnect();
  }
  client.loop();

  // Handle button press with debounce
  if (digitalRead(HOME_BUTTON_PIN) == 0) {
    unsigned long currentTime = millis();
    if (currentTime - lastButtonPressTime > buttonDebounceDelay) {
      lastButtonPressTime = currentTime;
      currentPinState = !currentPinState;

      // Only send MQTT message when state changes
      if (currentPinState != previousPinState) {
        String plainText = String(currentPinState ? "1" : "0");  // Send "1" for true, "0" for false
        String encryptedMsg = encryptMessage(plainText);
        client.publish(topicBedroomToiletFallDectector, encryptedMsg.c_str());

        // Only print alert when state changes to true
        if (currentPinState) {
          Serial.println("Fall Alert!");
        }

        previousPinState = currentPinState;  // Update previous state
      }
    }
  }
  delay(100);
}

void setupWifi() {
  delay(10);
  Serial.printf("Connecting to %s", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected");
}

void callback(char* topic, byte* payload, unsigned int length) {
  String received = "";
  for (int i = 0; i < length; i++) {
    received += (char)payload[i];
  }

  Serial.println("Received MQTT Msg");
  Serial.print("Topic: ");
  Serial.println(topic);
  Serial.print("Message: ");
  Serial.println(received);
}

void reConnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "BedroomToiletFallDetectorSensor-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("\nConnected");
    } else {
      Serial.print("Failed, rc=");
      Serial.println(client.state());
      delay(5000);
    }
  }
}