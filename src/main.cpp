#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <time.h>

// Include your custom headers
#include "utils/utils.h"
#include "api/api.h"

// Web server on port 80
WebServer server(80);

// Timer variables
unsigned long lastPublishTime = 0;
const unsigned long publishInterval = 30000;

unsigned long lastReconnectAttempt = 5000;
const unsigned long reconnectInterval = 10000;

unsigned long lastWifiRetryAttempt = 0;
const unsigned long wifiRetryInterval = 30000;


void startAccessPoint() {
  Serial.println("[AP MODE] Starting access point...");
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("Guardian_GMS_Relay", "");
  Serial.println("[AP MODE] ✓ Access point started");
  Serial.print("[AP MODE] SSID: Guardian_GMS_Relay (no password)");
  Serial.print("[AP MODE] IP address: ");
  Serial.println(WiFi.softAPIP());
  digitalWrite(STATUS_LED_BLUE, LOW); // LED on (active LOW)
}

void synchronizeTime() {
  Serial.println("Synchronizing system time...");
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) {
    Serial.println("Waiting for time synchronization...");
    delay(1000);
  }

  char timeStr[64];
  strftime(timeStr, sizeof(timeStr), "Current time: %A, %B %d %Y %H:%M:%S", &timeinfo);
  Serial.println(timeStr);
}

void setup() {
  Serial.begin(115200);
  delay(1000); // Give serial time to initialize on native USB
  Serial.println("\n\n===========================================");
  Serial.println("Guardian 1PM - Gen 2 ESP32-C3 Firmware");
  Serial.println("===========================================");
  Serial.println("Initializing...");


  // Initialize Preferences (replaces EEPROM)
  Serial.println("[INIT] Opening NVS storage...");
  preferences.begin("guardian", false); // namespace "guardian", read-write mode
  Serial.println("[INIT] NVS storage ready");

  Serial.println("[INIT] Configuring GPIO pins...");
  pinMode(STATUS_LED_BLUE, OUTPUT);
  digitalWrite(STATUS_LED_BLUE, HIGH); // LED off (active LOW)
  Serial.print("[INIT] Blue LED (GPIO");
  Serial.print(STATUS_LED_BLUE);
  Serial.println(") configured - OFF");

  initRelay();
  Serial.print("[INIT] Relay (GPIO");
  Serial.print(RELAY_CONTROL);
  Serial.println(") configured - OFF");



  Serial.println("\n[WIFI] Checking for stored credentials...");
  if (checkForWifiAndUser()) {
      Serial.println("[WIFI] Credentials found! Attempting connection...");
      if (connectToWiFi(String(storedConfig.ssid), String(storedConfig.password))) {
          Serial.println("[WIFI] ✓ Connected to WiFi successfully.");
          digitalWrite(STATUS_LED_BLUE, HIGH); // LED off
      } else {
          Serial.println("[WIFI] ✗ WiFi connection failed, starting Access Point...");
          startAccessPoint();
      }
  } else {
      Serial.println("[WIFI] No credentials found, starting Access Point...");
      startAccessPoint();
  }

  if (WiFi.status() == WL_CONNECTED) {
      Serial.println("[TIME] Synchronizing with NTP...");
      synchronizeTime();
  } else {
      Serial.println("[TIME] Skipping - WiFi not connected");
  }

  Serial.println("[WEB] Starting web server on port 80...");
  setupApiRoutes(server);
  server.begin();
  Serial.println("[WEB] ✓ Web server started");

  if (doesUserExist && WiFi.status() == WL_CONNECTED) {
      Serial.println("[MQTT] Connecting to broker...");
      if (connectToMQTT()) {
          Serial.println("[MQTT] ✓ Connected successfully");
      } else {
          Serial.println("[MQTT] ✗ Connection failed");
      }
  } else {
      Serial.println("[MQTT] Skipping - no credentials or WiFi");
  }

  Serial.println("\n===========================================");
  Serial.println("SETUP COMPLETE - Entering main loop");
  Serial.println("===========================================\n");
}



void loop() {
  server.handleClient();
  unsigned long currentMillis = millis();

  // WiFi Reconnect
  if (doesUserExist && (WiFi.status() != WL_CONNECTED)) {
      if (currentMillis - lastWifiRetryAttempt > wifiRetryInterval) {
          lastWifiRetryAttempt = currentMillis;
          Serial.println("Attempting to reconnect to WiFi...");
          if (connectToWiFi(String(storedConfig.ssid), String(storedConfig.password))) {
              Serial.println("Reconnected to WiFi successfully.");
              digitalWrite(STATUS_LED_BLUE, HIGH);
              synchronizeTime();

              if (!mqttClient.connected()) {
                  if (connectToMQTT()) {
                      Serial.println("MQTT Connected Successfully after WiFi reconnect.");
                  } else {
                      Serial.println("Failed to Connect to MQTT Broker after WiFi reconnect.");
                  }
              }
          } else {
              Serial.println("WiFi reconnect attempt failed.");
          }
      }
  }

  // MQTT Reconnect
  if (doesUserExist && (WiFi.status() == WL_CONNECTED)) {
      if (!mqttClient.connected()) {
          if (currentMillis - lastReconnectAttempt > reconnectInterval) {
              lastReconnectAttempt = currentMillis;
              Serial.println("Reconnecting to MQTT Broker...");
              if (connectToMQTT()) {
                  Serial.println("Reconnected to MQTT Broker.");
              } else {
                  Serial.println("Failed to connect to broker");
              }
          }
      }
      mqttClient.loop();

      // Heartbeat - publish relay state every 60 seconds
      static unsigned long lastHeartbeat = 0;
      if (currentMillis - lastHeartbeat >= 60000) {  // 60000 ms = 1 minute
          lastHeartbeat = currentMillis;
          Serial.println("[HEARTBEAT] Publishing relay state...");
          publishRelayStateupdate("heartbeat");
          Serial.print("[RELAY STATE] Current: ");
          Serial.println(relayState ? "ON" : "OFF");
      }

      delay(100);
  }

}
