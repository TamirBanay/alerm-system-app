#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

const char *ssid = "**WIFI_NAME**";
const char *password = "**WIFI_PASSWORD**";
const char *apiUrl = "https://www.oref.org.il/WarningMessages/alert/alerts.json";

const int buzzerPin = 2;
const int wifiIndicatorPin = 5;
const int alertIndicatorPin = 16;

void setup()
{
    Serial.begin(115200);

    pinMode(buzzerPin, OUTPUT);
    pinMode(wifiIndicatorPin, OUTPUT);
    pinMode(alertIndicatorPin, OUTPUT);

    connectToWiFi();
}

void connectToWiFi()
{
    digitalWrite(wifiIndicatorPin, LOW);
    Serial.println("Connecting to WiFi...");

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.println("Still connecting to WiFi...");
    }

    Serial.println("Connected to WiFi");
    digitalWrite(wifiIndicatorPin, HIGH);
}

String getAlertTypeByCategory(const char *category)
{
    if (!category)
    {
        return "missiles";
    }

    int cat = atoi(category); // Convert string to integer

    switch (cat)
    {
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

void loop()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        HTTPClient http;
        WiFiClientSecure client;
        client.setInsecure();
        http.begin(client, apiUrl);
        http.addHeader("Referer", "https://www.oref.org.il/11226-he/pakar.aspx");
        http.addHeader("X-Requested-With", "XMLHttpRequest");
        http.addHeader("User-Agent", "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_13_6) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/75.0.3770.100 Safari/537.36");

        int httpCode = http.GET();

        if (httpCode == 200)
        {
            String payload = http.getString();
            Serial.println("Server response:");
            Serial.println(payload);

            if (payload.startsWith("\xEF\xBB\xBF"))
            {
                payload = payload.substring(3);
            }

            DynamicJsonDocument doc(4096); 
            DeserializationError error = deserializeJson(doc, payload);

            // Check if JSON parsing was successful
            if (error)
            {
                Serial.print("deserializeJson() failed: ");
                Serial.println(error.c_str());
                http.end();
                return;
            }

            JsonArray dataArray = doc["data"].as<JsonArray>();
            Serial.print("Data array length: ");
            Serial.println(dataArray.size());
            Serial.println(dataArray);

            if (dataArray.size() > 0)
            {
                String alertType = getAlertTypeByCategory(doc["cat"]);
                Serial.println("Alert detected of type: " + alertType);
                for (int i = 0; i < 10; i++)
                {
                    digitalWrite(alertIndicatorPin, HIGH);
                    digitalWrite(buzzerPin, HIGH);
                    delay(1000);
                    digitalWrite(alertIndicatorPin, LOW);
                    digitalWrite(buzzerPin, LOW);
                    delay(1000);
                }
            }
        }
        else
        {
            Serial.printf("Error occurred: HTTP code: %d\n", httpCode);
        }
        http.end();
    }
    else
    {
        Serial.println("Disconnected from WiFi. Trying to reconnect...");
        connectToWiFi();
    }
    delay(1000);
}