// utils.h
#ifndef UTILS_H
#define UTILS_H

#include <WebServer.h>
#include <PubSubClient.h>
#include <Preferences.h>

// Constants
#define MAX_SSID_LENGTH     32
#define MAX_PASSWORD_LENGTH 64
#define UUID_LENGTH         37
#define DEVICEID_LENGTH     37

// Gen 2 ESP32-C3 Pin Definitions
#define STATUS_LED_BLUE 1      // Blue LED control (Active LOW)
#define RELAY_CONTROL 5        // Relay control via optocoupler
#define SWITCH_DETECTION 4     // AC switch detection input (Active LOW when switch on)

// Legacy compatibility
#define SHELLY_BUILTIN_LED STATUS_LED_BLUE

// Configuration Structure
struct Config {
  char ssid[MAX_SSID_LENGTH];
  char password[MAX_PASSWORD_LENGTH];
  char uuid[UUID_LENGTH];
  char deviceId[DEVICEID_LENGTH];
  uint32_t checksum; // For data integrity
};

// External Variables
extern Config storedConfig;
extern PubSubClient mqttClient;
extern Preferences preferences;
extern bool relayState;

// MQTT Topics
extern const char* mqtt_publish_topic;
extern const char* mqtt_subscribe_topic;

extern bool doesUserExist;

// Function Prototypes
void sendResponse(WebServer &server, int statusCode, const String &content);
void flashLED();
bool connectToWiFi(const String& ssid, const String& password);
bool saveUserAndWifiCreds(const String& ssid, const String& password, const String& uuid, const String& deviceId);
bool checkForWifiAndUser();
uint32_t calculateChecksum(const uint8_t* data, size_t length);
void clearEEPROM();

// MQTT Function Prototypes
void mqttCallback(char* topic, byte* payload, unsigned int length);
bool connectToMQTT();
void publishMessage(const char* topic, const char* message);
String getUserId();
String getDeviceId();
String getTopic();
void initRelay();
void toggleRelay();
void turnRelayOff();
void turnRelayOn();
void publishRelayStateupdate(const char* updateType);



#endif 
