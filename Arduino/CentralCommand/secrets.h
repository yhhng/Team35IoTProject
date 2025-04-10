// secrets.h

// Wi-Fi credentials
#define WIFI_SSID "Goh"  // Your Wi-Fi SSID
#define WIFI_PASSWORD "4357d3374d"  // Your Wi-Fi password

// MQTT credentials
#define mqtt_server "192.168.1.19"  // MQTT server address
#define mqtt_username "yuheng"      // MQTT username
#define mqtt_password "123456"      // MQTT password

// MQTT topics
#define topicLivingRoom "home/livingroom/door/ir_sensor"
#define topicBedroom "home/bedroom/door/ir_sensor"  
#define topicKitchen "home/kitchen/door/ir_sensor"
#define topicKitchenToiletFallDectector "home/kitchen/toilet/fall_detected"
#define topicBedroomToiletFallDectector "home/bedroom/toilet/fall_detected"
#define locationTopic "home/location/current"
#define topicBedroomBedPiezo "home/bedroom/bed/piezzo_sensor_data"
#define topicLivingRoomSofaPiezo "home/livingroom/sofa/piezzo_sensor_data"

// PSK identity & key
#define pskIdent "hint"
#define psKey "BAD123"

// Firebase credentials
#define REFERENCE_URL FIREBASE_HOST
#define FIREBASE_HOST "https://iot-project-fall-detection-default-rtdb.firebaseio.com/"  // Replace with your Firebase project URL

// Firebase Authentication
const char* FIREBASE_AUTH_TOKEN = "WBfTJ3MZboKy1KDq3amGU5a4uXIEXu4jOIOOlfVI";  // Replace with your Firebase auth token

