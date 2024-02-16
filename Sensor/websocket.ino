/**
 * Handles the websocket communication
 */
 
int wsPort = 3000;
WebSocketsServer webSocketServer = WebSocketsServer(wsPort);

bool initialSend = true;

uint8_t getWebsocketClientNum() {
  return webSocketServer.connectedClients();
}

String getWebsocketIp(uint8_t clientNum) {
  return webSocketServer.remoteIP(clientNum).toString().c_str();
}

void websocketLoop() {
  webSocketServer.loop();
}

void printWebsocketStatus() {
  Serial.printf("Websocket mode:\t\t%s\r\nPort:\t\t\t%d\r\nConnected Clients:\t%d\r\n", websocketModeString[networkConfig.websocketMode], networkConfig.websocketPort, webSocketServer.connectedClients());
  for (int i=0; i<webSocketServer.connectedClients(); i++) Serial.printf("\t\t\t%d\t%s\r\n", i+1, getWebsocketIp(i));
  Serial.printf("\r\n");
}

void initializeWebsocket() {
  Serial.printf("------------------------------------\r\nInitializing websocket\r\n\r\n");

  if (WiFi.status() != WL_CONNECTED && wifiMode != WIFI_AP) {
    Serial.printf("Failed: No WiFi\r\n\r\n");
    return;
  }
  
  webSocketServer.begin();
  webSocketServer.onEvent(webSocketServerEvent);
  printWebsocketStatus();
}

void broadcastWs(String msg) {
  if (webSocketServer.connectedClients() > 0) webSocketServer.broadcastTXT(msg);
  if (settings.serialMode == SERIAL_DEFAULT) Serial.println(msg);
}

void broadcastWsDebug(String msg) {
  String s = "{\"status\":\"debug\",\"message\":\"" + msg + "\"}";
  if (webSocketServer.connectedClients() > 0) webSocketServer.broadcastTXT(s);
  if (settings.serialMode == SERIAL_DEFAULT) Serial.println(s);
}

void webSocketServerEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      {
          debug("STATUS - WS - DISCONNECTED - " + (String)num);
          // cancelCalibrationOnDisconnect(num);
          #ifdef CONNECTION_LED
            if (getWebsocketClientNum() == 0) {
              ledcWrite(CONNECTION_LED_RED, LED_R_MAX);
              ledcWrite(CONNECTION_LED_GREEN, 0);
            }
          #endif
      }
      break;
    case WStype_CONNECTED:
      {
        //resetWebsocketActivityTimer();
        IPAddress ip = webSocketServer.remoteIP(num);
        debug("STATUS - WS - CONNECTED - " + (String)num + " - " + (String)ip[0] + ':' + (String)ip[1] + ':' + (String)ip[2] + ':' + (String)ip[3]);
        if (initialSend) {
          delay(100);
          initialSend = false;
        }
        statusTimer = millis() - STATUS_PERIOD;
        #ifdef CONNECTION_LED
          ledcWrite(CONNECTION_LED_RED, 0);
          ledcWrite(CONNECTION_LED_GREEN, LED_G_MAX);
        #endif
      }
      break;
    case WStype_TEXT:
      {
        //resetWebsocketActivityTimer();
        if (char(payload[0]) == '{') {
          char pl[length];
          String plStr = "";
          for (int i=0; i<length; i++) {
            pl[i] = char(payload[i]);
            plStr += char(payload[i]);
          }
          analyzeJson(pl, num);
          debug("STATUS - WS - RECJ - " + (String)num + " - " + plStr);
        }
        else {
          String pl = "";
          for (int i=0; i<length; i++) pl += char(payload[i]);
          pl += ' ';
          analyzeMessage(pl);
          debug("STATUS - WS - RECS - " + (String)num + " - " + pl);
        }
      }
      break;
    case WStype_BIN:
    case WStype_PING:
    case WStype_PONG:
    case WStype_ERROR:      
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
      break;
    }
}
