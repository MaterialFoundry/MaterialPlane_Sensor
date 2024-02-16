/**
 * Handles the webserver
 */

#include "FS.h"
#include "SPIFFS.h"
#include "src/AsyncTCP/src/AsyncTCP.h"
#include "src/ESPAsyncWebSrv/src/ESPAsyncWebSrv.h"
#include "src/ESPAsyncWebSrv/src/SPIFFSEditor.h"

AsyncWebServer webServer(WEBSERVER_PORT);

const String fallbackServer PROGMEM = 
"<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
"<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
"<div>Material Sensor</div><br>"
"<div>Error: Could not find webserver files.<br>Please follow <a href=\"https://github.com/MaterialFoundry/MaterialPlane_Sensor/wiki/Firmware-Update#updating-the-sensor-webserver\">these</a> instructions to continue.<div><br>"
;

void printWebserverStatus() {
  Serial.printf("Webserver Version:\tv%s\r\n", webserverVersion);
  if (wifiMode == WIFI_AP) Serial.print("Link:\t\t\thttp://192.168.4.1\r\n\r\n");
  else Serial.printf("Link 1:\t\t\t'http://%s.local'\r\nLink 2:\t\t\t%s\r\n\r\n", networkConfig.name.c_str(), sensorIP.toString().c_str());
}

void initializeWebserver() {
  Serial.printf("------------------------------------\r\nInitializing webserver\r\n\r\n");

  SPIFFS.begin();

 Serial.print("Wifi Status\t");
 Serial.println(WiFi.status());
  
  if (WiFi.status() == WL_CONNECTED || wifiMode == WIFI_AP) {
    webServer.addHandler(new SPIFFSEditor(SPIFFS, "admin", "admin"));

    webServer.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
      //resetServerActivityTimer();
      request->send(200, "text/plain", String(ESP.getFreeHeap()));
    });
  
    webServer.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
  
    webServer.onNotFound([](AsyncWebServerRequest *request){
      //resetServerActivityTimer();
      Serial.printf("NOT_FOUND: ");
      if(request->method() == HTTP_GET) {
        Serial.printf("GET");
        request->send(200, "text/html", fallbackServer);
      }
        
      else if(request->method() == HTTP_POST)
        Serial.printf("POST");
      else if(request->method() == HTTP_DELETE)
        Serial.printf("DELETE");
      else if(request->method() == HTTP_PUT)
        Serial.printf("PUT");
      else if(request->method() == HTTP_PATCH)
        Serial.printf("PATCH");
      else if(request->method() == HTTP_HEAD)
        Serial.printf("HEAD");
      else if(request->method() == HTTP_OPTIONS)
        Serial.printf("OPTIONS");
      else
        Serial.printf("UNKNOWN");
      Serial.printf(" http://%s%s\r\n", request->host().c_str(), request->url().c_str());
  
      if(request->contentLength()){
        Serial.printf("_CONTENT_TYPE: %s\r\n", request->contentType().c_str());
        Serial.printf("_CONTENT_LENGTH: %u\r\n", request->contentLength());
      }
  
      int headers = request->headers();
      int i;
      for(i=0;i<headers;i++){
        AsyncWebHeader* h = request->getHeader(i);
        Serial.printf("_HEADER[%s]: %s\r\n", h->name().c_str(), h->value().c_str());
      }
  
      int params = request->params();
      for(i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isFile()){
          Serial.printf("_FILE[%s]: %s, size: %u\r\n", p->name().c_str(), p->value().c_str(), p->size());
          if ((String)(p->name().c_str()) == "update") Serial.println("Update file found: " + (String)(p->value().c_str()));
        } else if(p->isPost()){
          Serial.printf("_POST[%s]: %s\r\n", p->name().c_str(), p->value().c_str());
        } else {
          Serial.printf("_GET[%s]: %s\r\n", p->name().c_str(), p->value().c_str());
        }
      }
  
      request->send(404);
    });
   
    webServer.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      //resetServerActivityTimer();
      if(!index)
        Serial.printf("BodyStart: %u\r\n", total);
      Serial.printf("%s", (const char*)data);
      if(index + len == total)
        Serial.printf("BodyEnd: %u\r\n", total);
    });
  
    AsyncElegantOTA.begin(&webServer);
    
    webServer.begin();
  }
  else {
    Serial.printf("Failed: No WiFi\r\n\r\n");
  }
  
  webserverVersion = getWebserverVersion();
  printWebserverStatus();
}

String getWebserverVersion() {
  File file = SPIFFS.open("/main.js", FILE_READ);
  String ver = "";
  uint8_t saveState = 0;
  while(file.available()) {
    char c = file.read();
    if (c == ';') break;
    else if (saveState == 0 && c == '"') saveState = 1;
    else if (saveState == 1 && c == 'v') saveState = 2;
    else if (saveState == 2 && c == '"') break;
    else if (saveState == 2) ver += c;
  }
  file.close();
  return ver;
}
