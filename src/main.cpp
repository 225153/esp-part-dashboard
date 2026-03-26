#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include "config.h"

// --- Configuration ---
// SSID and Supabase credentials are now moved to include/config.h

// Sensor Settings
#define DHTPIN 4
#define DHTTYPE DHT11
#define MQ7PIN 34
#define CLIENT_ID 105 // Unique ID for this specific ESP32

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  
  // WiFi Connection
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");

  // Sensor Init
  pinMode(MQ7PIN, INPUT);
  analogSetAttenuation(ADC_11db);
  analogReadResolution(12);
  dht.begin();
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    // 1. Read Sensors
    float t = dht.readTemperature();
    
    long sum = 0;
    for(int i=0; i<32; i++) sum += analogRead(MQ7PIN);
    int mq7Value = sum / 32;

    if (isnan(t)) {
      Serial.println("Failed to read DHT!");
      return;
    }

    // 2. Prepare JSON Payload
    // Match your schema: client_id, tmp, gaz_level, ai_status
    StaticJsonDocument<200> doc;
    doc["client_id"] = 999;
    doc["tmp"] = t;
    doc["gaz_level"] = mq7Value;
    doc["ai_status"] = (mq7Value > 1500 || t > 35) ? "Warning" : "Normal";
    doc["decision_reason"] = (mq7Value > 1500) ? "High Gas" : (t > 35 ? "High Temp" : "Safe");

    String jsonPayload;
    serializeJson(doc, jsonPayload);

    // 3. Send HTTPS POST Request
    HTTPClient http;
    // Note: use begin(url) for ESP32. We use skip certification check for ease of use.
    http.begin(supabaseUrl); 
    
    // Headers required by Supabase
    http.addHeader("apikey", supabaseKey);
    http.addHeader("Authorization", "Bearer " + String(supabaseKey));
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Prefer", "return=minimal"); // Standard for Supabase inserts

    int httpResponseCode = http.POST(jsonPayload);

    if (httpResponseCode > 0) {
      Serial.print("Data Sent! Status Code: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("Error sending POST: ");
      Serial.println(http.errorToString(httpResponseCode).c_str());
    }

    http.end();
  }

  delay(100000); // Send every 100 seconds
}