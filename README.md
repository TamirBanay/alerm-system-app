

# ESP32-WROOM-DA module City Alert System

## Project Description
This project creates an ESP32-based alert system that allows users to select specific cities for monitoring alerts. It features a WiFi connection setup via WiFiManager and a local web server for city selection.

## Features
- **WiFi Configuration:** Utilize WiFiManager for easy WiFi setup without hardcoding credentials.
- **Local Server for City Selection:** A web interface to choose cities for alerts.
- **Persistent City Storage:** Selected cities are stored in the ESP32's non-volatile memory, ensuring settings are retained across reboots.

## Hardware Requirements
- ESP32 Module
- WS2812B LED Strip
- Buzzer (optional for additional alert mechanism)

## Hardware Connections
### ESP32 and WS2812B LED Strip
1. **Data Line:** Connect the data input pin of the WS2812B LED strip to GPIO 25 on the ESP32.
2. **Power:** The WS2812B strip requires 5V power. Connect the 5V and GND lines of the LED strip to a suitable 5V power supply. Do not power the LED strip directly from the ESP32 as it may not provide sufficient current.
3. **Common Ground:** Ensure that the GND of the ESP32 is connected to the GND of the LED strip power supply. This common ground is necessary for proper communication between the ESP32 and the LED strip.

### Buzzer
- Connect your buzzer to GPIO 35 on the ESP32.

## Software Requirements
- Arduino IDE
- ESP32 Board support installed in Arduino IDE
- Libraries: FastLED, WiFi, WiFiManager, ArduinoJson, DNSServer, WebServer

## Installation
1. Clone the repository or download the source code.
2. Open the project in Arduino IDE.
3. Install required libraries via the Library Manager in Arduino IDE:
   - FastLED
   - WiFiManager
   - ArduinoJson
4. Select the appropriate board (ESP32) and COM port in Arduino IDE.

## Setup and Usage
### Initial Setup
1. Power on the ESP32 module.
2. On the first run, ESP32 will open a WiFi Access Point (AP) named "Alerm System".
3. Connect to this AP with any WiFi-enabled device.

### WiFi Configuration
1. A captive portal will automatically open for WiFi configuration.
2. Select your home or office WiFi network and enter the password.
3. Once connected, ESP32 will save these settings and automatically connect on subsequent reboots.

### Selecting Cities
1. Access http://alerm.local/ (Need to be on the same wifi as the module) address with a web browser to open the city selection interface.
2. The web interface lists available cities with checkboxes.
3. Select the desired cities for monitoring and submit.
4. The selected cities are saved in the ESP32's non-volatile memory.

### Operational Behavior
- Upon successful WiFi connection, the first LED on the strip turns green.
- The ESP32 monitors for alerts specific to the chosen cities.
- In case of an alert, the system activates predefined alert mechanisms (e.g., LED patterns, buzzer sound).

