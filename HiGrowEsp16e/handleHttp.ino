/** Handle root or redirect to captive portal */
void handleRoot() {
//  if (captivePortal()) { // If captive portal redirect instead of displaying the page.
//    return;
//  }
  sendHtmlHeader();
  server.sendContent(
    "<h3>Modalit&agrave impostazione HiGrow</h3>"
  );
  server.sendContent(
    "<div style='display:block;'>"
    "<p>Impostazioni attuali:<br>"
  );
  server.sendContent(String("SSID: ") + ssid + "<br>");
  server.sendContent(String("Password: ") + password + "<br>");
  server.sendContent(String("Device Id: ") + deviceid + "<br></p>");
  
  server.sendContent(
    "</div>"
  );
  server.sendContent(
    "<div><a class='button' href='/wifi'>Modifica impostazioni</a></div>"
  );
  server.sendContent(
    "<div><a class='button' href='/sensor'>Dati sensore</a></div>"
  );
  sendHtmlFooter();
  server.client().stop(); // Stop is needed because we sent no content length
}

/** Redirect to captive portal if we got a request for another domain. Return true in that case so the page handler do not try to handle the request again. */
//boolean captivePortal() {
//  if (!isIp(server.hostHeader()) && server.hostHeader() != (String(myHostname)+".local")) {
//    Serial.println("Request redirected to captive portal");
//    server.sendHeader("Location", String("http://") + toStringIp(server.client().localIP()), true);
//    server.send ( 302, "text/plain", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
//    server.client().stop(); // Stop is needed because we sent no content length
//    return true;
//  }
//  return false;
//}

/** Wifi config page handler */
void handleWifi() {
  sendHtmlHeader();
  server.sendContent(
    "<h3>Connetti HiGrow alla rete WiFi</h3>"
  );
  Serial.println("scan start");
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n > 0) {
    server.sendContent(
      "\r\n<br /><form method='POST' action='wifisave' style='max-width: 600px; margin: auto;'>"
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
//  if (captivePortal()) { // If caprive portal redirect instead of displaying the error page.
//    return;
//  }
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
    "<h3>Dati sensori</h3> <div>"
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
    "%</p></div>"
  );
  server.sendContent(
    "<div><a class='button' href='/start'>Avvia modalit&agrave; sensore Iot</a></div>"
    "<div><a class='button' href='/'>Torna alla home</a></div>"
  );
  sendHtmlFooter();
  server.client().stop(); 
}

void handleStart(){
  setupMode=false;
  sendHtmlHeader();
  server.sendContent(
    "<h3>Ora HiGrow &egrave; in modalit&agrave; sensore. Inseriscilo nel terreno.</h3>"
  );
  server.sendContent(
    "<p>Per riavviare la modalit&agrave; setup riavviare il tuo device</p>"
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
    "div{margin-bottom:2em;}"
    "input,select,textarea{display: block;width: 100%;padding: 8px 16px;font-size: 18px;line-height: 22px;color: #fff;background-color: transparent;background-image: none;border: 1px solid #ccc;border-radius: 0px;-webkit-box-shadow: inset 0 1px 1px rgba(0,0,0,.075);box-shadow: inset 0 1px 1px rgba(0,0,0,.075);-webkit-transition: border-color ease-in-out .15s,-webkit-box-shadow ease-in-out .15s;-o-transition: border-color ease-in-out .15s,box-shadow ease-in-out .15s;transition: border-color ease-in-out .15s,box-shadow ease-in-out .15s;}"
    ".button{padding:8px 16px;color:#A4CA64;border:1px solid #A4CA64;text-decoration:none; font-weight:400; font-size:18px;line-height:22px; margin-bottom:2em; min-width:150px;}"
    "body{padding:0px;margin:0px;font-size:18px;line-height:22px;font-family:helvetica,sans-serif;color:white;background-color:#313130;text-align:center;font-weight:300;}"
    "</style>"
    "</head>"
    "<body>"
    "<nav style='display:block; margin:0; padding:10px;color:white;background-color:#313130; width:100%; height:80px; font-weight:600;text-align:center;'>"
    "<img src='data:image/png;base64,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'>"
    "</nav>"
    "<div style='display:block; padding:20px;color:white;background-color:#313130;'>"
  );
}

void sendHtmlFooter(){
  server.sendContent(
    "<div></body></html>"
  );
}

