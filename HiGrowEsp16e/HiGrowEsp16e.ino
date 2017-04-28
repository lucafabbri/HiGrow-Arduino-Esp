#include <RestClient.h>
#include <SimpleDHT.h>
//#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>

const int SENSOR_SLEEP = 300000;

const size_t MAX_CONTENT_SIZE = 4096;
const unsigned long HTTP_TIMEOUT = 10000;  // max respone time from server

const char *softAP_ssid = "HiGrowSetupWiFi";
//const char *softAP_password = "HiGrowSetupWiFi";

const char *myHostname = "higrow";
const char* host = "higrowapp.azurewebsites.net";

char ssid[32] = "";
char password[32] = "";
char deviceid[48] = "﻿";

const byte DNS_PORT = 53;
DNSServer dnsServer;

ESP8266WebServer server(80);
SimpleDHT11 dht11;

IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);

boolean connect;
boolean sleep;
boolean setupMode;

long setupStartTime = 0;

int status = WL_IDLE_STATUS;
int waitTime = 1000;

int thGrow = D4;
int anaGrow = A0;

void setup() {

  initiate();
  setApMode();
}

void loop() {
  if(setupMode){
    apMode();
  }else{
    sensorMode();
  }
}

void initiate(){
  pinMode(thGrow,INPUT);
  setupStartTime = millis();
  
  Serial.begin(9600);
  
  loadCredentials(); // Load WLAN credentials from network
  delay(100);
  connect = strlen(ssid) > 0; // Request WLAN connect if there is a SSID
  sleep=false;
  setupMode=true;
}

void connectWifi() {
  WiFi.disconnect();
  WiFi.begin ( ssid, password );
  int connRes = WiFi.waitForConnectResult();
}

void setApMode(){
    Serial.println("Setup AP");
  waitTime = 50;
  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(softAP_ssid);
  delay(500); // Without delay I've seen the IP address blank
  
  /* Setup the DNS server redirecting all the domains to the apIP */  
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);
  
  /* Setup web pages: root, wifi config pages, SO captive portal detectors and not found. */
  server.on("/", handleRoot);
  server.on("/sensor", handleSensor);
  server.on("/wifi", handleWifi);
  server.on("/wifisave", handleWifiSave);
  server.on("/start", handleStart);
  server.on("/generate_204", handleRoot);  //Android captive portal. Maybe not needed. Might be handled by notFound handler.
  server.on("/fwlink", handleRoot);  //Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
  server.onNotFound ( handleNotFound );
  server.begin(); // Web server start
  delay(waitTime);
}

void setWifiMode(){
    Serial.println("Setup WiFi");
    waitTime = 100;
    WiFi.mode(WIFI_STA);
    delay(waitTime);
}

void apMode(){
  dnsServer.processNextRequest();
  server.handleClient();
  if((setupStartTime + 600000) < millis()){
    Serial.println("setup timeout reached");
    setupMode=false;
  }
  if(!setupMode){
    Serial.println("setup is over setting sensor mode up");
    setWifiMode();
  }
}
void sensorMode(){
    if(sleep){
      WiFi.forceSleepWake();
      delay(100);
      sleep=false;
    }
    waitTime = 500;
    if (connect) {
      Serial.println("connect wifi");
      connect = false;
      connectWifi();
    }
    int s = WiFi.status();
    if (status != s) {
      Serial.println("WiFi status changed");
      status = s;
    }
    if(status == WL_CONNECTED && strlen(deviceid) > 0){
        Serial.println("WiFi connected");
        postRecord();
        waitTime = SENSOR_SLEEP;
        sleep=true;
        
        WiFi.mode(WIFI_OFF);
        WiFi.forceSleepBegin();
        delay(100);
    } else if (status == WL_NO_SSID_AVAIL) {
        Serial.println("wifi no ssid avail");
        WiFi.disconnect();
        delay(500);
        setWifiMode();
        delay(500);
        connect = true;
    } else if (status == WL_NO_SHIELD) {
        Serial.println("wifi no shield");
    } else if (status == WL_SCAN_COMPLETED) {
        Serial.println("wifi scan completed");
    } else if (status == WL_CONNECT_FAILED) {
        Serial.println("wifi failed");
        setWifiMode();
    } else if (status == WL_CONNECTION_LOST) {
        Serial.println("wifi lost");
    } else if (status == WL_DISCONNECTED) {
        Serial.println("wifi is disconnected");
        connect = true;
    } else if (status == WL_IDLE_STATUS) {
        Serial.println("wifi is idle");
    } else {
      Serial.println(status);
      setWifiMode();
    }
    delay(waitTime);
}

void postRecord(){
  char body[1024];
  String response;

  byte temperature = 0;
  byte humidity = 0;
  byte data[40] = {0};
  if (dht11.read(thGrow, &temperature, &humidity, data)) {
    Serial.print("Read DHT11 failed");
  }
  
  String water = String(analogRead(anaGrow));
  char warray[4];
  water.toCharArray(warray,4);

  String temp = String((double)temperature);
  char tarray[4];
  temp.toCharArray(tarray,4);

  String humi = String((int)humidity);
  char harray[4];
  humi.toCharArray(harray,4);
  
  strcpy(body,"{\"deviceId\":\"");
  strcat(body,deviceid);
  strcat(body,"\",\"water\":\"");
  strcat(body,warray);
  strcat(body,"\",\"humidity\":\"");
  strcat(body,harray);
  strcat(body,"\",\"temperature\":\"");
  strcat(body,tarray);
  strcat(body,"\"}");
  Serial.println(body);
  RestClient client = RestClient(host);
  client.setContentType("application/json");
  int code = client.post("/api/records",body,&response);
  Serial.println(code);
  Serial.println(response);
}

