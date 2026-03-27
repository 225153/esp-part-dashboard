#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

/**
 * STANDALONE RECEIVER GATEWAY
 * ---------------------------
 * 1. Connects to Local WiFi
 * 2. Connects to Mosquitto Broker on PC (192.168.2.207)
 * 3. Scans & Connects to SENDER via BLE (ESP32_SENSOR_HUB)
 * 4. Forwards data to MQTT scs/enterprise/telemetry
 */

// --- WiFi & MQTT Configuration ---
const char* ssid     = "Flybox_5587_EXT";      
const char* password = "6edgxcwetind";        
const char* mqttServer = "192.168.2.207";       
const int mqttpPort  = 1883;
const char* mqttTopic = "scs/enterprise/telemetry";

// --- BLE Configuration ---
static BLEUUID serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
static BLEUUID charUUID("beb5483e-36e1-4688-b7f5-ea07361b26a8");

static BLEAdvertisedDevice* myDevice;
static bool doConnect = false;
static bool connected = false;
static bool doScan = true;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// --- BLE Callback ---
static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    String jsonPayload = "";
    for (int i = 0; i < length; i++) jsonPayload += (char)pData[i];
    
    Serial.println("RECEIVE: " + jsonPayload);

    if (mqttClient.publish(mqttTopic, jsonPayload.c_str())) {
        Serial.println(">>> Sent to Mosquitto");
    }
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    connected = true;
    Serial.println(">>> BLE Connected to Sender!");
  }
  void onDisconnect(BLEClient* pclient) {
    connected = false;
    doScan = true;
    Serial.println(">>> Lost BLE Connection to Sender.");
  }
};

bool connectToServer() {
    Serial.print("Connecting to BLE Device... ");
    BLEClient* pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallback());

    if (!pClient->connect(myDevice)) {
        Serial.println("FAILED to connect.");
        return false;
    }

    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
        Serial.print("Failed to find Service: ");
        Serial.println(serviceUUID.toString().c_str());
        pClient->disconnect();
        return false;
    }

    BLERemoteCharacteristic* pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
        Serial.print("Failed to find Characteristic: ");
        Serial.println(charUUID.toString().c_str());
        pClient->disconnect();
        return false;
    }

    if(pRemoteCharacteristic->canNotify()) {
        pRemoteCharacteristic->registerForNotify(notifyCallback);
        Serial.println("Notification Registered!");
    } else {
        Serial.println("Characteristic does not support notify.");
    }

    return true;
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Scan Found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // Look for name ESP32_SENSOR_HUB OR the specific Service UUID
    if (advertisedDevice.getName() == "ESP32_SENSOR_HUB" || 
       (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID))) {
      
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = false;
      Serial.println(">>> Target Device Identified! Stopping scan...");
    }
  }
};

// --- MQTT Helper ---
void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection to ");
    Serial.print(mqttServer);
    Serial.println("...");
    String clientId = "ESP32Receiver-";
    clientId += String(random(0xffff), HEX);
    
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("MQTT Connected Successfully!");
    } else {
      Serial.print("MQTT Failed, rc=");
      Serial.println(mqttClient.state());
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- GATEWAY STARTING ---");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nWiFi Connected!");

  mqttClient.setServer(mqttServer, mqttpPort);

  BLEDevice::init("ESP32_RECEIVER");
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  
  Serial.println("Starting BLE Scan...");
  pBLEScan->start(5, false);
}

void loop() {
  if (doConnect) {
    if (connectToServer()) {
        Serial.println("Success! Now relaying data.");
    } else {
        Serial.println("Failed to connect, will retry scan.");
        doScan = true;
    }
    doConnect = false;
  }

  if (WiFi.status() == WL_CONNECTED) {
    if (!mqttClient.connected()) reconnectMQTT();
    mqttClient.loop();
  }

  if (!connected && doScan) {
    Serial.println("Restarting BLE Scan...");
    BLEDevice::getScan()->start(5, false);
    delay(1000);
  }

  delay(100);
}
