# Home Alert System

## Project Overview
This project was developed to provide a visual alert system for the deaf, particularly to assist my parents who cannot hear auditory alarms. It utilizes an ESP8266 microcontroller connected to an LED strip that lights up when a specific alert is received, such as a siren or emergency alert in the area.

## How It Works
The ESP8266 is programmed to connect to the local WiFi network and continuously monitor for alerts specific to the region set by the user. When an alert is detected, the connected LED strip is activated, providing a visual cue that an event is occurring.

## Features
- **WiFi Configuration**: On initial setup, the device enters WiFi configuration mode, allowing users to connect it to their home WiFi network without needing to hardcode network credentials.
- **Dynamic City Targeting**: Users can set and change the targeted cities for which they want to receive alerts through a user-friendly web interface accessible at `http://alermsystem.local/`.
- **Visual Alert System**: In the event of an alert, the system triggers an LED strip to flash, providing a visual indicator that can be seen throughout the house.

## Setting Up the Device
1. Power on the ESP8266 module.
2. If not previously configured, the device will enter WiFi configuration mode.
3. Connect to the WiFi network named "Alerm System" using any WiFi-enabled device.
4. Follow the prompts to select your home WiFi network and enter the password.
5. Once connected to your home network, the device will exit configuration mode.

## Configuring Target Cities
1. Navigate to `http://alermsystem.local/` on any web browser connected to the same network as the device.
2. The web interface will display a list of cities with checkboxes next to them.
3. Select the cities for which you wish to receive alerts.
4. Submit your selection, and the device will update its monitoring to the chosen cities.

## Technical Details
- **Microcontroller**: ESP8266
- **Programming Language**: C++ (Arduino framework)
- **Additional Hardware**: LED strip compatible with the ESP8266's output voltage and current requirements
- **Third-Party Libraries**: ESP8266WiFi, ESP8266HTTPClient, ArduinoJson, WiFiManager, ESP8266WebServer, EEPROM, NTPClient, WiFiUdp

## Acknowledgements
This project is dedicated to my parents and the deaf community, aiming to provide an additional layer of safety and convenience in daily life.

For more information, assistance, or contributions to the project, please contact [Your Contact Information].

## License
[Specify the license under which this project is made available, if applicable]
