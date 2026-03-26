# ESP32 Dashboard (DHT11 Sensor)

A simple ESP32-based dashboard for monitoring environmental data. This project currently reads temperature and humidity from a DHT11 sensor and outputs the data to the Serial Monitor.

## 🛠 Hardware Components
- **Microcontroller:** ESP32 WROOM-32D (Dev Module)
- **Sensor:** DHT11 (Temperature and Humidity)
- **Built-in LED:** GPIO 2 (typically)

## 🔌 Pinout & Wiring

| DHT11 Pin | ESP32 Pin | Description |
|-----------|-----------|-------------|
| **VCC**   | **3V3**   | Power Supply (3.3V) |
| **DATA**  | **G4 (GPIO 4)** | Data Signal Pin |
| **GND**   | **GND**   | Ground |

> **Note:** If you are using a bare DHT11 sensor (not a module), place a 4.7kΩ - 10kΩ resistor between VCC and DATA pins as a pull-up.

## 🚀 Getting Started

### Prerequisites
- [VS Code](https://code.visualstudio.com/)
- [PlatformIO IDE Extension](https://platformio.org/platformio-ide)

### Setup & Upload
1. Clone the repository:
   ```bash
   git clone https://github.com/225153/esp-part-dashboard.git
   ```
2. Open the folder in VS Code.
3. PlatformIO will automatically install the necessary libraries:
   - `DHT sensor library`
   - `Adafruit Unified Sensor`
4. Connect your ESP32 via USB.
5. Click the **Build** button.
6. Click the **Upload** button.
7. Open the **Serial Monitor** (115200 baud) to view the data.

## 📊 Project Structure
- `src/main.cpp`: Main application code.
- `platformio.ini`: Project configuration and dependencies.
- `.gitignore`: Files excluded from version control.

## 📜 License
MIT License
