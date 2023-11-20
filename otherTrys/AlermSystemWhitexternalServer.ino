#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include <WiFiClientSecure.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <ESP8266HTTPClient.h>

// Function prototypes
void loadCitiesFromEEPROM();
void handleRoot();
void handleSaveCities();
void handleDisplaySavedCities();
void connectToAlertServer();
void anAlarmSounds();
void checkCitiesAndActiveAlermIfItsMach(String payload);
ESP8266WebServer server(80);
#define EEPROM_SIZE 512
#define CITY_ADDRESS 0

// Global variable
String savedCitiesJson;
const int buzzerPin = 2;
const int wifiIndicatorPin = 5;
const int alertIndicatorPin = 16;
const int serverConnection = 13;
const char *apiUrlAlert = "https://alerm-script.onrender.com/get-alerts";
void setup() {
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);

  // Initialize SPIFFS
  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }

  // List all files in SPIFFS (for debugging purposes)
  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) {
    String fileName = dir.fileName();
    size_t fileSize = dir.fileSize();
    Serial.printf("FS File: %s, size: %s\n", fileName.c_str(), String(fileSize).c_str());
  }

  // Initialize other components
  pinInit();
  wifiConfigAndMDNS();
  loadCitiesFromEEPROM();

  Serial.println("Connected to WiFi");
  digitalWrite(wifiIndicatorPin, HIGH);


  // Set up the web server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/save-cities", HTTP_POST, handleSaveCities);
  server.on("/save-cities", HTTP_GET, handleDisplaySavedCities);
  server.begin(); // Start the ESP8266 web server

}
void loop()
{
  connectToAlertServer();
  server.handleClient(); // Handle client requests
  MDNS.update();




}


void pinInit()
{
  pinMode(buzzerPin, OUTPUT);
  pinMode(wifiIndicatorPin, OUTPUT);
  pinMode(alertIndicatorPin, OUTPUT);
  pinMode(serverConnection, OUTPUT);

}
void wifiConfigAndMDNS() {
  WiFiManager wifiManager;
  WiFi.mode(WIFI_STA);

  if (MDNS.begin("alermsystem")) { // Start the mDNS responder for alermsystem.local
    Serial.println("mDNS responder started");
  }

  // This will attempt to connect using stored credentials
  if (!wifiManager.autoConnect("Alerm System")) {
    Serial.println("Failed to connect and hit timeout");
    ESP.restart(); // Restart the ESP if connection fails
    delay(1000);
  }

  // Now we will load the JSON array of cities from EEPROM
  loadCitiesFromEEPROM(); // Call the function that loads cities from EEPROM
  // If no saved cities were found, we can set a default value or leave it empty
  if (savedCitiesJson.isEmpty()) {
    savedCitiesJson = "[]"; // Default to an empty JSON array if needed
    Serial.println("No saved cities in EEPROM, using default empty array.");
  }
}

void connectToAlertServer() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    WiFiClientSecure client; // Use WiFiClientSecure for HTTPS
    client.setInsecure(); // Optionally, disable certificate verification
    http.begin(client, apiUrlAlert); // Pass the WiFiClientSecure and the URL
    int httpCode = http.GET(); // Make the request

    if (httpCode > 0) {
      digitalWrite(serverConnection, HIGH);
      String payload = http.getString(); // Get the request response payload
      Serial.println("Payload received: " + payload);
      checkCitiesAndActiveAlermIfItsMach(payload);

    } else {
      Serial.println("Failed to connect to alert server, HTTP code: " + String(httpCode));
      digitalWrite(serverConnection, LOW);

    }

    http.end(); // Close connection
  } else {
    Serial.println("Not connected to WiFi.");
    digitalWrite(wifiIndicatorPin, LOW);

  }
}

void handleRoot() {
  String htmlContent = R"(
<!DOCTYPE html>
<html lang='en'>
<head>
    <meta charset='UTF-8'>
    <title>Cities</title>
    <style>
        body {
            font-family: 'Arial', sans-serif;
            background-color: #f4f4f4;
            margin: 0;
            padding: 20px;
            color: #333;
        }
        h1 {
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
            border-bottom: 1px solid #ddd; /* Divider between cities */
            cursor: pointer;
        }
        /* Last city label should not have a divider */
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
    </style>
</head>
<body>
    <h1>Select Cities</h1>
    <input type='text' id='filterInput' placeholder='Filter cities...'>
    <form id='cityForm'>
        <div id='cityList'></div>
        <input type='submit' value='Save Cities'>
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
  DynamicJsonDocument doc(1024); // Adjust the size of the document as necessary
  deserializeJson(doc, savedCitiesJson);
  
  // If savedCitiesJson is an object with a "cities" key, access it like this
  JsonArray cities = doc["cities"].as<JsonArray>();

  // Start building the HTML response with enhanced design
  String responseHtml = R"(
<!DOCTYPE html>
<html lang='en'>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>Selected Cities</title>
    <style>
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background-color: #e9ecef;
            color: #495057;
            margin: 0;
            padding: 20px;
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
    <h1>Selected Cities</h1>
    <ul>
)";

  // Add each city to the HTML list with enhanced design
  for(JsonVariant city : cities) {
    responseHtml += "<li>" + city.as<String>() + "</li>";
  }

  // End the HTML response with enhanced design
  responseHtml += R"(
    </ul>
    <a href='/'><button>Return</button></a>
</body>
</html>
)";

  // Send the response with enhanced design
  server.send(200, "text/html", responseHtml);
}


// Save cities to EEPROM
void saveCitiesToEEPROM(const String& cities) {
  // Clear EEPROM
  for (int i = 0; i < EEPROM_SIZE; i++) {
    EEPROM.write(i, 0);
  }
  // Write cities to EEPROM
  for (unsigned int i = 0; i < cities.length(); i++) {
    EEPROM.write(CITY_ADDRESS + i, cities[i]);
  }
  EEPROM.commit(); // Save changes to EEPROM
}
void anAlarmSounds(){
            for (int i = 0; i < 5; i++)
          {
            digitalWrite(alertIndicatorPin, HIGH);
            digitalWrite(buzzerPin, HIGH);
            delay(500); // Alert on for 500ms
            digitalWrite(alertIndicatorPin, LOW);
            digitalWrite(buzzerPin, LOW);

            delay(500); // Alert off for 500ms
          }
  
  }

void handleSaveCities() {

  
  if (server.hasArg("plain")) {
    String requestBody = server.arg("plain");
    Serial.println("Received cities to save: " + requestBody);

    // Save the JSON string of cities to EEPROM
    saveCitiesToEEPROM(requestBody);

    // Save the JSON string in the global variable for immediate use
    savedCitiesJson = requestBody;

    // Redirect to the GET route to display saved cities
    server.sendHeader("Location", "/save-cities", true);
    server.send(302, "text/plain", ""); // HTTP 302 Redirect
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}


// Load cities from EEPROM
void loadCitiesFromEEPROM() {
  savedCitiesJson = ""; // Clear current data
  for (unsigned int i = CITY_ADDRESS; i < EEPROM_SIZE; i++) {
    char readValue = char(EEPROM.read(i));
    if (readValue == 0) { // Null character signifies end of data
      break;
    }
    savedCitiesJson += readValue; // Append to the savedCitiesJson string
  }
  Serial.println("Loaded cities from EEPROM: " + savedCitiesJson);
}

void checkCitiesAndActiveAlermIfItsMach(String payload){
  
        DynamicJsonDocument payloadDoc(4096);
      DeserializationError error = deserializeJson(payloadDoc, payload);

      if (!error) {
        JsonArray payloadArray = payloadDoc.as<JsonArray>();
        Serial.println("Full Payload: ");
        serializeJsonPretty(payloadArray, Serial);

        if (!savedCitiesJson.isEmpty()) {
          DynamicJsonDocument savedDoc(ESP.getFreeHeap() / 2);
          DeserializationError savedError = deserializeJson(savedDoc, savedCitiesJson);

          if (!savedError) {
            JsonArray savedCities = savedDoc["cities"].as<JsonArray>();
            Serial.println("Saved cities array: ");
            serializeJsonPretty(savedCities, Serial);
            Serial.println();

            bool alertDetected = false;
            for (JsonObject alertCityObject : payloadArray) {
              JsonArray alertCities = alertCityObject["cities"].as<JsonArray>();
              for (String alertCityName : alertCities) {
                Serial.println("Checking city: " + alertCityName);

                for (String savedCity : savedCities) {
                  Serial.println(alertCityName + " compare with " + savedCity);
                  if (alertCityName.equals(savedCity)) {
                    alertDetected = true;
                    Serial.println("Alert detected for: " + savedCity);
                    break;
                  }
                }
                if (alertDetected) break;
              }
              if (alertDetected) break;
            }

            if (alertDetected) {
              Serial.println("Trigger alert...");
              anAlarmSounds();
            } else {
              Serial.println("No alert for saved cities.");
            }
          } else {
            Serial.println("Error in saved cities JSON deserialization: " + String(savedError.c_str()));
          }
        } else {
          Serial.println("Saved cities JSON is empty or not in the correct format.");
        }
      } else {
        Serial.println("Error in payload JSON deserialization: " + String(error.c_str()));
      }
  
  
  
  
  }