#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiManager.h> // Include the WiFiManager library
#include <ESP8266WebServer.h>
#include <EEPROM.h>

#define EEPROM_SIZE 512
#define CITY_ADDRESS 0

ESP8266WebServer server(80);
const char* apiUrl = "https://www.oref.org.il/WarningMessages/alert/alerts.json";

const int buzzerPin = 2;
const int wifiIndicatorPin = 5;
const int alertIndicatorPin = 16;

String targetCity;  // This will be loaded from EEPROM
char storedCity[100]; // Declare storedCity globally

void setup() {
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE); // Initialize EEPROM to store 'targetCity'

  pinMode(buzzerPin, OUTPUT);
  pinMode(wifiIndicatorPin, OUTPUT);
  pinMode(alertIndicatorPin, OUTPUT);

  WiFiManager wifiManager;
  // wifiManager.resetSettings(); // Comment this out to prevent resetting WiFi settings

  WiFi.mode(WIFI_STA);
  // WiFi.disconnect(); // You can also comment this out if you want to auto-reconnect

 // This will attempt to connect using stored credentials
  if (!wifiManager.autoConnect("Alerm System")) {
    Serial.println("Failed to connect and hit timeout");
    // If connection fails, it restarts the ESP
    ESP.restart();
    delay(1000);
  }

  // Load 'targetCity' from EEPROM
  if (EEPROM.read(CITY_ADDRESS) != 255) { // Check first byte to see if it's set
    for (unsigned int i = 0; i < sizeof(storedCity); ++i) {
      storedCity[i] = EEPROM.read(CITY_ADDRESS + i);
    }
    storedCity[sizeof(storedCity) - 1] = '\0'; // Ensure null-terminated string
    targetCity = String(storedCity);
  } else {
    targetCity = "כיסופים"; // Default city name if EEPROM is empty
  }

  Serial.println("Connected to WiFi");
  digitalWrite(wifiIndicatorPin, HIGH);

  // Serve HTML form at root which allows updating 'targetCity'
  server.on("/", HTTP_GET, [&]() {
    String form = "<!DOCTYPE html><html lang='he'><head><meta charset='UTF-8'></head><body>"
                  "<h1>Current target city: " + targetCity + "</h1>"
                  "<form action='/setcity' method='get'>"
                  "Enter new target city: <input type='text' name='city'><br>"
                  "<input type='submit' value='Submit'>"
                  "</form>"
                  "</body></html>";
    server.send(200, "text/html", form);
  });
 // Handle form submission and update 'targetCity'
  server.on("/setcity", HTTP_GET, [&]() {
    if (server.hasArg("city")) {
      targetCity = urlDecode(server.arg("city"));
      Serial.print("Updated target city: ");
      printAsUtf8(targetCity); // Debug print

      // Ensure we don't exceed EEPROM size
      size_t cityLength = targetCity.length() + 1; // +1 for null terminator
      if (cityLength > sizeof(storedCity)) {
        server.send(500, "text/plain", "City name is too long!");
        return;
      }
    
      // Store 'targetCity' in EEPROM
      memset(storedCity, 0, sizeof(storedCity)); // Clear buffer
      targetCity.toCharArray(storedCity, sizeof(storedCity)); // Convert to char array
      for (unsigned int i = 0; i < cityLength; ++i) {
        EEPROM.write(CITY_ADDRESS + i, storedCity[i]);
      }
      EEPROM.commit();
    
      server.send(200, "text/html", "<p>Target city set to: " + targetCity + "</p><a href=\"/\">Return</a>");
      Serial.println("Target city updated to: " + targetCity);
    } else {
      server.send(400, "text/plain", "City parameter missing");
    }
  });

  server.begin(); // Start the web server
}
String getAlertTypeByCategory(const char* category) {
  if (!category) {
    return "missiles";
  }

  int cat = atoi(category);  // Convert string to integer

  switch (cat) {
    case 1:
      return "missiles";
    case 4:
      return "radiologicalEvent";
    case 3:
      return "earthQuake";
    case 5:
      return "tsunami";
    case 6:
      return "hostileAircraftIntrusion";
    case 7:
      return "hazardousMaterials";
    case 13:
      return "terroristInfiltration";
    case 101:
      return "missilesDrill";
    case 103:
      return "earthQuakeDrill";
    case 104:
      return "radiologicalEventDrill";
    case 105:
      return "tsunamiDrill";
    case 106:
      return "hostileAircraftIntrusionDrill";
    case 107:
      return "hazardousMaterialsDrill";
    case 113:
      return "terroristInfiltrationDrill";
    default:
      return "unknown";
  }
}

void loop() {
  server.handleClient(); // Handle client requests

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    WiFiClientSecure client;
    client.setInsecure(); // Disable SSL/TLS certificate validation
    http.begin(client, apiUrl);
    http.addHeader("Referer", "https://www.oref.org.il/11226-he/pakar.aspx");
    http.addHeader("X-Requested-With", "XMLHttpRequest");
    http.addHeader("User-Agent", "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_13_6) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/75.0.3770.100 Safari/537.36");

    int httpCode = http.GET();

    Serial.print("HTTP Status Code: ");
    Serial.println(httpCode);

    if (httpCode == 200) {
      String payload = http.getString();
      Serial.println("Server response:");
      Serial.println(payload);
      Serial.println("Hereeeeee1");


      if (payload.startsWith("\xEF\xBB\xBF")) {
        payload = payload.substring(3); // Remove BOM
              Serial.println("Hereeeeee2");
              Serial.println( "the targetCity: "+targetCity);

      }

      DynamicJsonDocument doc(4096); // Increase buffer size
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
      JsonArray dataArray = doc["data"].as<JsonArray>();
       // Create an empty string to hold the serialized JSON
        String jsonArrayAsString;
        // Serialize the JsonArray to a string
        serializeJson(dataArray, jsonArrayAsString);
        // Now print the string
        Serial.println("dataArray: " + jsonArrayAsString);
      Serial.println("Hereeeeee3");

        bool alertDetected = false;

      for (String city : dataArray) {
          Serial.println(city +" comper with " +targetCity);
        if (city.equals(targetCity)) {
            alertDetected = true;
            Serial.println("alertDetected is change to: " +alertDetected);
            break;
    }
          Serial.println("Hereeeeee4");

  }

        if (alertDetected) {
          Serial.println("Triggering alarm...");
          for (int i = 0; i < 10; i++) {
            digitalWrite(alertIndicatorPin, HIGH);
            digitalWrite(buzzerPin, HIGH);
            delay(500); // Alert on for 500ms
            digitalWrite(alertIndicatorPin, LOW);
            digitalWrite(buzzerPin, LOW);
            Serial.println("Hereeeeee5");

            delay(500); // Alert off for 500ms
          }
        } else {
          Serial.println("No alert for target city.");
        }
      } else {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        Serial.println("Hereeeeee6");

      }
    } else {
      Serial.print("HTTP request failed, error: ");
      Serial.println(http.errorToString(httpCode));

              Serial.println("Hereeeeee7");

    }
    http.end();
  } else {
    Serial.println("Disconnected from WiFi. Trying to reconnect...");
  }
  delay(1000); // Wait for 10 seconds before next check
}

void printAsUtf8(const String &text) {
  for (int i = 0; i < text.length(); i++) {
    Serial.print((byte)text[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}

String urlDecode(const String &text) {
  String decoded = "";
  char temp[] = "0x00";
  for (int i = 0; i < text.length(); i++) {
    if ((text[i] == '%') && (i + 2 < text.length())) {
      temp[2] = text[i + 1];
      temp[3] = text[i + 2];
      decoded += (char) strtol(temp, NULL, 16);
      i += 2;
    } else if (text[i] == '+') {
      decoded += ' ';
    } else {
      decoded += text[i];
    }
  }
  return decoded;
}
