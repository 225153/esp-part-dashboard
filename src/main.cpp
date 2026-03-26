#include <Arduino.h>
#include <DHT.h>

// Define DHT Sensor settings
#define DHTPIN 4       // Connect the DHT11 signal pin to GPIO 4 (D4)
#define DHTTYPE DHT11

// Define MQ7 Sensor settings
#define MQ7PIN 34      // Connect the MQ7 analog output to GPIO 34 (ADC1_CH6)

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n--- ESP32 Multi-Sensor Dashboard ---");
  Serial.println("Monitoring: DHT11 (Temp/Hum) and MQ7 (CO Gas)");
  
  // Ensure G34 is set as input and ADC is configured for full range
  pinMode(MQ7PIN, INPUT);
  analogSetAttenuation(ADC_11db);
  analogReadResolution(12);
  
  dht.begin();
}

void loop() {
  // Wait a few seconds between measurements
  delay(2000);

  // --- DHT11 Readings ---
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // --- MQ7 Readings with Averaging ---
  long sum = 0;
  for(int i=0; i<32; i++) {
    sum += analogRead(MQ7PIN);
  }
  int mq7Value = sum / 32;
  float voltage = mq7Value * (3.3 / 4095.0);

  // Check if DHT reads failed
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
  } else {
    Serial.print("Humidity: ");
    Serial.print(h);
    Serial.print("%  |  ");
    Serial.print("Temperature: ");
    Serial.print(t);
    Serial.println("°C");
  }

  // Display MQ7 Results
  Serial.print("MQ7 Gas Value: ");
  Serial.print(mq7Value);
  Serial.print(" (Raw)  |  Approx Voltage: ");
  Serial.print(voltage);
  Serial.println("V");
  
  // Simple safety threshold example
  if (mq7Value > 1500) {
    Serial.println("WARNING: High Gas/CO levels detected!");
  }
  
  Serial.println("---------------------------------------");
}