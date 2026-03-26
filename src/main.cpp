#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include "config.h"

// --- BLE Configuration ---
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("BLE Client Connected!");
    };
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("BLE Client Disconnected!");
      BLEDevice::startAdvertising();
    }
};

// --- Sensor Settings ---
#define DHTPIN 4
#define DHTTYPE DHT11
#define MQ7PIN 34
#define CLIENT_ID 105 

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  
  // 1. Initialize BLE
  BLEDevice::init("ESP32_SENSOR_HUB");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_NOTIFY 
                    );
  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();
  BLEDevice::startAdvertising();
  Serial.println("BLE Advertising Started...");

  /* 2. Local WiFi (Optional parallel connection) - Commented for BLE testing
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 5000) {
    delay(500);
    Serial.print(".");
  }
  */

  // 3. Sensor Init
  pinMode(MQ7PIN, INPUT);
  analogSetAttenuation(ADC_11db);
  dht.begin();
}

void loop() {
  Serial.println("--- Loop Start ---");
  
  Serial.print("Reading Humidity... ");
  float h = dht.readHumidity();
  Serial.print("Reading Temperature... ");
  float t = dht.readTemperature();
  Serial.println("Done.");
  
  Serial.print("Sampling MQ7... ");
  long sum = 0;
  for(int i=0; i<32; i++) {
    sum += analogRead(MQ7PIN);
    delay(10); // Minimal delay between samples
  }
  int mq7Value = sum / 32;
  Serial.println("Done.");

  Serial.println("Final JSON Values: T=" + String(t) + " H=" + String(h) + " MQ7=" + String(mq7Value));

  if (isnan(t) || isnan(h)) {
    Serial.println("!!! DHT Read Error - Check wiring on Pin 4 !!!");
    delay(2000);
    return;
  }

  // Prepare Payload
  StaticJsonDocument<200> doc;
  doc["client_id"] = CLIENT_ID;
  doc["tmp"] = t;
  doc["hum"] = h; // Added humidity to JSON
  doc["gaz_level"] = mq7Value;
  doc["ai_status"] = (mq7Value > 1500 || t > 35) ? "Warning" : "Normal";

  String jsonPayload;
  serializeJson(doc, jsonPayload);
  Serial.println("JSON Payload: " + jsonPayload);

  // Send via BLE Notify
  if (deviceConnected) {
    pCharacteristic->setValue(jsonPayload.c_str());
    pCharacteristic->notify();
    Serial.println(">>> BLE NOTIFIED SUCCESS <<<");
  } else {
    Serial.println(">>> BLE Client NOT connected - Advertising... <<<");
  }

  /* Keep original Supabase logic if WiFi is available - Commented for BLE testing
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(supabaseUrl); 
    http.addHeader("apikey", supabaseKey);
    http.addHeader("Authorization", "Bearer " + String(supabaseKey));
    http.addHeader("Content-Type", "application/json");
    int httpCode = http.POST(jsonPayload);
    http.end();
  }
  */

  delay(5000); 
}
