#include <WiFi.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include "DHT.h"
#include <ESP.h>

//#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
#define DHTTYPE DHT11   // DHT 22  (AM2302), AM2321
#define uS_TO_S_FACTOR 1000000

bool wifi_setup_mode = true;

Preferences preferences;

WiFiServer server(80);

HTTPClient http;

uint64_t chipid;

long timeout;

const int dhtpin = 22;
const int soilpin = 32;

// Initialize DHT sensor.
DHT dht(dhtpin, DHTTYPE);

// Temporary variables
static char celsiusTemp[7];
static char humidityTemp[7];

// Client variables 
char linebuf[80];
int charcount=0;

String ssid;
String pwd;

char deviceid[21];

void setup() {
  dht.begin();
  
  Serial.begin(115200);
  while(!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  esp_deep_sleep_enable_timer_wakeup(1800 * uS_TO_S_FACTOR);
  esp_deep_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
  preferences.begin("higrow", false);

  pinMode(16, OUTPUT); 
  pinMode(34, INPUT);
  digitalWrite(16, LOW);  

  wifi_setup_mode = digitalRead(34)==HIGH;

  timeout = 0;

  chipid = ESP.getEfuseMac();
  sprintf(deviceid, "%" PRIu64, chipid);
  Serial.println(deviceid);

  ssid = preferences.getString("ssid");
  pwd = preferences.getString("pwd");

  if (wifi_setup_mode) {
    Serial.println("Smart Config");
    
    WiFi.mode(WIFI_AP_STA);
    WiFi.beginSmartConfig();

    while (!WiFi.smartConfigDone()) {
      delay(500);
      Serial.print(".");
    }
    
    Serial.println("Got Credentials");
    
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    
    Serial.println("WiFi connected");

    preferences.putString("ssid", WiFi.SSID());
    preferences.putString("pwd", WiFi.psk());

    delay(1000);
    Serial.println(WiFi.localIP());
    preferences.putBool("isSetup", true);
    server.begin();    
  }else{
    Serial.println("WiFi");
    
    WiFi.begin(ssid.c_str(), pwd.c_str());
  
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
    }
  }

}

void loop() {
  
  char body[1024];
  digitalWrite(16, LOW); //switched on

  if(wifi_setup_mode){
    WiFiClient client = server.available(); 
    if (client) {
      sensorsData(body);
      Serial.println("New client");
      memset(linebuf,0,sizeof(linebuf));
      charcount=0;                       
      String currentLine = "";   
      while (client.connected()) {       
        if (client.available()) {         
          char c = client.read();    
          Serial.write(c);
          linebuf[charcount]=c;
          if (charcount<sizeof(linebuf)-1) charcount++;                   
          if (c == '\n') {                   
            
            if (currentLine.length() == 0) {
  
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:application/json");
              client.println();
              client.print(body);
              client.println();
              break;
            } else {   
              currentLine = "";
            }
          } else if (c != '\r') { 
            currentLine += c;  
          }
        }
      }
      delay(1);
      client.stop();
      Serial.println("client disconnected");
    }
  }else{
    sensorsData(body);
    http.begin("http://higrowapp.azurewebsites.net/api/records");
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode = http.POST(body);
    Serial.println(httpResponseCode);
    esp_deep_sleep_start();
  }
}

void sensorsData(char* body){

  //This section read sensors
  timeout = millis();
  
  int waterlevel = analogRead(soilpin)/4;
  
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float humidity = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float temperature = dht.readTemperature();
  
  float hic = dht.computeHeatIndex(temperature, humidity, false);       
  dtostrf(hic, 6, 2, celsiusTemp);               
  dtostrf(humidity, 6, 2, humidityTemp);
  
  String did = String(deviceid);
  String water = String((int)waterlevel);

  strcpy(body, "{\"deviceId\":\"");
  strcat(body, did.c_str());
  strcat(body, "\",\"water\":\"");
  strcat(body, water.c_str());
  strcat(body, "\",\"humidity\":\"");
  strcat(body, humidityTemp);
  strcat(body, "\",\"temperature\":\"");
  strcat(body, celsiusTemp);
  strcat(body, "\"}");
}

