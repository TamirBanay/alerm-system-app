# README for Rocket Alarm System Project

## Project Overview
This project is a specialized alarm system designed to alert individuals, especially those with hearing impairments, about rocket alerts in their area. It was developed specifically to assist my deaf parents, who cannot hear standard alarm sirens during rocket attacks. The system uses an ESP8266 microcontroller to connect to the internet and monitor real-time alerts for specific locations. When a rocket alert is issued for the configured target city, the system activates LED strips, providing a vivid visual cue to signal the danger.

## Purpose
The primary aim of this project is to ensure that deaf and hard-of-hearing individuals receive timely warnings about rocket attacks, enhancing their safety and security during such emergencies.

## Features
- **Real-Time Rocket Alert Monitoring:** Connects to a public API to receive live updates on rocket alerts.
- **Visual Alert System:** Uses LED strips to provide a clear and unmistakable visual signal during an alert.
- **Web-Based Configuration:** Offers a user-friendly web interface to set and change the target city for alerts.
- **Persistent Settings:** Stores the selected city in EEPROM, retaining the setting even after power cycles.
- **NTP Time Synchronization:** Ensures accurate timekeeping for log entries and time-stamped alert notifications.
- **WiFi Connectivity:** Automatically connects to a home WiFi network for internet access.

## Hardware Components
- ESP8266 Microcontroller
- LED Strips (for visual alerts)
- Buzzer (optional for additional auditory alerts)
- Indicator LEDs (for system status indication)

## Software Dependencies
- ESP8266WiFi.h
- ESP8266HTTPClient.h
- ArduinoJson.h
- WiFiManager.h
- ESP8266WebServer.h
- EEPROM.h
- NTPClient.h
- WiFiUdp.h
- ESP8266mDNS.h

## Setup Instructions
1. **Install Firmware:** Upload the `AlermSystem.ino` sketch to the ESP8266 using the Arduino IDE.
2. **Configure WiFi:** Initially, the device will create a WiFi access point for setup. Connect to it and enter your WiFi network details.
3. **Set Target City:** Use a web browser to go to `http://alermsystem.local/` and enter the name of your city or the area you want to monitor for rocket alerts.
4. **Operation:** The system will continuously monitor for rocket alerts and activate the LED strips if an alert is detected in the specified area.

## Usage
- **Monitoring:** The device stays connected to your WiFi network and checks for rocket alerts at set intervals.
- **Alert Activation:** In the event of a rocket alert in your configured area, the LED strips will light up, providing a visual signal.
- **Web Interface:** You can change the target city at any time by accessing the web interface at `http://alermsystem.local/`.

## Safety and Reliability
- This system is designed to enhance awareness and should be used alongside official alert systems.
- It's crucial to regularly verify that the system is functioning correctly and to maintain the hardware.

## Motivation
This system was created to assist people like my parents, who are deaf, and cannot hear traditional rocket sirens. The visual alert system bridges this gap, ensuring they have immediate and reliable notifications in the event of a rocket attack.

## Future Developments
- Expanding the system to integrate with various smart home devices.
- Adding customizable alert patterns and intensities for the LED strips.
- Implementing battery backup for uninterrupted operation during power failures.

## Disclaimer
This project is a personal endeavor and is not officially associated with any emergency response services. It relies on publicly available information sources and should serve as a supplementary alert system only. For critical information and official alerts, always refer to authorized government channels.