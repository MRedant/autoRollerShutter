#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         // https://github.com/tzapu/WiFiManager
#include <time.h>

// Set web server port number to 80
WiFiServer server(80);
WiFiClient client;


//1 day = 86400000 milliseconds
const unsigned long aDay = 86400000;
const unsigned long anHour = 3600000;
String latitude = "51.02";
String longitude = "3.67";
String apiKey = "90b1bade0633eb6ac8546782d2f4cbb5";

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

//to prevent the continuously giving pulses to the motor controller
int today;
bool alreadyClosedToday = false;
bool alreadyOpenedToday = false;

//naming the used I/O pins
const int shutterUpPin = 5;
const int shutterDownPin = 4;
const int manualControlStatusPin = 0;
const int manualControlInput = 2;

bool buttonPressed = false; //value to check if the button is continuously pressed

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

  client  = server.available();   // Listen for incoming clients;

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
  if (digitalRead(manualControlInput) == HIGH && !buttonPressed) {
    digitalWrite(manualControlStatusPin, (digitalRead(manualControlStatusPin)) == HIGH ? LOW : HIGH);
    Serial.print("automatic shutters ");
    Serial.println((digitalRead(manualControlStatusPin) == HIGH) ? "DISABLED" : "ENABLED");
    delay(300);
    buttonPressed = true;
  } else {
    buttonPressed = false;
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
