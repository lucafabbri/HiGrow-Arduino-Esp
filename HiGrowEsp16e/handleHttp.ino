/** Handle root or redirect to captive portal */
void handleRoot() {
  if (captivePortal()) { // If captive portal redirect instead of displaying the page.
    return;
  }
  sendHtmlHeader();
  server.sendContent(
    "<h1>HiGrow!</h1>"
    "<h3>Hai accesso alla modalit&agrave per impostare il tuo sensore HiGrow</h3>"
  );
  server.sendContent(
    "<p>Impostazioni attuali:<br>"
  );
  server.sendContent(String("SSID: ") + ssid + "<br>");
  server.sendContent(String("Password: ") + password + "<br>");
  server.sendContent(String("Device Id: ") + deviceid + "<br>");
  server.sendContent(
    "</p><p><a class='button' href='/wifi'>Impostazioni</a></p>"
  );
  server.sendContent(
    "<p><a class='button' href='/sensor'>Sensore</a></p>"
  );
  sendHtmlFooter();
  server.client().stop(); // Stop is needed because we sent no content length
}

/** Redirect to captive portal if we got a request for another domain. Return true in that case so the page handler do not try to handle the request again. */
boolean captivePortal() {
  if (!isIp(server.hostHeader()) && server.hostHeader() != (String(myHostname)+".local")) {
    Serial.println("Request redirected to captive portal");
    server.sendHeader("Location", String("http://") + toStringIp(server.client().localIP()), true);
    server.send ( 302, "text/plain", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
    server.client().stop(); // Stop is needed because we sent no content length
    return true;
  }
  return false;
}

/** Wifi config page handler */
void handleWifi() {
  sendHtmlHeader();
  server.sendContent(
    "<h1>Connetti HiGrow, seleziona la rete</h1>"
  );
  Serial.println("scan start");
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n > 0) {
    server.sendContent(
      "\r\n<br /><form method='POST' action='wifisave'><h4>Connetti al network:</h4>"
    );
    server.sendContent(
      "<select name='n'>"
      );
    for (int i = 0; i < n; i++) {
      server.sendContent(String() + "<option value='"+WiFi.SSID(i)+"'>SSID " + WiFi.SSID(i) + String((WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":" *") + " (" + WiFi.RSSI(i) + ")</option>");
    }
    server.sendContent(
      "</select>"
      "<br /><input type='password' placeholder='password' name='p'/>"
      "<br /><input type='deviceid' value='"+(String)deviceid+"' placeholder='Device Id' name='d'/>"
      "<br /><input type='submit' class='button' value='Salva'/></form>"
      );
  }else{
    server.sendContent(
      "<p>Nessuna WiFi trovata :(</p>"
      );
  }
  server.sendContent(
    "<p><a class='button' href='/'>Torna alla home</a></p>"
  );
  sendHtmlFooter();
  server.client().stop(); // Stop is needed because we sent no content length
}

/** Handle the WLAN save form and redirect to WLAN config page again */
void handleWifiSave() {
  Serial.println("wifi save");
  server.arg("n").toCharArray(ssid, sizeof(ssid) - 1);
  server.arg("p").toCharArray(password, sizeof(password) - 1);
  server.arg("d").toCharArray(deviceid, sizeof(deviceid) - 1);
  server.sendHeader("Location", "sensor", true);
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send ( 302, "text/plain", "");  // Empty content inhibits Content-length header so we have to close the socket ourselves.
  server.client().stop(); // Stop is needed because we sent no content length
  saveCredentials();
  connect = strlen(ssid) > 0; // Request WLAN connect with new credentials if there is a SSID
}

void handleNotFound() {
  if (captivePortal()) { // If caprive portal redirect instead of displaying the error page.
    return;
  }
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send ( 404, "text/plain", message );
}

void handleSensor(){
  sendHtmlHeader();
  server.sendContent(
    "<h1>LA TUA PIANTA</h1>"
  );
  double anavalue = ((double)analogRead(anaGrow)/1023)*100;
  byte temperature = 0;
  byte humidity = 0;
  byte data[40] = {0};
  if (dht11.read(thGrow, &temperature, &humidity, data)) {
    Serial.print("Read DHT11 failed");
  }
  server.sendContent(
    "<p>il livello di idratazione del terreno &egrave;: "
  );
  int level = ((anavalue)>100)?100:(int)(anavalue);
  server.sendContent(String(level));
  server.sendContent(
    "%</p>"
  );
  server.sendContent(
    "<p>la temperatura dell\'ambiente &egrave;: "
  );
  server.sendContent(String((double)temperature));
  server.sendContent(
    "C&deg;</p>"
  );
  server.sendContent(
    "<p>il livello di umidita dell\'ambiente &egrave;: "
  );
  server.sendContent(String(humidity));
  server.sendContent(
    "%</p>"
  );
  server.sendContent(
    "<p><a class='button' href='/start'>Avvia modalit&agrave; sensore Iot</a></p>"
    "<p><a class='button' href='/'>Torna alla home</a></p>"
  );
  sendHtmlFooter();
  server.client().stop(); 
}

void handleStart(){
  setupMode=false;
  sendHtmlHeader();
  server.sendContent(
    "<h1>MODALIT&Agrave; IOT AVVIATA.</h1><h2>Inserisci il tuo sensore HiGrow nel terreno.</h2>"
  );
  server.sendContent(
    "<p>Per riavviare la modalità setup riavviare il tuo device</p>"
  );
  sendHtmlFooter();
  server.client().stop(); 
}

void sendHtmlHeader(){
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
  server.sendContent(
    "<html>"
    "<head>"
    "<title>HiGrow iot sensor by Zepfiro v0.1</title>"
    "<style>"
    "input,select,textarea{display: block;width: 100%;padding: 8px 16px;font-size: 18px;line-height: 22px;color: #555;background-color: #fff;background-image: none;border: 1px solid #ccc;border-radius: 0px;-webkit-box-shadow: inset 0 1px 1px rgba(0,0,0,.075);box-shadow: inset 0 1px 1px rgba(0,0,0,.075);-webkit-transition: border-color ease-in-out .15s,-webkit-box-shadow ease-in-out .15s;-o-transition: border-color ease-in-out .15s,box-shadow ease-in-out .15s;transition: border-color ease-in-out .15s,box-shadow ease-in-out .15s;}"
    ".button{padding:8px 16px;color:#A4CA64;border:2px solid #A4CA64;text-decoration:none; font-weight:600;background-color:white;font-size:18px;line-height:22px;}"
    "body{padding:0px;margin:0px;font-size:18px;line-height:22px;font-family:helvetica,sans-serif;}"
    "</style>"
    "</head>"
    "<body>"
    "<nav style='display:block; margin:0; padding:10px; background-color:#A4CA64; color:white; width:100%; height:80px; font-weight:600;'>"
    
    "<img style='float:left;' src='data:image/png;base64,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'>"
    "</nav>"
    "<div style='display:block; padding:20px;'>"
  );
}

void sendHtmlFooter(){
  server.sendContent(
    "<div></body></html>"
  );
}

