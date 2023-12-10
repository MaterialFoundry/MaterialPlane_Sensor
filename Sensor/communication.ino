/**
 * Handles communication, including serial communication and analyzing and preparing websocket communication
 */

#define COMBUFFERSIZE 300

uint16_t updatePeriod = 16;
unsigned long updateTimer = 0;

/**
 * Ping loop to keep the websocket connection alive, called from pingTask
 */
void pingLoop(void * parameter) {
  while(1) {
    broadcastWs(PING_STRING);
    //if (serialConnected && settings.debug) Serial.println(PING_STRING);
    delay(PING_PERIOD);
  }
}

/**
 * Communication loop that determines when to transmit status and coordinate updates
 */
void communicationLoop() {
  if (millis() - statusTimer >= STATUS_PERIOD) {
    statusTimer = millis();
    transmitStatus(false);
  }
  
  if (millis() - updateTimer >= framePeriodLUT[sensorConfig.updateRate]) {
    updateTimer = millis();
    transmitCoordinates();
  }
  //delay(1);
}

/**
 * Initialize communication
 */
void initializeCommunication() {
  /* Create pingLoop task */
  xTaskCreatePinnedToCore(
      pingLoop,         /* Function to implement the task */
      "PingTask",       /* Name of the task */
      PING_STACK_SIZE,  /* Stack size in words */
      NULL,             /* Task input parameter */
      20,               /* Priority of the task */
      &pingTask,        /* Task handle. */
      1                 /* Core where the task should run */
  );
}

/**
 * Transmit status over websocket and/or serial
 */
void transmitStatus(bool forceTransmit) {
  StaticJsonDocument<JSON_OBJECT_SIZE(100)> statusDoc;
  statusDoc["status"] = "update";
  statusDoc["firmwareVersion"] = FIRMWARE_VERSION;
  statusDoc["webserverVersion"] = webserverVersion;
  statusDoc["hardwareVersion"] = HARDWARE_VERSION;
  statusDoc["hardwareVariant"] = HARDWARE_VARIANT;
  statusDoc["uptime"] = (unsigned long)(millis()/1000);

  StaticJsonDocument<JSON_OBJECT_SIZE(15)> sett;
  sett["debug"] = settings.debug;
  sett["serialMode"] = serialModeString[settings.serialMode];
  sett["usbConnected"] = usbConnected;
  sett["serialConnected"] = serialConnected;
  statusDoc["settings"] = sett;

  uint8_t clientNum = 0;
  if (networkConfig.websocketMode == WS_MODE_SERVER) clientNum = getWebsocketClientNum();
  StaticJsonDocument<JSON_OBJECT_SIZE(10)> clientList;
  for (int i=0; i<clientNum; i++) {
      clientList.add(getWebsocketIp(i));
  }
  StaticJsonDocument<JSON_OBJECT_SIZE(15)> network;
  network["ssid"] = networkConfig.ssid;
  network["ip"] = WiFi.localIP();
  network["name"] = networkConfig.name;
  network["wifiConnected"] = WiFi.isConnected();
  network["websocketMode"] = websocketModeString[networkConfig.websocketMode];
  network["websocketPort"] = networkConfig.websocketPort;
  network["connectedClients"] = clientNum;
  network["clients"] = clientList;
  statusDoc["network"] = network;

  StaticJsonDocument<JSON_OBJECT_SIZE(19)> ir;
  ir["calibrationEnable"] = sensorConfig.calibrationEnable;
  ir["offsetEnable"] = sensorConfig.calibrationOffsetEnable;
  ir["mirrorX"] = sensorConfig.mirrorX;
  ir["mirrorY"] = sensorConfig.mirrorY;
  ir["rotation"] = sensorConfig.rotation;
  ir["offsetX"] = sensorConfig.offsetX;
  ir["offsetY"] = sensorConfig.offsetY;
  ir["scaleX"] = sensorConfig.scaleX;
  ir["scaleY"] = sensorConfig.scaleY;
  ir["average"] = sensorConfig.average;
  ir["brightness"] = sensorConfig.brightness;
  ir["minBrightness"] = sensorConfig.minBrightness;
  ir["updateRate"] = sensorConfig.updateRate;
  statusDoc["ir"] = ir;

  #ifdef PRODUCTION_BATTERY_MONITOR
    StaticJsonDocument<JSON_OBJECT_SIZE(11)> maxAlertList;
    for (int i=0; i<11; i++) {
      if (MAX17260alertFlags>>i) maxAlertList.add(MAX17260.alertFlagsToString(i));
    }
  
    StaticJsonDocument<JSON_OBJECT_SIZE(50)> power;
    power["percentage"] = (uint8_t)MAX17260.getPercentage();
    power["voltage"] = (int16_t)MAX17260.getAverageVoltage();
    power["current"] = (int16_t)MAX17260.getAverageCurrent();
    power["capacity"] = (uint16_t)MAX17260.getCapacity();
    power["fullCapacity"] = (uint16_t)MAX17260.getFullCapacity();
    power["currentMode"] = MCP73871.getCurrentModeString();
    if (MCP73871.getStatus() == STAT_CHARGING) power["time"] = MAX17260.getTimeToFull();
    else if (MCP73871.getStatus() == STAT_SHUTDOWN) power["time"] = MAX17260.getTimeToEmpty();
    else power["time"] = 0;
    power["fuelGaugeFault"] = maxAlertList;
    power["chargerStatus"] = MCP73871.getStatusString();
    power["chargerStatusRaw"] = MCP73871.getStatusRaw();
    power["source"] = digitalRead(LM66200_POWER_STATE_PIN) ? "Dock" : "USB";
    statusDoc["power"] = power;
  #endif

  #ifdef TINYPICO_BATTERY_MONITOR
    StaticJsonDocument<JSON_OBJECT_SIZE(50)> power;
    power["percentage"] = tpBatteryPercentage;
    power["voltage"] = tpVoltage;
    power["chargerStatus"] = chargingStatus;
    statusDoc["power"] = power;
  #endif

  char output[1000];
  
  serializeJson(statusDoc, output);
  broadcastWs(output);
  if (settings.debug || forceTransmit) Serial.println(output);
}

/**
 * Transmit coordinates over websocket and/or serial
 */
void transmitCoordinates() {
  uint8_t detectedPoints = 0;
  uint8_t validPoints = 0;
  
  StaticJsonDocument<JSON_OBJECT_SIZE(160)> points;

  /* Check all ir points */
  for (int i=0; i<OBJECT_NUM; i++) {
    /* If ir point is valid (has a brightness value) */
    if (irPoints[i].valid) {
      /* Add point to json document */
      detectedPoints++;
      StaticJsonDocument<JSON_OBJECT_SIZE(10)> point;
      point["number"] = irPoints[i].number;
      point["x"] = irPoints[i].x;
      point["y"] = irPoints[i].y;
      point["avgBrightness"] = irPoints[i].avgBrightness;
      point["maxBrightness"] = irPoints[i].maxBrightness;
      point["area"] = irPoints[i].area;
      if (irPoints[i].x != -9999 && irPoints[i].y != -9999) validPoints++;
      irPoints[i].clearInvalidPoint();
      points.add(point);
    }
  }
  if (detectedPoints == 0) return;

  /* Add general data to json document (id, cmd, battery) */
  StaticJsonDocument<JSON_OBJECT_SIZE(100)> coordsDoc;
  coordsDoc["status"] = "IR data";
  coordsDoc["detectedPoints"] = detectedPoints;
  coordsDoc["id"] = validPoints > 0 ? baseId : 0;
  coordsDoc["command"] = validPoints > 0 ? baseCmd : 0;
  coordsDoc["battery"] = baseBatToPercentage(baseBat);
  coordsDoc["irPoints"] = points;
  
  char output[1000];
  serializeJson(coordsDoc, output);
  broadcastWs(output);
 
  if (settings.debug) Serial.println(output);
}

/**
 * Convert base/pen battery state to a percentage
 */
uint8_t baseBatToPercentage(uint8_t bat) {
  if (bat == 0) return 10;
  if (bat == 1) return 25;
  if (bat == 2) return 40;
  if (bat == 3) return 55;
  if (bat == 4) return 70;
  if (bat == 5) return 85;
  if (bat == 6) return 100;
  return 100;
}

/**
 * Analyze incoming json data and perform relevant action
 */
void analyzeJson(char* msg, uint8_t source) {
  //Serial0.print("Analyze JSON: ");
  //Serial0.println(msg);
  
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, msg);
  
  if (doc["settings"]["debug"]) setDebug((uint8_t)doc["settings"]["debug"]);
  if (doc["settings"]["serialMode"]) setSerialMode((uint8_t)doc["settings"]["serialMode"]);

  if (doc["ir"]["updateRate"]) setUpdateRate(doc["ir"]["updateRate"], true);
  if (doc["ir"]["brightness"]) setBrightness(doc["ir"]["brightness"]);
  if (doc["ir"]["minBrightness"]) setMinBrightness(doc["ir"]["minBrightness"]);
  if (doc["ir"]["average"]) setAverage(doc["ir"]["average"]);
  if (doc["ir"]["mirrorX"]) setMirrorX((uint8_t)doc["ir"]["mirrorX"]);
  if (doc["ir"]["mirrorY"]) setMirrorY((uint8_t)doc["ir"]["mirrorY"]);
  if (doc["ir"]["rotation"]) setRotation((uint8_t)doc["ir"]["rotation"]);
  if (doc["ir"]["offsetX"]) setOffsetX(doc["ir"]["offsetX"]);
  if (doc["ir"]["offsetY"]) setOffsetY(doc["ir"]["offsetY"]);
  if (doc["ir"]["scaleX"]) setScaleX(doc["ir"]["scaleX"]);
  if (doc["ir"]["scaleY"]) setScaleY(doc["ir"]["scaleY"]);
  if (doc["ir"]["calibration"]) setCalibration((uint8_t)doc["ir"]["calibration"]);
  if (doc["ir"]["offsetCalibration"]) setOffsetCalibration((uint8_t)doc["ir"]["offsetCalibration"]);

  if (doc["network"]["name"]) setName(doc["network"]["name"]);
  
  if (doc["event"] == "autoExposure") performAutoExposure();
  if (doc["event"] == "restart") ESP.restart();
  if (doc["event"] == "calibration") {
    if (doc["state"] == "start") performCalibration(doc["mode"], source);
    else if (doc["state"] == "next") nextCalibrationPoint();
    else if (doc["state"] == "cancel") cancelCalibration();
  }
  if (doc["event"] == "getStatus") transmitStatus(true);

  if (doc["event"] == "scanWifi") scanWifi();
  if (doc["event"] == "connectWifi") {
    String ssid = doc["ssid"];
    String pw = doc["pw"];
    setStation(ssid, pw);
  }

  if (doc["event"] == "reset") {
    Serial.println("reset");
    bool bat = doc["battery"];
    Serial.println(bat);
    if (doc["ir"] == 1) clearIrPreferences();
    if (doc["battery"] == 1) clearBatteryPreferences();
    if (doc["network"] == 1) {
      clearNetworkPreferences();
      debug("STATUS - Restarting sensor");
      delay(1000);
      ESP.restart();
    }
  }
  statusTimer = millis() - STATUS_PERIOD;
}

/**
 * Check the serial port for new messages to analyze
 */
uint8_t serialLoop() {
  if (Serial.available() > 0) {
    resetSerialActivityTimer();
    return analyzeMessage(Serial.readStringUntil('\n'));
  }
  else return 0;
}

/**
 * Analyze incoming string message and perform relevant action
 */
uint8_t analyzeMessage(String msg) {
  //Serial.println("Received message: " + msg);
  
  /* In case of JSON string, return */
  if (msg[0] == '{') {
    char msgArray[msg.length()+1];
    msg.toCharArray(msgArray, msg.length()+1);
    analyzeJson(msgArray,255);
    return 0;
  }

  String recArray[6] = {"","","","","",""};   /* Stores the read string as individual words */
  uint8_t counter = 0;                        /* Word counter */
  
  /* Check each individual character in the received string */
  for (int i=0; i<(msg.length()-1); i++) {
    /* If the character is a space, increment the word counter */
    if (msg[i] == ' ') 
      counter++;

    /* Else if the character is ", add all following characters together, until the next " */
    else if (msg[i] == '\"') {
      i++;
      for (i; i<(msg.length()-1); i++){
        if (msg[i] == '\"') break;
        else recArray[counter] += msg[i];
      }
    }

    /* Else add the character to the current word */
    else                                                                            
      recArray[counter] += msg[i];
  }

 /*
  Serial.print("Decoded: ");
  for (int i=0; i<6; i++) {
    Serial.print(recArray[i] + ';');
  }
  Serial.println();
  */

  /*********************** Perform relevant action ***********************/
  if (recArray[0] == "SCAN" && recArray[1] == "WIFI") {
    Serial.println("Scanning for WIFI stations. Please wait.");
    int n = WiFi.scanNetworks();
    String wsMsg = "{\"status\":\"wifiStations\",\"data\":[";
    String msg = "";
    if (n==0)
      msg = "No stations found"; 
    else {
      msg += (String)n + " stations found\nNr\tSSID\t\t\tRSSI\tAuthentication Mode\n";
      for (int i = 0; i < n; ++i) {
        String authMode = "";
        if (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) authMode = "Open";
        else if (WiFi.encryptionType(i) == WIFI_AUTH_WEP) authMode = "WEP";
        else if (WiFi.encryptionType(i) == WIFI_AUTH_WPA_PSK) authMode = "WPA-PSK";
        else if (WiFi.encryptionType(i) == WIFI_AUTH_WPA2_PSK) authMode = "WPA2-PSK";
        else if (WiFi.encryptionType(i) == WIFI_AUTH_WPA_WPA2_PSK) authMode = "WPA-WPA2-PSK";
        else if (WiFi.encryptionType(i) == WIFI_AUTH_WPA2_ENTERPRISE) authMode = "WPA2-Enterprise";
        else if (WiFi.encryptionType(i) == WIFI_AUTH_WPA3_PSK) authMode = "WPA3-PSK";
        else if (WiFi.encryptionType(i) == WIFI_AUTH_WPA2_WPA3_PSK) authMode = "WPA2-WPA3-PSK";
        else if (WiFi.encryptionType(i) == WIFI_AUTH_WAPI_PSK) authMode = "WAPI-PSK";
        else authMode = "Unknown";
        msg += (String)(i + 1) + "\t" + (String)WiFi.SSID(i) + "\t\t\t" + (String)WiFi.RSSI(i) + "dBm\t" + authMode + '\n';
      }
    }
    Serial.println(msg);
  }
  else if (recArray[0] == "CONNECT" && recArray[1] == "WIFI"){
      setStation(recArray[2], recArray[3]);
  }
  else if (recArray[0] == "STATUS") {
    printStatus();
  }
  else if (recArray[0] == "PRINT"&& recArray[1] == "LOG") {
    //printLog();
  }

  return 0;
}

/**
 * Print status by calling all relevant printStatus functions
 */
void printStatus() {
  printInitStatus();
  Serial.printf("WiFi\n");
  printWiFiStatus();
  Serial.printf("Webserver\n");
  printWebserverStatus();
  Serial.printf("Websocket\n");
  printWebsocketStatus();
  Serial.printf("Battery\n");
  printBatteryStatus();
  Serial.printf("IR\n");
  printIRStatus();
  Serial.printf("------------------------------------------------------------------------\n\n");
}

/**
 * Print debug message
 */
void debug(String msg) {
  if (!settings.debug) return;
  broadcastWsDebug(msg);
  Serial.println(msg);
}

/**
 * Enable or disable debugging
 */
void setDebug(bool en) {
  debug("SETT - DEBUG - " + (String)en);
  settings.debug = en;
  preferences.begin("config", false);
  preferences.putBool("debug",en);
  preferences.end();
}

void setSerialMode(bool en) {
  debug("SETT - SERIAL - " + (String)en);
  settings.serialMode = en ? SERIAL_DEFAULT : SERIAL_OFF;
  preferences.begin("config", false);
  preferences.putUChar("serialMode",settings.serialMode);
  preferences.end();
}
