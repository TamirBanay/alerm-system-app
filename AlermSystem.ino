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
String savedCitiesJson; // This should be a valid JSON string.

void setup()
{
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE); // Initialize EEPROM to store 'targetCity'
  timeClient.begin();
  pinInit();
  server.on("/", HTTP_GET, handleRoot);
  server.on("/save-cities", HTTP_POST, handleSaveCities);
  server.on("/save-cities", HTTP_GET, handleDisplaySavedCities); 
  wifiConfigAndMDNS();
  savedCitiesJson = loadCitiesFromEEPROM();
  server.begin();

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
            direction: rtl; 
        }
        h1 {
            text-align: center;
            color: #444;
        }
           #myCities {
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
    <h1>בחר איזורים מהרשימה</h1>
    <h3  id='myCities'>
    <a href='/save-cities'>האיזורים שלי</a>
    <h3/>
    <input type='text' id='filterInput' placeholder='חפש איזורים ...'>
    <form id='cityForm'>
        <div id='cityList'></div>
        <input type='submit' value='שמור ערים'>
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
    <title>איזורים שנבחרו</title>
    <style>
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background-color: #e9ecef;
            color: #495057;
            margin: 0;
            padding: 20px;
            direction: rtl; 
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
    <h1>איזורים שנבחרו</h1>
    <ul>
)";

  // Add each city to the HTML list with enhanced design
  for(JsonVariant city : cities) {
    responseHtml += "<li>" + city.as<String>() + "</li>";
  }

  // End the HTML response with enhanced design
  responseHtml += R"(
    </ul>
    <a href='/'><button>חזור</button></a>
</body>
</html>
)";

  // Send the response with enhanced design
  server.send(200, "text/html", responseHtml);
}



void saveCitiesToEEPROM(const String& data) {
  // Make sure the data to save does not exceed the EEPROM size
  int len = min(data.length(), (unsigned int)EEPROM_SIZE - 1);
  for (int i = 0; i < len; ++i) {
    EEPROM.write(i, data[i]);
  }
  // Write the null terminator to mark the end of the string
  EEPROM.write(len, '\0');
  EEPROM.commit();
}

String loadCitiesFromEEPROM() {
  String data;
  for (int i = 0; i < EEPROM_SIZE; ++i) {
    char c = EEPROM.read(i);
    if (c == '\0') break; // Stop reading at the null terminator
    data += c;
  }
  return data;
}

void handleSaveCities() {

  
  if (server.hasArg("plain")) {
    String requestBody = server.arg("plain");
    targetCity = requestBody;
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