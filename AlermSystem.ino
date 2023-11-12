#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiManager.h> // Include the WiFiManager library
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP8266mDNS.h>

WiFiUDP ntpUDP;
// Adjust the NTP client's time offset to match Israel's timezone
// For Standard Time (UTC+2) use 7200, for Daylight Saving Time (UTC+3) use 10800
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7200);
ESP8266WebServer server(80);

#define EEPROM_SIZE 512
#define CITY_ADDRESS 0

const char *apiUrl = "https://www.oref.org.il/WarningMessages/alert/alerts.json";
const int buzzerPin = 2;
const int wifiIndicatorPin = 5;
const int alertIndicatorPin = 16;
String targetCity;    // This will be loaded from EEPROM
char storedCity[100]; // Declare storedCity globally

void setup()
{
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE); // Initialize EEPROM to store 'targetCity'
  timeClient.begin();
  pinInit();
  handleRoot();
  wifiConfigAndMDNS();
  Serial.println("Connected to WiFi");
  digitalWrite(wifiIndicatorPin, HIGH);
}

void loop()
{
  server.handleClient(); // Handle client requests
  MDNS.update();

  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;
    WiFiClientSecure client;
    client.setInsecure(); // Disable SSL/TLS certificate validation
    http.begin(client, apiUrl);
    http.addHeader("Referer", "https://www.oref.org.il/11226-he/pakar.aspx");
    http.addHeader("X-Requested-With", "XMLHttpRequest");
    http.addHeader("User-Agent", "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_13_6) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/75.0.3770.100 Safari/537.36");
    timeClient.update();

    int httpCode = http.GET();

    Serial.print("HTTP Status Code: ");
    Serial.println(httpCode);

    if (httpCode == 200)
    {
      String payload = http.getString();
      Serial.println("Server response:");
      Serial.println(payload);
      Serial.print("Current time: ");
      Serial.println(timeClient.getFormattedTime());

      if (payload.startsWith("\xEF\xBB\xBF"))
      {
        payload = payload.substring(3); // Remove BOM
        Serial.println("the targetCity: " + targetCity);
      }

      DynamicJsonDocument doc(4096); // Increase buffer size
      DeserializationError error = deserializeJson(doc, payload);

      if (!error)
      {
        JsonArray dataArray = doc["data"].as<JsonArray>();
        // Create an empty string to hold the serialized JSON
        String jsonArrayAsString;
        // Serialize the JsonArray to a string
        serializeJson(dataArray, jsonArrayAsString);
        // Now print the string
        Serial.println("dataArray: " + jsonArrayAsString);

        bool alertDetected = false;

        for (String city : dataArray)
        {
          Serial.println(city + " comper with " + targetCity);
          if (city.equals(targetCity))
          {
            alertDetected = true;
            Serial.println("alertDetected is change to: " + alertDetected);
            break;
          }
        }

        if (alertDetected)
        {
          Serial.println("Triggering alarm...");
          for (int i = 0; i < 10; i++)
          {
            digitalWrite(alertIndicatorPin, HIGH);
            digitalWrite(buzzerPin, HIGH);
            delay(500); // Alert on for 500ms
            digitalWrite(alertIndicatorPin, LOW);
            digitalWrite(buzzerPin, LOW);

            delay(500); // Alert off for 500ms
          }
        }
        else
        {
          Serial.println("No alert for target city.");
        }
      }
      else
      {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
      }
    }
    else
    {
      Serial.print("HTTP request failed, error: ");
      Serial.println(http.errorToString(httpCode));
    }
    http.end();
  }
  else
  {
    Serial.println("Disconnected from WiFi. Trying to reconnect...");
    digitalWrite(wifiIndicatorPin, LOW);
  }
  delay(1000); // Wait for 10 seconds before next check
}

void pinInit()
{
  pinMode(buzzerPin, OUTPUT);
  pinMode(wifiIndicatorPin, OUTPUT);
  pinMode(alertIndicatorPin, OUTPUT);
}
void wifiConfigAndMDNS()
{
  WiFiManager wifiManager;
  WiFi.mode(WIFI_STA);
  // wifiManager.resetSettings(); // Comment this out to prevent resetting WiFi settings
  // WiFi.disconnect(); // You can also comment this out if you want to auto-reconnect

  if (MDNS.begin("alermsystem"))
  { // the localHost address is in - > http://alermsystem.local/
    Serial.println("mDNS responder started");
  }
  // This will attempt to connect using stored credentials
  if (!wifiManager.autoConnect("Alerm System"))
  {
    Serial.println("Failed to connect and hit timeout");
    // If connection fails, it restarts the ESP
    ESP.restart();
    delay(1000);
  }

  // Load 'targetCity' from EEPROM
  if (EEPROM.read(CITY_ADDRESS) != 255)
  { // Check first byte to see if it's set
    for (unsigned int i = 0; i < sizeof(storedCity); ++i)
    {
      storedCity[i] = EEPROM.read(CITY_ADDRESS + i);
    }
    storedCity[sizeof(storedCity) - 1] = '\0'; // Ensure null-terminated string
    targetCity = String(storedCity);
  }
  else
  {
    targetCity = "אשקלון - דרום"; // Default city name if EEPROM is empty
  }
}

void handleRoot()
{
  server.on("/", HTTP_GET, [&]()
            {
  String form = "<!DOCTYPE html><html lang='he'><head><meta charset='UTF-8'>"
                "<style>"
                "body { font-family: Arial, sans-serif; background-color: #f8f8f8; text-align: center; }"
                "h1 { color: #333; margin-top: 50px; }"
                "form { max-width: 300px; margin: 50px auto 0 auto; padding: 1em; background: #fff; border-radius: 1em; box-shadow: 0 4px 8px rgba(0,0,0,0.1); }"
                "input[type='text'], input[type='submit'] { width: 90%; padding: 0.7em; margin: 0.5em 0; border-radius: 0.5em; border: 1px solid #ddd; box-sizing: border-box; }"
                "input[type='submit'] { background: #007bff; color: white; font-weight: bold; border: none; cursor: pointer; }"
                "input[type='submit']:hover { background: #0056b3; }"
                "</style>"
                "</head><body>"
                "<h1>Current target city: " + targetCity + "</h1>"
                "<form action='/setcity' method='get'>"
                "Enter new target city: <input type='text' name='city'><br>"
                "<input type='submit' value='Submit'>"
                "</form>"
                "<h4> Go to that link, find you location alerm and copy, then paste it on the Target city.</h4>"
                "<a href = 'https://www.oref.org.il/Shared/Ajax/GetCitiesMix.aspx?lang=he'>Choose city</a>"
                "</body></html>";
  server.send(200, "text/html", form); });

  server.on("/setcity", HTTP_GET, [&]()
            {
    if (server.hasArg("city")) {
        targetCity = urlDecode(server.arg("city"));
        Serial.print("Updated target city: ");
        printAsUtf8(targetCity); // Debug print

        size_t cityLength = targetCity.length() + 1; // +1 for null terminator
        if (cityLength > sizeof(storedCity)) {
            String response = "<!DOCTYPE html><html lang='he'><head><meta charset='UTF-8'><title>Update Error</title></head><body>"
                              "<h2 style='color: red;'>Error: City name is too long!</h2>"
                              "<a href='/' style='display:inline-block;margin-top:20px;padding:10px;background-color:#007bff;color:white;text-decoration:none;border-radius:5px;'>Return</a>"
                              "</body></html>";
            server.send(500, "text/html", response);
            return;
        }

        memset(storedCity, 0, sizeof(storedCity)); // Clear buffer
        targetCity.toCharArray(storedCity, sizeof(storedCity)); // Convert to char array
        for (unsigned int i = 0; i < cityLength; ++i) {
            EEPROM.write(CITY_ADDRESS + i, storedCity[i]);
        }
        EEPROM.commit();

        String response = "<!DOCTYPE html><html lang='he'><head><meta charset='UTF-8'><title>City Updated</title></head><body>"
                          "<h2 style='color: green;'>Target city set to: " + targetCity + "</h2>"
                          "<a href='/' style='display:inline-block;margin-top:20px;padding:10px;background-color:#007bff;color:white;text-decoration:none;border-radius:5px;'>Return</a>"
                          "</body></html>";
        server.send(200, "text/html", response);
        Serial.println("Target city updated to: " + targetCity);
    } else {
        String response = "<!DOCTYPE html><html lang='he'><head><meta charset='UTF-8'><title>Update Error</title></head><body>"
                          "<h2 style='color: red;'>Error: City parameter missing</h2>"
                          "<a href='/' style='display:inline-block;margin-top:20px;padding:10px;background-color:#007bff;color:white;text-decoration:none;border-radius:5px;'>Return</a>"
                          "</body></html>";
        server.send(400, "text/html", response);
    } });

  server.begin(); // Start the web server
}

void printAsUtf8(const String &text)
{
  for (int i = 0; i < text.length(); i++)
  {
    unsigned char c = (unsigned char)text[i];
    if (c >= 128)
    {
      Serial.print("0x");
      Serial.print(c, HEX);
      Serial.print(" ");
    }
    else
    {
      Serial.write(c);
    }
  }
  Serial.println();
}

String urlDecode(const String &text)
{
  String decoded = "";
  char temp[] = "0x00";
  for (int i = 0; i < text.length(); i++)
  {
    if ((text[i] == '%') && (i + 2 < text.length()))
    {
      temp[2] = text[i + 1];
      temp[3] = text[i + 2];
      decoded += (char)strtol(temp, NULL, 16);
      i += 2;
    }
    else if (text[i] == '+')
    {
      decoded += ' ';
    }
    else
    {
      decoded += text[i];
    }
  }
  return decoded;
}
