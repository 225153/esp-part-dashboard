#include <Arduino.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

/**
 * STANDALONE SENDER NODE
 * ----------------------
 * 1. Reads DHT11 (Temp/Hum) and MQ7 (Gas)
 * 2. Displays data on Serial Monitor
 * 3. Sends data via BLE to Receiver
 */

// --- BLE Configuration ---
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

// --- Sensor Settings ---
#define DHTPIN 4
#define DHTTYPE DHT11
#define MQ7PIN 34

DHT dht(DHTPIN, DHTTYPE);

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println(">>> Receiver Connected!");
    };
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println(">>> Receiver Disconnected!");
      BLEDevice::startAdvertising(); // Restart advertising to allow reconnect
    }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Sender Node Booting...");

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
  Serial.println("BLE Advertising Started (ESP32_SENSOR_HUB)...");

  // 2. Sensor Init
  pinMode(MQ7PIN, INPUT);
  analogSetAttenuation(ADC_11db);
  dht.begin();
  Serial.println("Sensors Initialized.");
}

void loop() {
  Serial.println("\n--- Sampling Sensors ---");
  
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  
  // MQ7 Sampling
  long sum = 0;
  for(int i=0; i<32; i++) {
    sum += analogRead(MQ7PIN);
    delay(10);
  }
  int mq7Value = sum / 32;

  if (isnan(t) || isnan(h)) {
    Serial.println("!!! DHT Read Error !!! Check wiring.");
    delay(2000);
    return;
  }

  // 1. Display on Serial Monitor
  Serial.print("Temp: "); Serial.print(t); Serial.println(" C");
  Serial.print("Hum:  "); Serial.print(h); Serial.println(" %");
  Serial.print("Gas:  "); Serial.println(mq7Value);

  // 2. Prepare JSON
  StaticJsonDocument<200> doc;
  doc["temperature"] = t;
  doc["humidity"] = h;
  doc["mq7_raw"] = mq7Value;
  doc["status"] = "OK";

  String jsonString;
  serializeJson(doc, jsonString);

  // 3. Send via BLE
  if (deviceConnected) {
    pCharacteristic->setValue(jsonString.c_str());
    pCharacteristic->notify();
    Serial.println(">>> Packet Sent to Receiver");
  } else {
    Serial.println("... Waiting for Receiver to connect...");
  }

  delay(5000); // Wait 5 seconds
}
