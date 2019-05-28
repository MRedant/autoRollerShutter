#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         // https://github.com/tzapu/WiFiManager
#include <time.h>

// Web server port 80
WiFiServer server(80);
WiFiClient client;


//1 day = 86400000 milliseconds
#define A_DAY 86400000;
#define AN_HOUR 3600000;

String latitude = "51.02";
String longitude = "3.67";
String apiKey = "90b1bade0633eb6ac8546782d2f4cbb5";

//check whether the button went to low before accepting new high
bool previousButtonStatusWasLow = true;

//store info about api call astro-info
unsigned long millisWhenAstroChecked = 0;
const unsigned long astroCheckInterval = A_DAY;
//store info about api call time-info
unsigned long millisWhenTimeChecked = 0;
const unsigned long timeCheckInterval = AN_HOUR;
//store moment when sunsetSunrise[0] = sunset - sunsetSunrise[1] = sunrise
unsigned long sunsetSunrise[2] = {0, 0};
//store how long it takes until sunset/sunrise
unsigned long timeUntilSunsetSunrise[2] = {0, 0};

//to prevent the unnecessary pulses to the motor controller
int today;
bool alreadyClosedToday = false;
bool alreadyOpenedToday = false;

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

  //setup wifi
  WiFiManager wifiManager;
  // wifiManager.resetSettings();
  wifiManager.autoConnect("AutoConnectAP");
  Serial.println("Connected.");

  //setup time
  configTime(2 * 3600, 0, "pool.ntp.org", "time.nist.gov"); //timezone = 2 - dst = 0
  Serial.println("\nWaiting for Internet time");
  while (!time(nullptr)) {
    Serial.print("*");
    delay(1000);
  }
  Serial.println("\nTime response....OK");

  //webserver
  server.begin();
}

void loop() {

  // Listen for incoming clients;
  client  = server.available();

  //astro times checked once a Day or after reboot
  if (millis() - millisWhenAstroChecked > astroCheckInterval || millisWhenAstroChecked == 0 ) {
    Serial.println("connecting...");
    // if there's a successful connection:
    if (client.connect("api.openweathermap.org", 80)) {

      // send the HTTP PUT request:
      client.println("GET /data/2.5/weather?lat=" + latitude + "&lon=" + longitude + "&APPID=" + apiKey);
      client.println("Host: api.openweathermap.org");
      client.println("User - Agent: ESP8266 / 1.1");
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

      //save the time when we checked the weather for later use
      millisWhenAstroChecked = millis();
      delay(3000);

      parseJson(client, sunsetSunrise);
      client.stop();
    }
    else {
      // if no connection was made:
      Serial.println("connection failed");
      delay(1000);
      return;
    }
  }

  //time checked once per Hour or after reboot
  if (millis() - millisWhenTimeChecked > timeCheckInterval || millisWhenTimeChecked == 0 ) {
    time_t now = time(nullptr);
    struct tm* p_tm = localtime(&now);
    millisWhenTimeChecked = millis();
    Serial.println(ctime(&now));
  }

  //disable-enable automatic shutters button
  if (digitalRead(manualControlInput) == HIGH && previousButtonStatusWasLow) {
    digitalWrite(manualControlStatusPin, (digitalRead(manualControlStatusPin)) == HIGH ? LOW : HIGH);
    Serial.print("automatic shutters ");
    Serial.println((digitalRead(manualControlStatusPin) == HIGH) ? "DISABLED" : "ENABLED");
    previousButtonStatusWasLow = false;
    delay(300);
  } else if (digitalRead(manualControlInput) == LOW) {
    previousButtonStatusWasLow = true;
  }

  //todo : put stores up/down in case of sunset/sunrise passed
}

//to parse json data recieved from OWM
void parseJson(WiFiClient client, unsigned long sunsetSunrise[2]) {

  //memory allocation needed (calculated on: https://arduinojson.org/v6/assistant/)
  const size_t capacity = 900;
  DynamicJsonDocument doc(capacity);
  //parse
  DeserializationError err = deserializeJson(doc, client);
  //parse succeeded?
  if (err) {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(err.c_str());
  } else {
    delay(3000);

    JsonObject sys = doc["sys"];
    sunsetSunrise[0] = sys["sunrise"].as<long>();
    Serial.print("sunrise ");
    Serial.println(sunsetSunrise[0]);
    sunsetSunrise[1] = sys["sunset"].as<long>();
    Serial.print("sunset ");
    Serial.println(sunsetSunrise[1]);
  }
}
