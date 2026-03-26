#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

/**
 * STANDALONE RECEIVER GATEWAY (Single-File)
 * -----------------------------------------
 * 1. Connects to Local WiFi.
 * 2. Connects to Local Mosquitto Broker on PC.
 * 3. Generates Virtual Data for testing.
 * 4. (Commented) BLE Scanning for Sensor Hub.
 */

// --- WiFi & MQTT Configuration ---
const char* ssid     = "Flybox_5587_EXT";      
const char* password = "6edgxcwetind";        
const char* mqttServer = "192.168.2.201";       // !!! CHANGE TO YOUR PC IP !!!
const int mqttpPort  = 1883;
const char* mqttTopic = "scs/enterprise/telemetry";

// --- Virtual Data Configuration ---
unsigned long lastVirtualSend = 0;
const int virtualInterval = 5000; 

// --- BLE Configuration ---
static BLEUUID serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
static BLEUUID charUUID("beb5483e-36e1-4688-b7f5-ea07361b26a8");

static BLEAdvertisedDevice* myDevice;
static bool doConnect = false;
static bool connected = false;
static bool doScan = false;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// --- MQTT Connection Helper ---
void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection to ");
    Serial.print(mqttServer);
    Serial.println("...");
    
    String clientId = "ESP32Gateway-";
    clientId += String(random(0xffff), HEX);
    
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("MQTT Connected Successfully!");
    } else {
      Serial.print("MQTT Failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" - Retrying in 5s");
      delay(5000);
    }
  }
}

// --- Virtual Data Generator ---
void sendVirtualData() {
  if (millis() - lastVirtualSend > virtualInterval) {
    lastVirtualSend = millis();

    JsonDocument doc;
    doc["temperature"] = random(200, 300) / 10.0;
    doc["humidity"] = random(40, 60);
    doc["mq7_raw"] = random(200, 800);
    doc["status"] = "VIRTUAL_TEST";

    String jsonPayload;
    serializeJson(doc, jsonPayload);

    Serial.println("Generated Virtual Data: " + jsonPayload);

    if (mqttClient.publish(mqttTopic, jsonPayload.c_str())) {
        Serial.println(">>> Sent to Mosquitto");
    } else {
        Serial.println("!!! Publish Failed !!!");
    }
  }
}

// --- BLE Callbacks ---
static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    String jsonPayload = "";
    for (int i = 0; i < length; i++) jsonPayload += (char)pData[i];
    if (mqttClient.publish(mqttTopic, jsonPayload.c_str())) Serial.println("BLE Forwarded: " + jsonPayload);
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {}
  void onDisconnect(BLEClient* pclient) { connected = false; doScan = true; }
};

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
    }
  }
};

bool connectToServer() {
    BLEClient* pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallback());
    if (!pClient->connect(myDevice)) return false;
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) { pClient->disconnect(); return false; }
    BLERemoteCharacteristic* pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) { pClient->disconnect(); return false; }
    if(pRemoteCharacteristic->canNotify()) pRemoteCharacteristic->registerForNotify(notifyCallback);
    connected = true;
    return true;
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nWiFi Connected!");

  mqttClient.setServer(mqttServer, mqttpPort);

  // BLE is initialized but scan is not started by default to save resources
  BLEDevice::init("ESP32_GATEWAY");
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    if (!mqttClient.connected()) reconnectMQTT();
    mqttClient.loop();
    sendVirtualData(); // Generate and send data every 5s
  }
  
  // Future BLE logic
  if (doConnect) {
    if (connectToServer()) Serial.println("BLE Connected!");
    doConnect = false;
  }
  
  delay(100);
}
