#include <WiFi.h>
#include <HTTPClient.h>
#include "DHT.h"
#include <ESP.h>

//#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
#define DHTTYPE DHT11   // DHT 22  (AM2302), AM2321
#define uS_TO_S_FACTOR 1000000LL

unsigned long now;
int DEEPSLEEP_SECONDS = 1800;

WiFiServer server(80);

HTTPClient http;

uint64_t chipid;

long timeout;

const int dhtpin = 22;
const int soilpin = 32;
const int POWER_PIN = 34;
const int LIGHT_PIN = 33;

// Initialize DHT sensor.
DHT dht(dhtpin, DHTTYPE);

// Temporary variables
static char celsiusTemp[7];
static char humidityTemp[7];

// Client variables 
char linebuf[80];
int charcount=0;

String ssid = "YOURSSID";
String pwd = "YOURPWD";

char deviceid[21];

void setup() {
  dht.begin();
  
  Serial.begin(115200);
  while(!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  esp_deep_sleep_enable_timer_wakeup(1800 * uS_TO_S_FACTOR);
  esp_deep_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);

  pinMode(16, OUTPUT); 
  pinMode(POWER_PIN, INPUT);
  digitalWrite(16, LOW);  

  timeout = 0;

  chipid = ESP.getEfuseMac();
  sprintf(deviceid, "%" PRIu64, chipid);
  Serial.print("DeviceId: ");
  Serial.println(deviceid);
}

void loop() {
  
  char body[1024];
  digitalWrite(16, LOW); //switched on

  sensorsData(body);
  http.begin("http://api.higrow.tech/api/records");
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST(body);
  Serial.println(httpResponseCode);
  esp_sleep_enable_timer_wakeup(DEEPSLEEP_SECONDS * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
  
}

void sensorsData(char* body){

  //This section read sensors
  timeout = millis();
  
  int waterlevel = analogRead(soilpin);
  int lightlevel = analogRead(LIGHT_PIN);
  
  waterlevel = map(waterlevel, 0, 4095, 0, 1023);
  waterlevel = constrain(waterlevel, 0, 1023);
  lightlevel = map(lightlevel, 0, 4095, 0, 1023);
  lightlevel = constrain(lightlevel, 0, 1023);
  
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float humidity = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float temperature = dht.readTemperature();
  
  float hic = dht.computeHeatIndex(temperature, humidity, false);       
  dtostrf(hic, 6, 2, celsiusTemp);               
  dtostrf(humidity, 6, 2, humidityTemp);
  
  String did = String(deviceid);
  String water = String((int)waterlevel);
  String light = String((int)lightlevel);

  strcpy(body, "{\"deviceId\":\"");
  strcat(body, did.c_str());
  strcat(body, "\",\"water\":\"");
  strcat(body, water.c_str());
  strcat(body, "\",\"light\":\"");
  strcat(body, light.c_str());
  strcat(body, "\",\"humidity\":\"");
  strcat(body, humidityTemp);
  strcat(body, "\",\"temperature\":\"");
  strcat(body, celsiusTemp);
  strcat(body, "\"}");

  if(lightlevel<100){
    DEEPSLEEP_SECONDS = 10800;
  }
}

