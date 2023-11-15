#include <ESP8266WiFi.h>
#include <WiFiManager.h> // Include the WiFiManager library
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h> // Include ArduinoJson library
#include <FS.h>          // Include the SPIFFS library

String cities;
String targetCity;    // This will be loaded from EEPROM
char storedCity[100]; // Declare storedCity globally
#define EEPROM_SIZE 512
#define CITY_ADDRESS 0
ESP8266WebServer server(80);

const int buzzerPin = 2;
const int wifiIndicatorPin = 5;
const int alertIndicatorPin = 16;
const int serverConnection = 13;
const char *apiUrlAlert = "https://alerm-script.onrender.com/get-alerts";
void setup()
{
    Serial.begin(115200);

    // Initialize SPIFFS
    if (!SPIFFS.begin())
    {
        Serial.println("Failed to mount file system");
        return;
    }

    // List all files in SPIFFS (for debugging purposes)
    Dir dir = SPIFFS.openDir("/");
    while (dir.next())
    {
        String fileName = dir.fileName();
        size_t fileSize = dir.fileSize();
        Serial.printf("FS File: %s, size: %s\n", fileName.c_str(), String(fileSize).c_str());
    }

    // Initialize other components
    pinInit();
    wifiConfigAndMDNS();
    Serial.println("Connected to WiFi");
    digitalWrite(wifiIndicatorPin, HIGH);

    // Set up the web server
    server.begin();                       // Start the ESP8266 web server
    server.on("/", HTTP_GET, handleRoot); // Define the handler for the root path
    server.on("/save-cities", HTTP_POST, handleSaveCities);
}

void loop()
{
    connectToAlertServer();
    server.handleClient(); // Handle client requests
    MDNS.update();
    Serial.println(cities);
}

void pinInit()
{
    pinMode(buzzerPin, OUTPUT);
    pinMode(wifiIndicatorPin, OUTPUT);
    pinMode(alertIndicatorPin, OUTPUT);
    pinMode(serverConnection, OUTPUT);
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

void connectToAlertServer()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        HTTPClient http;
        WiFiClientSecure client;         // Use WiFiClientSecure for HTTPS
        client.setInsecure();            // Optionally, disable certificate verification
        http.begin(client, apiUrlAlert); // Pass the WiFiClientSecure and the URL
        int httpCode = http.GET();       // Make the request
        if (httpCode > 0)
        { // Check the returning code
            digitalWrite(serverConnection, HIGH);
            Serial.println("connect to alert server");
            String payload = http.getString(); // Get the request response payload
            Serial.println(payload);           // Print the response payload
        }

        http.end(); // Close connection
    }
}

void handleRoot()
{
    String htmlContent = "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><title>Cities</title></head><body>"
                         "<h1>Select Cities</h1>"
                         "<form id='cityForm'>"
                         "<div id='cityList'></div>"
                         "<input type='submit' value='Save Cities'>"
                         "</form>"
                         "<script>"
                         "document.getElementById('cityForm').onsubmit = function(event) {"
                         "  event.preventDefault();"
                         "  var checkedBoxes = document.querySelectorAll('input[name=city]:checked');"
                         "  var targetCities = Array.from(checkedBoxes).map(function(box) { return box.value; });"
                         "  fetch('/save-cities', {"
                         "    method: 'POST',"
                         "    headers: { 'Content-Type': 'application/json' },"
                         "    body: JSON.stringify({ cities: targetCities })"
                         "  }).then(function(response) {"
                         "    return response.text();"
                         "  }).then(function(text) {"
                         "    console.log('Response:', text);"
                         "  }).catch(function(error) {"
                         "    console.error('Error:', error);"
                         "  });"
                         "};"
                         "fetch('https://alerm-script.onrender.com/citiesjson')"
                         ".then(function(response) { return response.json(); })"
                         ".then(function(cities) {"
                         "  var cityListContainer = document.getElementById('cityList');"
                         "  cities.forEach(function(city) {"
                         "    var label = document.createElement('label');"
                         "    var checkbox = document.createElement('input');"
                         "    checkbox.type = 'checkbox';"
                         "    checkbox.name = 'city';"
                         "    checkbox.value = city;"
                         "    label.appendChild(checkbox);"
                         "    label.appendChild(document.createTextNode(city));"
                         "    label.appendChild(document.createElement('br'));"
                         "    cityListContainer.appendChild(label);"
                         "  });"
                         "})"
                         ".catch(function(error) {"
                         "  console.error('Error fetching the cities:', error);"
                         "});"
                         "</script>"
                         "</body></html>";
    server.send(200, "text/html", htmlContent);
}
void handleSaveCities()
{
    if (server.hasArg("plain"))
    {
        cities = server.arg("plain");
        Serial.println("Received cities to save: " + cities);

        // Parse the JSON body with the cities array
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, cities);
        JsonArray targetCities = doc["cities"];

        // TODO: Handle the targetCities array as needed
        // You could store the values in EEPROM, a file, or keep in memory

        server.send(200, "text/plain", "Cities saved");
    }
    else
    {
        server.send(400, "text/plain", "Bad Request");
    }
}