# ESP32 Dashboard (DHT11 & MQ7 Sensors)

A simple ESP32-based dashboard for monitoring environmental data. This project reads data from a DHT11 sensor (climate) and an MQ7 sensor (carbon monoxide gas).

## 🛠 Hardware Components
- **Microcontroller:** ESP32 WROOM-32D (Dev Module)
- **Climate Sensor:** DHT11 (Temperature & Humidity)
- **Gas Sensor:** MQ7 (CO Gas Detection)

## 🔌 Pinout & Wiring

### DHT11 (Climate)
| DHT11 Pin | ESP32 Pin | Description |
|-----------|-----------|-------------|
| **VCC**   | **3V3**   | Power Supply (3.3V) |
| **DATA**  | **G4 (GPIO 4)** | Data Signal Pin |
| **GND**   | **GND**   | Ground |

### MQ7 (Gas)
| MQ7 Pin | ESP32 Pin | Description |
|-----------|-----------|-------------|
| **VCC**   | **5V (VIN)** | Needs 5V for heating coil |
| **AO**    | **G34 (GPIO 34)** | Analog Output |
| **GND**   | **GND**   | Ground |

> **Note:** The MQ7 requires a heating phase. Let the sensor warm up for a few minutes for stable raw values.

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
3. PlatformIO will automatically install dependencies (`DHT sensor library`, `Adafruit Unified Sensor`).
4. Connect ESP32 via USB.
5. Click **Build** then **Upload**.
6. Open the **Serial Monitor** (115200 baud).

## 📊 Project Structure
- `src/main.cpp`: Main application code.
- `platformio.ini`: Project configuration and dependencies.
- `.gitignore`: Files excluded from version control.

## 📜 License
MIT License
