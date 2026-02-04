// utils.cpp
#include "utils.h"
#include "secrets.h"
#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <string.h>
#include <stdlib.h>
#include <cmath>
#include <ArduinoJson.h>

// Global Variables
Config storedConfig;
bool relayState = false; // true = ON, false = OFF
Preferences preferences;


String getPubTopic(){
  String userId = getUserId();
  String deviceId = getDeviceId();

  Serial.println("Getting pub topic String");
  Serial.println(userId);


  String topic = "/toDaemon/" + userId + "/" + deviceId;

  Serial.println(topic);

  return topic;

}

String getSubTopic(){
  String userId = getUserId();
  String deviceId = getDeviceId();

  String topic = "/toDevice/" + userId + "/" + deviceId;

  Serial.println("Getting sub topic String");
  Serial.println(userId);

  Serial.println(topic);


  return topic;

}



const char* mqtt_publish_topic    = getPubTopic().c_str();
const char* mqtt_subscribe_topic  = getSubTopic().c_str();

// Initialize MQTT Client with a plain WiFiClient
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Define the global state variable
bool doesUserExist = false;

// Calculate Checksum for Data Integrity
uint32_t calculateChecksum(const uint8_t* data, size_t length) {
  uint32_t checksum = 0;
  for (size_t i = 0; i < length; ++i) {
    checksum += data[i];
  }
  return checksum;
}

// Save User and WiFi Credentials to Preferences (NVS)
bool saveUserAndWifiCreds(const String& ssid, const String& password, const String& uuid, const String& deviceId) {
  // Clear the Config struct
  memset(&storedConfig, 0, sizeof(Config));

  // Copy SSID, password, UUID, and DeviceID into the struct
  ssid.toCharArray(storedConfig.ssid, MAX_SSID_LENGTH);
  password.toCharArray(storedConfig.password, MAX_PASSWORD_LENGTH);
  uuid.toCharArray(storedConfig.uuid, UUID_LENGTH);
  deviceId.toCharArray(storedConfig.deviceId, DEVICEID_LENGTH);

  // Ensure null-termination (just to be safe)
  storedConfig.ssid[MAX_SSID_LENGTH - 1] = '\0';
  storedConfig.password[MAX_PASSWORD_LENGTH - 1] = '\0';
  storedConfig.uuid[UUID_LENGTH - 1] = '\0';
  storedConfig.deviceId[DEVICEID_LENGTH - 1] = '\0';

  // ✅ Verify each field after copying
  if (String(storedConfig.ssid) != ssid) {
    Serial.println("[ERROR] SSID mismatch after copying to storedConfig.");
    return false;
  }

  if (String(storedConfig.password) != password) {
    Serial.println("[ERROR] Password mismatch after copying to storedConfig.");
    return false;
  }

  if (String(storedConfig.uuid) != uuid) {
    Serial.println("[ERROR] UUID mismatch after copying to storedConfig.");
    return false;
  }

  if (String(storedConfig.deviceId) != deviceId) {
    Serial.println("[ERROR] Device ID mismatch after copying to storedConfig.");
    return false;
  }

  Serial.println("[SUCCESS] All fields verified successfully.");

  // Save to Preferences (NVS) - more reliable than EEPROM
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.putString("uuid", uuid);
  preferences.putString("deviceId", deviceId);
  preferences.putBool("configured", true);

  Serial.println("[NVS] Configuration saved successfully.");
  return true;
}

// Check for WiFi and User Configuration in Preferences (NVS)
bool checkForWifiAndUser() {
  // Check if device has been configured
  if (!preferences.getBool("configured", false)) {
    Serial.println("[NVS] No configuration found.");
    doesUserExist = false;
    return false;
  }

  // Read configuration from Preferences
  String ssid = preferences.getString("ssid", "");
  String password = preferences.getString("password", "");
  String uuid = preferences.getString("uuid", "");
  String deviceId = preferences.getString("deviceId", "");

  // Check if all required fields are non-empty
  if (ssid.isEmpty() || password.isEmpty() || uuid.isEmpty() || deviceId.isEmpty()) {
    Serial.println("[NVS] Configuration found, but one or more fields are empty.");
    doesUserExist = false;
    return false;
  }

  // Store in global config struct
  memset(&storedConfig, 0, sizeof(Config));
  ssid.toCharArray(storedConfig.ssid, MAX_SSID_LENGTH);
  password.toCharArray(storedConfig.password, MAX_PASSWORD_LENGTH);
  uuid.toCharArray(storedConfig.uuid, UUID_LENGTH);
  deviceId.toCharArray(storedConfig.deviceId, DEVICEID_LENGTH);

  Serial.println("[NVS] Valid configuration found.");
  Serial.printf("SSID: %s\n", storedConfig.ssid);
  Serial.printf("UUID: %s, Length: %d\n", storedConfig.uuid, strlen(storedConfig.uuid));
  Serial.printf("DeviceID: %s, Length: %d\n", storedConfig.deviceId, strlen(storedConfig.deviceId));

  doesUserExist = true;
  return true;
}

// Send HTTP Response with CORS Headers
void sendResponse(WebServer &server, int statusCode, const String &content) {
// Debug output
Serial.printf("[DEBUG] Sending response with status code %d, content: %s\n", statusCode, content.c_str());

// Set headers for CORS
server.sendHeader("Access-Control-Allow-Origin", "*");
server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
server.sendHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");

// Send the response
server.send(statusCode, "application/json", content);
}

// Flash LED (Example Function)
void flashLED() {
  digitalWrite(STATUS_LED_BLUE, LOW);  // Turn LED on
  delay(500);
  digitalWrite(STATUS_LED_BLUE, HIGH); // Turn LED off
  delay(500);
}

// Connect to WiFi with Given Credentials
bool connectToWiFi(const String& ssid, const String& password) {
  Serial.println("Attempting to connect to WiFi...");

  // Turn on the LED (solid, indicating connection attempt)
  digitalWrite(STATUS_LED_BLUE, LOW);

  // Disconnect any previous connection
  WiFi.disconnect();
  delay(100);

  // Start WiFi connection
  WiFi.begin(ssid.c_str(), password.c_str());

  // Wait until connected or timeout (15 seconds)
  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000) {
    delay(500);
    Serial.print(".");
    Serial.printf(" Current WiFi.status(): %d\n", WiFi.status());
  }

  // Check connection status
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.printf("Signal Strength: %d dBm\n", WiFi.RSSI());
    digitalWrite(STATUS_LED_BLUE, HIGH);

    doesUserExist = true;
    return true;
  } else {
    Serial.println("\nFailed to connect to WiFi.");
    Serial.printf("WiFi.status(): %d\n", WiFi.status());

    doesUserExist = false;
    return false;
  }
}

// Clear NVS Storage (Use with Caution)
void clearEEPROM() {
  preferences.clear(); // Clear all keys in the "guardian" namespace
  Serial.println("NVS storage cleared.");
  ESP.restart();
}

void restart(){
  ESP.restart();
}

// MQTT Callback Function to Handle Incoming Messages
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.println(topic);

  // Convert payload to a string
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print("Message: ");
  Serial.println(message);

  // Parse the JSON message
  StaticJsonDocument<200> doc; // Adjust buffer size as needed

  DeserializationError error = deserializeJson(doc, message);
  if (error) {
    Serial.print("Failed to parse JSON: ");
    Serial.println(error.c_str());
    return;
  }

  // Check for the "power" key in JSON
  if (doc.containsKey("power")) {
    bool powerState = doc["power"];
    
    if (powerState) {
      turnRelayOn();
      Serial.println("Relay turned ON via MQTT");
    } else {
      turnRelayOff();
      Serial.println("Relay turned OFF via MQTT");
    }

    publishRelayStateupdate("state_change");
  } else {
    Serial.println("JSON does not contain 'power' key.");
  }
  if(doc.containsKey("command")){
    String command = doc["command"];
    Serial.println("commmand recieved");

    if(command == "restart"){
      restart();
    } 
    else if (command == "clearEEPROM"){
      clearEEPROM();
    }

  }
}

// Connect to MQTT Broker
bool connectToMQTT() {
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(512);
  int randomSessionId = random(1, 201); // Random number from 1 to 200

  String clientId = "ESP8266Client-" + String(WiFi.macAddress()) + "-" + String(storedConfig.deviceId) + "-" + String(randomSessionId);
  String pubTopic = getPubTopic().c_str();
  String subTopic = getSubTopic().c_str();

  bool connected = mqttClient.connect(clientId.c_str());

  Serial.println("about to connect to mqtt broker with this topic");
  Serial.println(getSubTopic().c_str());

  if (connected) {
    Serial.println("Connected to MQTT Broker.");
    mqttClient.subscribe(getSubTopic().c_str());
    Serial.print("Subscribed to topic: ");
    Serial.println(getSubTopic().c_str());

    publishRelayStateupdate("init");

    // mqttClient.publish(topic.c_str(), "ESP8266 Connected");
    return true;
  } else {
    Serial.print("Failed to connect to MQTT Broker, state: ");
    Serial.println(mqttClient.state());
    return false;
  }
}

// Publish Message to MQTT Broker
void publishMessage(const char* topic, const char* message) {

  if (mqttClient.publish(topic, message)) {
    Serial.print("Message published to topic ");
    Serial.print(topic);
    Serial.print(": ");
    Serial.println(message);
  } else {
    Serial.print("Failed to publish message to topic ");
    Serial.println(topic);
  }
}

String getUserId() {
  String uuid = preferences.getString("uuid", "");
  if (uuid.isEmpty()) {
    Serial.println("[NVS] UUID field is empty.");
  }
  return uuid;
}


String getDeviceId() {
  String deviceId = preferences.getString("deviceId", "");
  if (deviceId.isEmpty()) {
    Serial.println("[NVS] DeviceID field is empty.");
  }
  return deviceId;
}

void initRelay() {
  pinMode(RELAY_CONTROL, OUTPUT);

  // Read last state from EEPROM
  relayState = LOW; // Default to OFF
  // Apply the last state to the relay
  digitalWrite(RELAY_CONTROL, LOW);
}

void toggleRelay() {
  // Toggle state
  relayState = !relayState;

  // Apply state to GPIO4
  digitalWrite(RELAY_CONTROL, relayState ? HIGH : LOW);

  // Persist to EEPROM if required
  
}

void turnRelayOn() {
  relayState = true;
  digitalWrite(RELAY_CONTROL, HIGH);
 
}

void turnRelayOff() {
  relayState = false;
  digitalWrite(RELAY_CONTROL, LOW);
 
}

void publishRelayStateupdate(const char* updateType){

  String userId = getUserId();
  String deviceId = getDeviceId();

  Serial.println("Publishing relay state update...");
  Serial.print("User ID: ");
  Serial.println(userId);
  Serial.print("Device ID: ");
  Serial.println(deviceId);


  DynamicJsonDocument doc(512);
  doc["relay_update"] = true;
  doc["update_type"] = updateType;  // "init", "state_change", or "heartbeat"
  doc["relay_state"] = relayState;  // current state: true or false
  doc["device_type"] = "relay";
  doc["status"] = "online";
  // Include the user and device IDs in the message.
  doc["user_id"] = userId;
  doc["device_id"] = deviceId;
  // Optionally add a timestamp (using millis() here)
  doc["timestamp"] = millis();

  char buffer[256];
  serializeJson(doc, buffer, sizeof(buffer));
  
  // Use the dynamic topic based on stored user/device IDs.
  publishMessage(getPubTopic().c_str(), buffer);
}