#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         // https://github.com/tzapu/WiFiManager

// Set web server port number to 80
WiFiServer server(80);
WiFiClient client;

//1 day = 86400000 milliseconds
const unsigned long aDay = 86400000;
String latitude = "51.02";
String longitude = "3.67";

//store info about api call astro-info
unsigned long millisWhenAstroChecked = 0;
const unsigned long astroCheckInterval = aDay;
//store info about api call time-info
unsigned long millisWhenTimeChecked = 0;
const unsigned long timeCheckInterval = aDay;
//store moment when sunsetSunrise[0] = sunset - sunsetSunrise[1] = sunrise
unsigned long sunsetSunrise[2] = {0, 0};
//store how long it takes until sunset/sunrise
unsigned long timeUntilSunsetSunrise[2] = {0, 0};

//user want to manually override auto shutter function?
bool userManualControl;

//naming the used I/O pins
const int shutterUpPin = 5;
const int shutterDownPin = 4;
const int manualControlStatusPin = 0;
const int manualControlInput = 2;

void setup() {
  Serial.begin(9600);

  // Initialize the output variables
  pinMode(shutterUpPin, OUTPUT);
  pinMode(shutterDownPin, OUTPUT);
  pinMode(manualControlStatusPin, OUTPUT);
  pinMode(manualControlInput, INPUT);

  // Set outputs to LOW
  digitalWrite(shutterUpPin, LOW);
  digitalWrite(shutterDownPin, LOW);
  digitalWrite(manualControlStatusPin, LOW);
  userManualControl = digitalRead(manualControlStatusPin);

  WiFiManager wifiManager;
  // Uncomment and run it once, if you want to erase all the stored information
  //wifiManager.resetSettings();
  // set custom ip for portal
  //wifiManager.setAPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
  // fetches ssid and pass from eeprom and tries to connect
  // if it does not connect it starts an access point with the specified name
  // here  "AutoConnectAP"
  // and goes into a blocking loop awaiting configuration
  wifiManager.autoConnect("AutoConnectAP");

  // or use this for auto generated name ESP + ChipID
  //wifiManager.autoConnect();
  // if you get here you have connected to the WiFi
  Serial.println("Connected.");

  server.begin();
}

void loop() {

  client  = server.available();   // Listen for incoming clients;

  //only make one call per 60minutes
  if (millis() - millisWhenAstroChecked > astroCheckInterval || millisWhenAstroChecked == 0 ) {
    Serial.println("connecting...");
    // if there's a successful connection:
    if (client.connect("api.sunrise-sunset.org", 80)) {

      // send the HTTP PUT request:
      client.println("GET /json?lat=" + latitude + "&lng=" + longitude + "&date=today&formatted=0");
      client.println("Host: api.sunrise-sunset.org");
      client.println("User - Agent: ArduinoWiFi / 1.1");
      client.println("Connection: close");
      client.println();

      // stop process if connection times out
      unsigned long timeout = millis();
      while (client.available() == 0) {
        if (millis() - timeout > 5000) {
          Serial.println(" >>> Client Timeout !");
          client.stop();
          return;
        }
      }
      parseAstroJson(client, sunsetSunrise, millisWhenAstroChecked);

    }
    else {
      // if no connection was made:
      Serial.println("connection failed");
      return;
    }
  }
}


//to parse json data recieved from OWM
void parseAstroJson(WiFiClient client, unsigned long sunsetSunrise[2], unsigned long millisWhenAstroChecked) {
  //memory allocation needed (calculated on: https://arduinojson.org/v6/assistant/)
  const size_t capacity = 766;
  DynamicJsonDocument doc(capacity);
  DeserializationError err = deserializeJson(doc, client);
  //parse succeeded?
  if (err) {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(err.c_str());
    return;
  } else {
    JsonObject sys = doc["sys"];

    JsonObject results = doc.createNestedObject("results");

    //todo : do actual parsing of object


    //save the time when we checked the weather for later use
    millisWhenAstroChecked = millis();
    return;
  }
}
