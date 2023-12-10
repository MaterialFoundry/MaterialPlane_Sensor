/*
 * Includes and class definitions
 */
#include "definitions.h"
#include "src/WebSocketsServer/src/WebSocketsServer.h"
#include "src/IrPoint/IrPoint.h"
#include <Preferences.h>
#include <WiFi.h>
#include "src/ArduinoJson-6.x/ArduinoJson-v6.20.0.h"
#include "src/RunningAverage/RunningAverage.h"

#ifdef PAJ_SENSOR
  #include "src/PAJ7025R3/PAJ7025R3.h"
  PAJ7025R3 IRsensor(PAJ_CS_PIN, PAJ_MOSI_PIN, PAJ_MISO_PIN, PAJ_SCK_PIN, PAJ_LEDSIDE_PIN, PAJ_VSYNC_PIN, PAJ_FOD_PIN);
#endif

#ifdef WIIMOTE_SENSOR
  #include "src/wiiCam/wiiCam.h"
  wiiCam IRsensor(SDA_PIN, SCL_PIN);
#endif

#ifdef NATIVE_USB
  #include "USB.h"
#endif

#ifdef PRODUCTION_BATTERY_MONITOR
  #include "./src/MAX17260/MAX17260.h"
  #include "./src/MCP73871/MCP73871.h"
  MCP73871 MCP73871(MCP73871_USB_SEL_PIN, MCP73871_PROG2_PIN, MCP73871_TE_PIN, MCP73871_CE_PIN, MCP73871_PG_PIN, MCP73871_STAT1_PIN, MCP73871_STAT2_PIN);
  MAX17260 MAX17260(MAX17260_SDA_PIN, MAX17260_SCL_PIN, MAX17260_CLK);
  MCP73871_STAT chargingStatus;
#endif

#ifdef TINYPICO_BATTERY_MONITOR
  #include "src/tinypico-helper/src/TinyPICO.h"
  TinyPICO tp = TinyPICO();
  uint16_t tpVoltage = 0;
  bool tpUsbActive = false;
  bool tpUsbActiveOld = false;
  String chargingStatus = "Not Charging";
  RunningAverage tpAverageVoltage;
  uint8_t tpChargeSum = 0;
  uint8_t tpBatteryPercentage = 0;
#endif

#ifdef SERIAL_DEBUG
  #define Serial Serial0
#endif

Preferences preferences;
IrPoint irPoints[OBJECT_NUM];

/*
 * Task handles
 */
TaskHandle_t pingTask;
TaskHandle_t irSensorTask;
TaskHandle_t activityMonitorTask;
TaskHandle_t rmtTask;
TaskHandle_t comTask;

/*
 * Structs
 */
Settings settings;                  //Stores the general settings
SensorConfig sensorConfig;          //Stores the IR sensor config
NetworkConfig networkConfig;        //Stores the network config

/*
 * Global variables
 */
uint16_t baseId = 0;                //Stores the base ID
uint8_t baseCmd = 0;                //Stores the base command
uint8_t baseBat = 0;                //Stores the base battery status
bool started = false;               //Stores whether the sensor has finished initializing
bool serialConnected = false;       //Stores whether a serial connection has been established
bool usbConnected = false;          //Stores whether a USB cable has been plugged in
unsigned long statusTimer = 0;      //Timer to print status updates
String webserverVersion = "";       //Stores the webserver version
uint8_t wifiMode;                   //Stores the WiFi mode (access point or station)

/*
 * Prototype functions for the ir tracker
 */
void setUpdateRate(uint8_t val, bool save = false);
void setAverage(uint8_t val, bool save = true);
void setMirrorX(bool val, bool save = true);
void setMirrorY(bool val, bool save = true);
void setRotation(bool val, bool save = true);
void setOffsetX(int16_t val, bool save = true);
void setOffsetY(int16_t val, bool save = true);
void setScaleX(float val, bool save = true);
void setScaleY(float val, bool save = true);
void setCalibration(bool val, bool save = true);
void setOffsetCalibration(bool val, bool save = true);
void setBrightness(uint8_t val, bool save = true);
void setMinBrightness(uint8_t val, bool save = true);

void setup() {
  #ifdef NATIVE_USB
    delay(2000);        /* Delay to allow native USB port to initialize */
  #endif
  initialization();     /* Initialize everything */

  xTaskCreatePinnedToCore(
      comTaskLoop,
      "comTask",
      COM_STACK_SIZE,
      NULL,
      2,
      &comTask,
      1
    );
}

void loop() {
  
}

void comTaskLoop(void * parameter) {
  delay(1000);
  while(1) {
    batteryManagementLoop();
    ledLoop();
    communicationLoop();
    websocketLoop();
    serialLoop();
    rmtLoop();
    delay(10);
  }
}

/**
 * Takes an integer an returns a binary string
 */
String intToBinString(uint32_t val, uint8_t size) {
  String s = "";
  for (int i=size-1; i>=0; i--) {
    if ((val>>i)&1) s += '1';
    else s += '0';
  }
  return s;
}

/**
 * Takes a timer value and converts it into elapsed seconds
 */
uint64_t timerToSeconds(uint64_t t) {
  return (millis() - t)/1000;
}

/**
 * Convert seconds integer to a time string
 */
String secondsToTime(uint32_t s) {
  uint8_t hr = s/3600;
  uint8_t min = (s%3600)/60;
  String t = (String)hr + ':';
  if (min < 10) t += '0';
  t+= (String)min;
  return t;
}
