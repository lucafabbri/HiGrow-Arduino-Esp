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

