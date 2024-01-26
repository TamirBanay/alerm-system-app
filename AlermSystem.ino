


#include <FastLED.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <ESPmDNS.h>

// Constants for LED configuration
#define LED_PIN     25
#define NUM_LEDS    30
#define BRIGHTNESS  50
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

// Global Variables
CRGB leds[NUM_LEDS];
String targetCities[4];
const char* apiEndpoint = "https://www.oref.org.il/WarningMessages/alert/alerts.json";
WebServer server(80);
String savedCitiesJson;
Preferences preferences;

// Function Declarations
void connectToWifi();
void handleRoot();
void makeApiRequest();
void ledIsOn();
void PermanentUrl();
void saveCitiesToPreferences();
void loadCitiesFromPreferences();
void configModeCallback(WiFiManager *myWiFiManager);
void handleDisplaySavedCities();
void handleSaveCities();

void setup() {
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
  Serial.begin(115200);
  connectToWifi();

  // Start the web server
  server.on("/", HTTP_GET, handleRoot);
  server.on("/save-cities", HTTP_POST, handleSaveCities);
  server.on("/save-cities", HTTP_GET, handleDisplaySavedCities);
  server.begin();
  Serial.println("HTTP server started");

  // Load saved cities and set up MDNS
  loadCitiesFromPreferences();
  PermanentUrl();
}

void loop() {
  makeApiRequest();
  server.handleClient();
}



//url to choose cities http://alerm.local/
void PermanentUrl() {
  if (!MDNS.begin("alerm")) {
    Serial.println("Error setting up MDNS responder!");
  } else {
    Serial.println("mDNS responder started");
    MDNS.addService("http", "tcp", 80);
  }
}


void saveCitiesToPreferences() {
  DynamicJsonDocument doc(4096);
  JsonArray array = doc.to<JsonArray>();

  for (int i = 0; i < sizeof(targetCities) / sizeof(targetCities[0]); i++) {
    if (targetCities[i] != "") {
      array.add(targetCities[i]);
    }
  }

  // Convert JSON array to string
  String jsonString;
  serializeJson(array, jsonString);

  preferences.begin("my-app", false);
  preferences.putString("savedCities", jsonString);
  preferences.end();
}

void loadCitiesFromPreferences() {
  size_t maxCities = sizeof(targetCities) / sizeof(targetCities[0]);

  preferences.begin("my-app", true);
  String jsonString = preferences.getString("savedCities", "");
  preferences.end();

  DynamicJsonDocument doc(4096);
  deserializeJson(doc, jsonString);
  JsonArray array = doc.as<JsonArray>();

  size_t cityIndex = 0;
  for (JsonVariant city : array) {
    if (cityIndex < maxCities) {
      targetCities[cityIndex++] = city.as<String>();
    }
  }
}

void configModeCallback (WiFiManager * myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());
}


void connectToWifi() {
  WiFiManager wifiManager;
  wifiManager.setAPCallback(configModeCallback);
  String ssid = "Alerm System " + String((uint32_t)ESP.getEfuseMac(), HEX); // Create an SSID with the chip ID

  Serial.println("Connecting to: " + ssid);

  if (!wifiManager.autoConnect(ssid.c_str())) { // Convert the String to a char array
    Serial.println("Failed to connect and hit timeout");
    delay(3000);
    ESP.restart();
  } else {
    Serial.println("Success: Connected to WiFi");

  }
}


void handleRoot() {
  // Assuming idTitle is a global variable already set somewhere in your code
  String idTitle = String((uint32_t)ESP.getEfuseMac(), HEX);

  String htmlContent = R"(
<!DOCTYPE html>
<html lang='en'>
<head>
    <meta charset='UTF-8'>        
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>Cities</title>
    <style>
        body {
            font-family: 'Arial', sans-serif;
            background-color: #f4f4f4;
            margin: 0;
            padding: 20px;
            color: #333;
            direction:rtl;
        }
        h1,h3 ,h2{
            text-align: center;
            color: #444;
        }
        #filterInput {
            display: block;
            margin: 20px auto;
            padding: 10px;
            width: 90%;
            max-width: 500px;
            border: 1px solid #ddd;
            border-radius: 5px;
        }
        #cityForm {
            background: white;
            padding: 20px;
            border-radius: 8px;
            box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
        }
        .city-label {
            display: block;
            margin: 10px 0;
            padding-bottom: 10px;
            border-bottom: 1px solid #ddd; 
            cursor: pointer;
        }
        
        .city-label:last-child {
            border-bottom: none;
        }
        input[type="submit"] {
            display: block;
            background-color: #007bff;
            color: white;
            padding: 10px 20px;
            border: none;
            border-radius: 5px;
            margin: 20px auto;
            cursor: pointer;
        }
        input[type="submit"]:hover {
            background-color: #0056b3;
        }
               @media (max-width: 768px) {
            button {
                width: auto;
            }
        }
    </style>
</head>
<body>
)";

  // Concatenate the idTitle with the HTML content
  htmlContent += "<h1>בחירת אזורים</h1>"; 
  htmlContent += "<h2>מספר צ'יפ: " + idTitle + "</h2>"; 

  // Continue with the rest of your HTML content
  htmlContent += R"(
    <h3><a href="/save-cities">אזורים שמורים </a></h3>
    <input type='text' id='filterInput' placeholder='חפש איזורים...'>
    <form id='cityForm'>
        <div id='cityList'></div>
        <input type='submit' value='שמור אזורים'>
    </form>
   
           <script>
        document.getElementById('cityForm').onsubmit = function(event) {
            event.preventDefault();
            var checkedBoxes = document.querySelectorAll('input[name=city]:checked');
            var targetCities = Array.from(checkedBoxes).map(box => box.value);
            fetch('/save-cities', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ cities: targetCities })
            }).then(function(response) {
                if (response.ok) {
                    window.location.href = '/save-cities';
                } else {
                    throw new Error('Server responded with status ' + response.status);
                }
            }).catch(function(error) {
                console.error('Error:', error);
            });
        };

        document.getElementById('filterInput').oninput = function() {
            var filter = this.value.toUpperCase();
            var labels = document.querySelectorAll('.city-label');
            labels.forEach(label => {
                var text = label.textContent || label.innerText;
                label.style.display = text.toUpperCase().includes(filter) ? '' : 'none';
            });
        };

        fetch('https://alerm-script.onrender.com/citiesjson')
        .then(response => response.json())
        .then(cities => {
            var cityListContainer = document.getElementById('cityList');
            cities.forEach(city => {
                var label = document.createElement('label');
                label.classList.add('city-label');
                var checkbox = document.createElement('input');
                checkbox.type = 'checkbox';
                checkbox.name = 'city';
                checkbox.value = city;
                label.appendChild(checkbox);
                label.appendChild(document.createTextNode(` ${city}`));
                cityListContainer.appendChild(label);
            });
        })
        .catch(error => {
            console.error('Error fetching the cities:', error);
        });
    </script>
  
</body>
</html>
)";

  server.send(200, "text/html", htmlContent);
}

void handleDisplaySavedCities() {
  // Parse the JSON
  DynamicJsonDocument doc(1024); 
  deserializeJson(doc, savedCitiesJson);
  
  JsonArray cities = doc["cities"].as<JsonArray>();

  String responseHtml = R"(
<!DOCTYPE html>
<html lang='en'>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>אזורים שנבחרו</title>
    <style>
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background-color: #e9ecef;
            color: #495057;
            margin: 0;
            padding: 20px;
            direction:rtl
        }
        h1 {
            text-align: center;
            color: #212529;
        }
        ul {
            list-style-type: none;
            padding: 0;
        }
        li {
            background-color: white;
            margin: 10px 0;
            padding: 15px;
            border-radius: 8px;
            box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1);
            transition: transform 0.2s;
        }
        li:hover {
            transform: translateY(-2px);
        }
        button {
            display: block;
            width: 200px;
            padding: 10px;
            margin: 20px auto;
            background-color: #007bff;
            color: white;
            border: none;
            border-radius: 5px;
            cursor: pointer;
            font-size: 16px;
            transition: background-color 0.2s;
        }
        button:hover {
            background-color: #0056b3;
        }
        @media (max-width: 768px) {
            button {
                width: auto;
            }
        }
    </style>
</head>
<body>
    <h1>אזורים שנבחרו</h1>
    <ul>
)";

 for (int i = 0; i < sizeof(targetCities) / sizeof(targetCities[0]); i++) {
    if (targetCities[i] != "") {  
      responseHtml += "<li>" + targetCities[i] + "</li>";
    }
  }

  responseHtml += R"(
    </ul>
    <a href='/'><button>חזור</button></a>
</body>
</html>
)";

  server.send(200, "text/html", responseHtml);
}
void handleSaveCities() {
  if (server.hasArg("plain")) {
    String requestBody = server.arg("plain");
    Serial.println("Received cities to save: " + requestBody);

    DynamicJsonDocument doc(1024);
    deserializeJson(doc, requestBody);
    JsonArray cities = doc["cities"].as<JsonArray>();

    // Clear existing cities
    for (int i = 0; i < sizeof(targetCities) / sizeof(targetCities[0]); i++) {
      targetCities[i] = ""; // Clear existing city
    }

    // Update targetCities with new cities
    int index = 0;
    for (JsonVariant city : cities) {
      if (index < (sizeof(targetCities) / sizeof(targetCities[0]))) {
        targetCities[index] = city.as<String>();
        index++;
      }
    }

    // Save the updated cities to preferences
    saveCitiesToPreferences();

    // Redirect to the GET route to display saved cities
    server.sendHeader("Location", "/save-cities", true);
    server.send(302, "text/plain", ""); 
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}


void makeApiRequest() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    WiFiClientSecure client;
    client.setInsecure();
    http.begin(client, apiEndpoint);
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

      if (payload.startsWith("\xEF\xBB\xBF")) {
        payload = payload.substring(3); // Remove BOM
      }
      for (int i = 0; i < sizeof(targetCities) / sizeof(targetCities[0]); i++) {
        Serial.println("Target city: " + targetCities[i]);
      }

      DynamicJsonDocument doc(4096);
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {

        JsonArray dataArray = doc["data"].as<JsonArray>();
        String jsonArrayAsString;
        serializeJson(dataArray, jsonArrayAsString);
        Serial.println("Data array: " + jsonArrayAsString);

        bool alertDetected = false;

        for (JsonVariant cityValue : dataArray) {
          for (String city : targetCities) {
            Serial.println(city + " compare with " + cityValue.as<String>());

            if (cityValue.as<String>() == city) {
              alertDetected = true;
              Serial.println("Alert for: " + city);
              break;
            }
          }
          if (alertDetected) {
            break; 
          }
        }

        if (alertDetected) {
          Serial.println("Triggering alarm...");
          ledIsOn();
        } else {
          Serial.println("No alert for target cities.");
        }
      } else {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
      }
    } else {
      Serial.print("HTTP request failed, error: ");
      Serial.println(http.errorToString(httpCode));
    }
    leds[0] = CRGB::Green;
    FastLED.show();
    http.end();
  } else {
    Serial.println("Disconnected from WiFi. Trying to reconnect...");
  }
  delay(1000);
}
void ledIsOn() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Red;  // Set to red color

  }
  FastLED.show();
  delay(500);  

  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Black;

  }
  FastLED.show();
  delay(500);  
}
