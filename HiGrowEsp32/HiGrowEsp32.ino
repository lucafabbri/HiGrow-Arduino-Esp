#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <SimpleDHT.h>

Preferences preferences;

WiFiServer server(80);

SimpleDHT11 dht11;

HTTPClient http;

uint64_t chipid;

long timeout;

char deviceid[21];

void setup() {
  Serial.begin(115200);

  preferences.begin("higrow", false);

  pinMode(16, OUTPUT);

  for (int i = 0; i < 30; i++) {
    int led = digitalRead(16);
    if (led == HIGH) {
      digitalWrite(16, LOW);
    } else {
      digitalWrite(16, HIGH);
    }
    if (i == 299) {
      digitalWrite(16, HIGH);
    }
    delay(100);
  }

  if (analogRead(0) == 0) {
    preferences.clear();
  }

  timeout = 0;

  chipid = ESP.getEfuseMac();
  sprintf(deviceid, "%" PRIu64, chipid);
  Serial.println(deviceid);

  String ssid = preferences.getString("ssid");
  String pwd = preferences.getString("pwd");

  if (ssid.length() > 0 && pwd.length() > 0) {
    // We start by connecting to a WiFi network
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid.c_str(), pwd.c_str());

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }

    digitalWrite(16, HIGH);
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    //Init WiFi as Station, start SmartConfig
    WiFi.mode(WIFI_AP_STA);
    WiFi.beginSmartConfig();

    //Wait for SmartConfig packet from mobile
    Serial.println("Waiting for SmartConfig.");
    while (!WiFi.smartConfigDone()) {
      int led = digitalRead(16);
      if (led == HIGH) {
        digitalWrite(16, LOW);
      } else {
        digitalWrite(16, HIGH);
      }
      delay(500);
      Serial.print(".");
    }
    Serial.println("SmartConfig received.");

    //Wait for WiFi to connect to AP
    Serial.println("Waiting for WiFi");
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi Connected.");

    preferences.putString("ssid", WiFi.SSID());
    preferences.putString("pwd", WiFi.psk());

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  }

  server.begin();
}

void loop() {
  digitalWrite(16, LOW);
  bool isSetup = preferences.getBool("isSetup", true);
  char body[1024];
  timeout = millis();
  WiFiClient client = server.available();   // listen for incoming clients

  byte temperature = 0;
  byte humidity = 0;
  byte data[40] = {0};
  dht11.read(2, &temperature, &humidity, data);
  String did = String(deviceid);
  String water = String(analogRead(0));
  String temp = String((double)temperature);
  String humi = String((int)humidity);

  strcpy(body, "{\"deviceId\":\"");
  strcat(body, deviceid);
  strcat(body, "\",\"water\":\"");
  strcat(body, water.c_str());
  strcat(body, "\",\"humidity\":\"");
  strcat(body, humi.c_str());
  strcat(body, "\",\"temperature\":\"");
  strcat(body, temp.c_str());
  strcat(body, "\"}");
  if (!isSetup) {
    http.begin("http://higrowapp.azurewebsites.net/api/records");
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode = http.POST(body);

    Serial.println(httpResponseCode);
    Serial.println(body);
  }
  if (client) {                             // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:application/json");
            client.println();

            // the content of the HTTP response follows the header:
            client.print(body);

            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("client disonnected");
  }
  if (timeout >= (60000 * 5)) {
    preferences.putBool("isSetup", false);
    digitalWrite(16, LOW);
  }
  if (!isSetup) {
    Serial.println("Entering sleep mode");
    system_deep_sleep(60000000 * 10);
  }
}
