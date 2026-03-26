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
 * This code runs on the SECOND ESP32.
 * 1. Connects to Local WiFi.
 * 2. Connects to Local Mosquitto Broker on PC.
 * 3. Scans for "ESP32_SENSOR_HUB" BLE Server.
 * 4. Forwards sensor JSON to MQTT topic: scs/enterprise/telemetry
 */

// --- WiFi & MQTT Configuration ---
const char* ssid     = "Flybox_5587_EXT";      // Your WiFi SSID
const char* password = "6edgxcwetind";        // Your WiFi Password
const char* mqttServer = "192.168.1.10";       // !!! CHANGE THIS TO YOUR PC IP !!!
const int mqttpPort  = 1883;
const char* mqttTopic = "scs/enterprise/telemetry";

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
    
    // Create a random client ID
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

// --- BLE Notification Handler ---
static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    String jsonPayload = "";
    for (int i = 0; i < length; i++) {
        jsonPayload += (char)pData[i];
    }
    Serial.println("BLE Received: " + jsonPayload);

    // Forward to Local Mosquitto
    if (!mqttClient.connected()) reconnectMQTT();
    
    if (mqttClient.publish(mqttTopic, jsonPayload.c_str())) {
        Serial.println(">>> Forwarded to Mosquitto: " + String(mqttTopic));
    } else {
        Serial.println("!!! Failed to Publish to MQTT !!!");
    }
}

// --- BLE Client Callbacks ---
class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {}
  void onDisconnect(BLEClient* pclient) {
    connected = false;
    doScan = true;
    Serial.println("BLE Disconnected from Hub");
  }
};

bool connectToServer() {
    Serial.print("Connecting to Hub at ");
    Serial.println(myDevice->getAddress().toString().c_str());
    
    BLEClient*  pClient  = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallback());
    pClient->connect(myDevice);

    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      pClient->disconnect();
      return false;
    }

    BLERemoteCharacteristic* pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      pClient->disconnect();
      return false;
    }

    if(pRemoteCharacteristic->canNotify())
      pRemoteCharacteristic->registerForNotify(notifyCallback);

    connected = true;
    return true;
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = false;
    }
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Gateway Booting...");

  // 1. Connect WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");

  // 2. Setup MQTT
  mqttClient.setServer(mqttServer, mqttpPort);

  // 3. Setup BLE
  BLEDevice::init("ESP32_GATEWAY");
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
}

void loop() {
  // BLE Connection logic
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("BLE Connection Established!");
    }
    doConnect = false;
  }

  // Keep MQTT alive
  if (WiFi.status() == WL_CONNECTED) {
    if (!mqttClient.connected()) {
      reconnectMQTT();
    }
    mqttClient.loop();
  }

  // Re-scan if lost connection
  if (!connected && doScan) {
    BLEDevice::getScan()->start(0);
  }

  delay(100);
}
