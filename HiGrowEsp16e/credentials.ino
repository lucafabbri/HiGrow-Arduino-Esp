/** Load WLAN credentials from EEPROM */
void loadCredentials() {
  EEPROM.begin(512);
  EEPROM.get(0, ssid);
  EEPROM.get(0+sizeof(ssid), password);
  EEPROM.get(0+sizeof(ssid)+sizeof(password), deviceid);
  char ok[2+1];
  EEPROM.get(0+sizeof(ssid)+sizeof(password)+sizeof(deviceid), ok);
  EEPROM.end();
  Serial.println(ok);
  if (String(ok) != String("OK")) {
    ssid[0] = 0;
    password[0] = 0;
    deviceid[0] = 0;
  }
}

/** Store WLAN credentials to EEPROM */
void saveCredentials() {
  EEPROM.begin(512);
  EEPROM.put(0, ssid);
  EEPROM.put(0+sizeof(ssid), password);
  EEPROM.put(0+sizeof(ssid)+sizeof(password), deviceid);
  char ok[2+1] = "OK";
  EEPROM.put(0+sizeof(ssid)+sizeof(password)+sizeof(deviceid), ok);
  EEPROM.commit();
  EEPROM.end();
}
