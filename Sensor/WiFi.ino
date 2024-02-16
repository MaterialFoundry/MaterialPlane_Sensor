/**
   Handles the WiFi connection (connecting to access point or creating its own access point)
*/

#include <ESPmDNS.h>
#include <DNSServer.h>

/**
   Clear network preferences
*/
void clearNetworkPreferences() {
  debug("STATUS - WIFI - Resetting WiFi settings");
  Serial.println("Resetting WiFi settings");
  preferences.begin("WiFi", false);
  preferences.putString("SSID", "");
  preferences.putString("deviceName", DEVICE_NAME);
  preferences.putUChar("websocketMode", WS_MODE_SERVER);
  preferences.putUShort("websocketPort", WS_PORT_DEFAULT);
  preferences.end();
}

/**
   Print WiFi status
*/
void printWiFiStatus() {
  if (wifiMode == WIFI_STA) {
    Serial.printf("Station Mode\r\nConnection Status:\t%s\r\nSSID:\t\t\t'%s'\r\nIP Address:\t\t%s\r\nDevice Name:\t\t'%s'\r\n\r\n", WiFi.status() == WL_CONNECTED ? "Connected" : "Not Connected", networkConfig.ssid, sensorIP.toString().c_str(), networkConfig.name.c_str());
  }
  else if (wifiMode == WIFI_AP) {
    Serial.printf("Access Point Mode\r\nConnection Status:\t%s\r\nAP SSID:\t\t'%s'\r\nIP Address:\t\t%s\r\nDevice Name:\t\t'%s'\r\n\r\n", WiFi.status() == WL_CONNECTED ? "Connected" : "Not Connected", networkConfig.name.c_str(), sensorIP.toString().c_str(), networkConfig.name.c_str());
  }
}

/**
   Initialize WiFi.
   Connects to access point or starts its own access point
*/
void initializeWiFi() {
  /* Get WiFi preferences */
  preferences.begin("WiFi", false);
  networkConfig.ssid = preferences.getString("SSID", networkConfig.ssid);
  networkConfig.name = preferences.getString("deviceName", networkConfig.name);
  networkConfig.websocketMode = static_cast<WS_MODE>(preferences.getUChar("websocketMode", networkConfig.websocketMode));
  networkConfig.websocketPort = preferences.getUShort("websocketPort", networkConfig.websocketPort);
  String pw = preferences.getString("password");
  preferences.end();

  /* Convert SSID and password strings to character arrays */
  char ssidArray[networkConfig.ssid.length() + 1];
  networkConfig.ssid.toCharArray(ssidArray, networkConfig.ssid.length() + 1);
  char pwArray[pw.length() + 1];
  pw.toCharArray(pwArray, pw.length() + 1);

  Serial.printf("------------------------------------\r\nInitializing WiFi\r\n\r\n");

  /* Check if ssid has characters. If so, connect to access point */
  if (networkConfig.ssid.length() == 0) {
    Serial.printf("SSID not configured, not connecting to WiFi\r\n");
  }
  else {
    Serial.printf("Attempting to connect to '%s'.", networkConfig.ssid);
    WiFi.mode(WIFI_STA);
    wifiMode = WIFI_STA;
    WiFi.begin(ssidArray, pwArray);
    int counter = 0;
    while (WiFi.status() != WL_CONNECTED) {
      counter++;
      if (counter >= WIFI_TIMEOUT * 10) {
        Serial.printf("\r\nTimed out, connection failed\r\n");
        break;
      }
      Serial.printf(".");
      delay(100);
    }
    Serial.printf("\r\n\r\n");
  }

  /* If connected to access point, print IP and configure DNS */
  if (WiFi.status() == WL_CONNECTED) {
    IPAddress IP = WiFi.softAPIP();
    sensorIP = WiFi.localIP();
    Serial.printf("\r\nWiFi connected with IP address '%s'", sensorIP.toString().c_str());
    if (configureDNS(networkConfig.name)) {
      Serial.println(" using device name '" + networkConfig.name + "'");
    }
    activateOTA();
  }
  /* If not connected to access points, start access point on sensor */
  else {

    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    WiFi.mode(WIFI_AP);
    wifiMode = WIFI_AP;

    char name[networkConfig.name.length() + 1];
    networkConfig.name.toCharArray(name, networkConfig.name.length() + 1);
    IPAddress apIP(192, 168, 4, 1);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(name, "Material Foundry");
    //dnsServer.start(53, "*", apIP);
    sensorIP = WiFi.softAPIP();
    Serial.printf("\r\nStarted WiFi access point with IP address '%s'\r\n\r\n", sensorIP.toString().c_str());
    activateOTA();

  }

  /* Print status */
  printWiFiStatus();
}

void activateOTA() {
  String n = networkConfig.name;
  char nArray[n.length() + 1];
  n.toCharArray(nArray, n.length() + 1);
  ArduinoOTA.setHostname(nArray);

  ArduinoOTA
  .onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "Firmware";
    else // U_SPIFFS
      type = "Webserver";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    debug("OTA - Start updating " + type);
  })
  .onEnd([]() {
    debug("OTA - Done");
    debug("STATUS - Restarting sensor");
    delay(1000);
    ESP.restart();
  })
  .onProgress([](unsigned int progress, unsigned int total) {
    static uint8_t otaPercentage = 255;
    uint8_t percentage = progress / (total / 100);
    if (percentage != otaPercentage) {
      otaPercentage = percentage;
      debug("OTA - Progress: " + (String)otaPercentage + '%');
    }
  })
  .onError([](ota_error_t error) {
    if (error == OTA_AUTH_ERROR) debug("OTA - Err: Auth Failed");
    else if (error == OTA_BEGIN_ERROR) debug("OTA - Err: Begin Failed");
    else if (error == OTA_CONNECT_ERROR) debug("OTA - Err: Conn Failed");
    else if (error == OTA_RECEIVE_ERROR) debug("OTA - Err: Rec Failed");
    else if (error == OTA_END_ERROR) debug("OTA - Err: End Failed");
  });

  ArduinoOTA.begin();
}

/**
   Configure DNS so "materialserver.local" points to the webserver
*/
bool configureDNS(String name) {
  char hostName[name.length()];
  name.toCharArray(hostName, name.length() + 1);

  if (!MDNS.begin(hostName)) {
    Serial.printf("ERROR: could not set up MDNS responder, device not accessible through its device name\r\n");
    return false;
  }
  return true;
}

/**
   Sets the device name by storing it in the preferences and configuring the DNS
*/
void setName(String name) {
  preferences.begin("WiFi", false);
  networkConfig.name = name;
  preferences.putString("deviceName", name);
  preferences.end();
  if (configureDNS(name)) {
    Serial.println("Device name configured to '" + (String)name + "'");
    debug("SETT - NAME - " + (String)name);
  }
  else {
    debug("ERR - WIFI - MDNS failed for '" + (String)name + "'");
  }
}

/**
   Sets the station by storing it in the preferences and restarting the sensor (allowing it to connect to the access point on boot)
*/
void setStation(String ssid, String pw) {
  preferences.begin("WiFi", false);
  preferences.putString("SSID", ssid);
  preferences.putString("password", pw);
  preferences.end();
  if (settings.debug) {
    debug("SETT - SSID - " + (String)ssid);
    debug("STATUS - Restarting sensor");
    delay(1000);
  }
  ESP.restart();
}

/**
   Scan for WiFi stations
*/
void scanWifi() {
  Serial.println("Scanning WiFi stations");
  debug("STATUS - WIFI - Scanning WiFi");
  broadcastWs("{\"status\":\"disableTimeout\"}");
  int n = WiFi.scanNetworks();

  StaticJsonDocument<JSON_OBJECT_SIZE(100)> stationsDoc;
  stationsDoc["status"] = "wifiStations";
  if (n == 0)
    debug("STATUS - WIFI - No stations found");
  StaticJsonDocument<JSON_OBJECT_SIZE(150)> stationsList;
  for (int i = 0; i < 15; i++) {
    if (n < i) break;
    String authMode = "";
    if (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) authMode = "Open";
    else if (WiFi.encryptionType(i) == WIFI_AUTH_WEP) authMode = "WEP";
    else if (WiFi.encryptionType(i) == WIFI_AUTH_WPA_PSK) authMode = "WPA-PSK";
    else if (WiFi.encryptionType(i) == WIFI_AUTH_WPA2_PSK) authMode = "WPA2-PSK";
    else if (WiFi.encryptionType(i) == WIFI_AUTH_WPA_WPA2_PSK) authMode = "WPA-WPA2-PSK";
    else if (WiFi.encryptionType(i) == WIFI_AUTH_WPA2_ENTERPRISE) authMode = "WPA2-Enterprise";
    else authMode = "Unknown";
    StaticJsonDocument<JSON_OBJECT_SIZE(10)> stationData;
    stationData["ssid"] = WiFi.SSID(i);
    stationData["rssi"] = WiFi.RSSI(i);
    stationData["authMode"] = authMode;
    stationsList.add(stationData);
  }
  stationsDoc["stations"] = stationsList;

  char output[1000];
  serializeJson(stationsDoc, output);
  broadcastWs(output);
  Serial.println(output);
  debug("STATUS - WIFI - Stations found: " + (String)n);
  broadcastWs("{\"status\":\"enableTimeout\"}");
}
