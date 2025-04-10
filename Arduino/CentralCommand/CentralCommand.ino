#include <Boho.h>
#include "M5StickCPlus.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <Firebase.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "secrets.h"

// Stack for storing location history
String locationStack[10];
int stackTop = -1;

WiFiClientSecure espClient;
PubSubClient client(espClient);
Boho boho;

// Firebase client with authentication
Firebase fb(REFERENCE_URL, FIREBASE_AUTH_TOKEN);  // Initialize Firebase with URL and auth token

// NTP Client setup
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

#define SIZE 50
char plainData[SIZE];
uint8_t encData[SIZE + MetaSize_ENC_PACK];  // Encoded data pack size will be increased
char decData[SIZE];

// Flags for human presence detection
bool isPersonOnBed = false;
bool isPersonOnSofa = false;

// Flags for room presence
bool isPersonInBedroom = false;
bool isPersonInLivingRoom = false;
bool isPersonInKitchen = false;

// MQTT Keep Alive settings
const unsigned long MQTT_KEEP_ALIVE = 65535;  // Maximum keep-alive value (65535 seconds = ~18.2 hours)

// Firebase authentication status
bool isFirebaseAuthenticated = false;

// Fall incident counter
unsigned long fallIncidentCounter = 0;

void setupWifi();
void callback(char* topic, byte* payload, unsigned int length);
void reConnect();
void pushLocation(String loc);
void popLocation();
void displayLocation();
void publishLocationToFirebase(String loc);
void reportFallToFirebase(String location);
void updatePresenceStatus();
void updateDetailedLocation();
bool authenticateFirebase();
String generateFallIncidentId();

// Sensor state management
int lastSensorValueLivingRoom = -1;
int lastSensorValueBedroom = -1;
int lastSensorValueToilet = -1;

// Function to generate unique fall incident ID
String generateFallIncidentId() {
  fallIncidentCounter++;
  return "fall_" + String(fallIncidentCounter);
}

// Function to update display with a given message
void displayMessage(const char* message) {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.println(message);
}

void setup() {
  Serial.begin(115200);
  M5.begin();
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);

  // Initialize Boho with the same key as the sender
  boho.set_key("12345678");

  // Set up WiFi and MQTT connection
  setupWifi();
  espClient.setPreSharedKey(pskIdent, psKey);
  client.setServer(mqtt_server, 8883);
  client.setCallback(callback);
  
  // Set maximum keep-alive for MQTT
  client.setKeepAlive(MQTT_KEEP_ALIVE);

  // Initialize NTP Client
  timeClient.begin();
  timeClient.setTimeOffset(28800);  // UTC+8 for Singapore timezone (8 * 60 * 60)

  // Set up Firebase
  WiFi.disconnect();
  delay(1000);

  Serial.println();
  Serial.print("Connecting to: ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("-");
    delay(500);
  }

  Serial.println();
  Serial.println("WiFi Connected");
  Serial.println();

  // Authenticate with Firebase
  if (authenticateFirebase()) {
    Serial.println("Firebase Authentication Successful");
    displayMessage("Firebase Connected");
  } else {
    Serial.println("Firebase Authentication Failed");
    displayMessage("Firebase Auth Failed");
  }

  // Initialize stack with "NIL" as the default location
  locationStack[0] = "NIL";
  stackTop = 0;

  displayMessage("User Location Tracker Ready");
}

// Function to authenticate with Firebase
bool authenticateFirebase() {
  Serial.println("Testing Firebase connection...");
  
  // Try to read a test value
  String testValue = fb.getString("/test_connection");
  if (testValue.length() > 0) {
    isFirebaseAuthenticated = true;
    return true;
  }
  
  isFirebaseAuthenticated = false;
  return false;
}

void loop() {
  if (!client.connected()) {
    reConnect();
  }
  client.loop();
  timeClient.update();  // Keep the time updated

  // Update presence status display periodically
  static unsigned long lastPresenceUpdate = 0;
  if (millis() - lastPresenceUpdate > 5000) {  // Update every 5 seconds
    updatePresenceStatus();
    lastPresenceUpdate = millis();
  }

  // Check Firebase authentication periodically
  static unsigned long lastAuthCheck = 0;
  if (millis() - lastAuthCheck > 300000) {  // Check every 5 minutes
    if (!isFirebaseAuthenticated) {
      authenticateFirebase();
    }
    lastAuthCheck = millis();
  }

  delay(100);  // Small delay to prevent excessive polling
}

// Wi-Fi Setup Function
void setupWifi() {
  delay(10);
  M5.Lcd.printf("Connecting to %s", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    M5.Lcd.print(".");
  }
  M5.Lcd.println("\nConnected to Wi-Fi");

  publishLocationToFirebase("NIL"); // Start off with NIL
}

// Decrypt function with Boho
String decryptMessage(String encryptedMsg) {
  // Debug logging
  Serial.println("\nDecryption Debug:");
  Serial.print("Input message length: ");
  Serial.println(encryptedMsg.length());
  
  // Clear buffers
  memset(encData, 0, SIZE + MetaSize_ENC_PACK);
  memset(decData, 0, SIZE);
  
  // Convert hex string back to binary
  int encLen = encryptedMsg.length() / 2;
  for (int i = 0; i < encLen && i < SIZE + MetaSize_ENC_PACK; i++) {
    char hex[3] = { encryptedMsg[i * 2], encryptedMsg[i * 2 + 1], '\0' };
    encData[i] = strtol(hex, NULL, 16);
  }
  
  // Debug encrypted bytes
  Serial.print("Encrypted bytes: ");
  for (int i = 0; i < min(16, encLen); i++) {
    Serial.print((int)encData[i]);
    Serial.print(" ");
  }
  Serial.println();
  
  // Decrypt with Boho
  int decLen = boho.decryptPack(decData, encData, encLen);
  
  if (decLen <= 0) {
    Serial.println("Decryption failed!");
    return "0";
  }
  
  // Debug decrypted length
  Serial.print("Decrypted length: ");
  Serial.println(decLen);
  
  // Debug decrypted data
  Serial.print("Decrypted data: ");
  Serial.println(decData);
  
  // Return the decrypted string
  return String(decData);
}

// Function to update presence status display
void updatePresenceStatus() {
  String statusMessage = "Location: " + locationStack[stackTop] + "\n";
  statusMessage += "Bed: " + String(isPersonOnBed ? "Occupied" : "Empty") + "\n";
  statusMessage += "Sofa: " + String(isPersonOnSofa ? "Occupied" : "Empty");
  displayMessage(statusMessage.c_str());
}

// Function to update detailed location in Firebase
void updateDetailedLocation() {
  String detailedLocation = locationStack[stackTop];
  
  // Add more specific location details based on piezo sensors
  if (isPersonInBedroom && isPersonOnBed) {
    detailedLocation = "Bedroom on Bed";
  } else if (isPersonInLivingRoom && isPersonOnSofa) {
    detailedLocation = "Living Room on Sofa";
  }
  
  // Publish the detailed location to Firebase
  publishLocationToFirebase(detailedLocation);
  
  // Log the update
  Serial.println("Updated detailed location: " + detailedLocation);
}

// MQTT Callback Function
void callback(char* topic, byte* payload, unsigned int length) {
  String received = "";
  for (int i = 0; i < length; i++) {
    received += (char)payload[i];
  }

  // Check if this is one of the unencrypted piezo sensor topics
  if (String(topic) == topicBedroomBedPiezo || String(topic) == topicLivingRoomSofaPiezo) {
    // Handle unencrypted piezo sensor data
    Serial.println("\n=== Unencrypted Piezo Sensor Data ===");
    Serial.print("Topic: ");
    Serial.println(topic);
    Serial.print("Raw value: ");
    Serial.println(received);
    
    // Update presence flags based on the topic and value
    if (String(topic) == topicBedroomBedPiezo) {
      isPersonOnBed = (received == "1");
      Serial.print("Bed status: ");
      Serial.println(isPersonOnBed ? "Person on bed" : "Bed empty");
    } else if (String(topic) == topicLivingRoomSofaPiezo) {
      isPersonOnSofa = (received == "1");
      Serial.print("Sofa status: ");
      Serial.println(isPersonOnSofa ? "Person on sofa" : "Sofa empty");
    }
    
    // Update the display with current presence status
    updatePresenceStatus();
    
    // Update detailed location in Firebase
    updateDetailedLocation();
    
    Serial.println("=====================");
    return;
  } else {
    // Handle encrypted messages (existing code)
    Serial.println("\n=== Decrypted Message ===");
    Serial.print("Topic: ");
    Serial.println(topic);
    Serial.print("Encrypted message: ");
    Serial.println(received);
    
    String decryptedMsg = decryptMessage(received);
    Serial.print("Decrypted message: ");
    Serial.println(decryptedMsg);
    Serial.println("=====================");

    // Allow some time to decode
    delay(100);

    // Handle location change and fall detection via MQTT
    if (String(topic) == topicKitchenToiletFallDectector) {
      if (decryptedMsg == "1" && locationStack[stackTop] == "Kitchen") {
        reportFallToFirebase("Kitchen Toilet");
        displayMessage("Fall detected in Kitchen Toilet!");
        Serial.println("Fall detected in Kitchen Toilet!");
      }
    } else if (String(topic) == topicBedroomToiletFallDectector) {
      if (decryptedMsg == "1" && locationStack[stackTop] == "Bedroom") {
        reportFallToFirebase("Bedroom Toilet");
        displayMessage("Fall detected in Bedroom Toilet!");
        Serial.println("Fall detected in Bedroom Toilet!");
      }
    } else if (String(topic) == topicLivingRoom) {
      if (decryptedMsg == "1") {
        isPersonInLivingRoom = true;
        isPersonInBedroom = false;
        isPersonInKitchen = false;
        pushLocation("Living Room");
      }
    } else if (String(topic) == topicBedroom) {
      if (decryptedMsg == "1") {
        isPersonInBedroom = true;
        isPersonInLivingRoom = false;
        isPersonInKitchen = false;
        pushLocation("Bedroom");
      }
    } else if (String(topic) == topicKitchen) {
      if (decryptedMsg == "1") {
        isPersonInKitchen = true;
        isPersonInBedroom = false;
        isPersonInLivingRoom = false;
        pushLocation("Kitchen");
      }
    } else {
      String message = "Unknown Topic: " + String(topic);
      displayMessage(message.c_str());
    }
    
    // Update detailed location in Firebase
    updateDetailedLocation();
  }
}

// Publish location to Firebase
void publishLocationToFirebase(String loc) {
  if (!isFirebaseAuthenticated) {
    if (!authenticateFirebase()) {
      Serial.println("Failed to publish to Firebase: Not authenticated");
      return;
    }
  }

  String path = "/current_location/";
  if (fb.setString(path, loc)) {
    Serial.println("\n=====================");
    Serial.println("Location Data sent to Firebase");
    Serial.println("=====================\n");
  } else {
    Serial.println("\n=====================");
    Serial.println("Failed to send data to Firebase");
    Serial.println("=====================\n");
    isFirebaseAuthenticated = false;  // Reset authentication status on failure
  }
}

// Publish location to MQTT topic
void publishLocation() {
  if (stackTop >= 0) {
    client.publish(locationTopic, locationStack[stackTop].c_str());
  } else {
    client.publish(locationTopic, "No Location Found in Stack");
  }
}

// Push new location onto stack
void pushLocation(String loc) {
  // Prevent adding duplicate location to the stack
  if (stackTop >= 0 && locationStack[stackTop] == loc) {
    popLocation();
    return;
  }

  if (stackTop < 9) {
    stackTop++;
    locationStack[stackTop] = loc;
    publishLocation();  // Publish the new location to MQTT and Firebase
    displayLocation();  // Update LCD display
  }

  // Publish to Firebase
  publishLocationToFirebase(locationStack[stackTop]);
}

// Pop the top location off the stack
void popLocation() {
  if (stackTop > 0) {              // Prevent popping if it's only the "NIL" value
    locationStack[stackTop] = "";  // Clear the top location entry
    stackTop--;
    publishLocation();  // Publish the new location to MQTT and Firebase
    displayLocation();  // Update LCD display

    // Publish to Firebase
    publishLocationToFirebase(locationStack[stackTop]);
  }
}

// Display current location (only the top of the stack) on the M5StickC LCD
void displayLocation() {
  if (stackTop >= 0) {
    String message = "Current Location:\n" + locationStack[stackTop];
    displayMessage(message.c_str());
  } else {
    displayMessage("No Location Found");
  }
}

// Function to get current timestamp in format "YYYY-MM-DD HH:MM:SS"
String getCurrentTimestamp() {
  time_t epochTime = timeClient.getEpochTime();
  struct tm* ptm = gmtime((time_t*)&epochTime);

  char timestamp[25];
  sprintf(timestamp, "%04d-%02d-%02d %02d:%02d:%02d",
          ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday,
          ptm->tm_hour, ptm->tm_min, ptm->tm_sec);

  return String(timestamp);
}

// Function to report fall incidents to Firebase
void reportFallToFirebase(String location) {
  if (!isFirebaseAuthenticated) {
    if (!authenticateFirebase()) {
      Serial.println("Failed to report fall: Not authenticated");
      return;
    }
  }

  String path = "/fall_incidents/";
  String timestamp = getCurrentTimestamp();
  String incidentId = generateFallIncidentId();

  String jsonData = "{\"location\":\"" + location + 
                   "\",\"fall_detected\":true,\"timestamp\":\"" + timestamp + 
                   "\",\"incident_id\":\"" + incidentId + "\"}";

  if (fb.setJson(path + incidentId, jsonData)) {
    String message = "Fall incident reported!\nID: " + incidentId + "\nTime: " + timestamp;
    displayMessage(message.c_str());
    Serial.println("Fall incident reported to Firebase with ID: " + incidentId);
  } else {
    displayMessage("Failed to report fall!");
    Serial.println("Failed to report fall incident");
    isFirebaseAuthenticated = false;  // Reset authentication status on failure
  }
}

// Reconnect to MQTT server if disconnected
void reConnect() {
  while (!client.connected()) {
    displayMessage("Attempting MQTT connection...");
    Serial.println("Attempting MQTT connection...");
    String clientId = "UserLocationTracker-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str(), mqtt_username, mqtt_password)) {
      displayMessage("Connected to MQTT");
      Serial.println("Connected to MQTT");
      client.subscribe(topicBedroom);
      client.subscribe(topicLivingRoom);
      client.subscribe(topicKitchen);
      client.subscribe(topicKitchenToiletFallDectector);
      client.subscribe(topicBedroomToiletFallDectector);
      client.subscribe(topicBedroomBedPiezo);
      client.subscribe(topicLivingRoomSofaPiezo);
    } else {
      String message = "Failed to connect\nrc=" + String(client.state());
      displayMessage(message.c_str());
      delay(5000);
    }
  }
}