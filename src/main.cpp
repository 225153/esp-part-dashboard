#include <Arduino.h>
#include <DHT.h>

// Define DHT Sensor settings
#define DHTPIN 4       // Connect the DHT11 signal pin to GPIO 4 (D4)
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n--- DHT11 Sensor Test ---");
  dht.begin();
}

void loop() {
  // Wait a few seconds between measurements
  delay(2000);

  // Reading temperature or humidity takes about 250 milliseconds!
  float h = dht.readHumidity();
  float t = dht.readTemperature(); // Temperature in Celsius

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print("%  |  ");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.println("°C");
}