/**
 * Handles all initialization of the sensor
 */

#include "definitions.h"
#include <nvs_flash.h>

/**
 * Clear all preferences
 */
void clearPreferences() {
  nvs_flash_erase();  /* Erase the NVS partition */
  nvs_flash_init();   /* Initialize the NVS partition */
}

/**
 * Initialize preferences (for general settings)
 */
void initializePreferences() {
  nvs_flash_init();
  preferences.begin("config", true);
  settings.debug = preferences.getBool("debug",settings.debug);
  settings.serialMode = static_cast<SERIAL_MODE>(preferences.getUChar("serialMode",settings.serialMode));
  preferences.end();
}

/**
 * Print initialization status
 */
void printInitStatus() {
  Serial.printf("\n\n------------------------------------------------------------------------\nMaterial Plane Sensor\n------------------------------------\n\nHardware Variant:\t%s\nHardware Version:\tv%s\nFirmware Version:\tv%s\n\n", HARDWARE_VARIANT, HARDWARE_VERSION, FIRMWARE_VERSION);
}

/**
 * Main initialization function from which everything is initialized
 */
void initialization() {
  
  #if defined(PRODUCTION_HW)
    Serial0.begin(115200);
    pinMode(TEST_PIN, INPUT_PULLUP);
    if (!digitalRead(TEST_PIN)) startTest();
    Serial0.printf("Starting");
  #endif
  
  Serial.begin(115200);
  Serial.printf("Starting initialization\n");
  
  printInitStatus();
  initializeUSB();
  initializeLeds();

  #ifdef BATTERY_LED
    /* Switch green LED off */
    ledcWrite(BATTERY_LED_GREEN, 0);
    #ifdef CONNECTION_LED
      ledcWrite(CONNECTION_LED_GREEN, 0);
    #endif
  
    /* Fade-in red LED */
    for (int i=0; i<LED_R_MAX; i++) {
      ledcWrite(BATTERY_LED_RED, i);
      #ifdef CONNECTION_LED
        ledcWrite(CONNECTION_LED_RED, i);
      #endif
      delay(10);
    }
  #endif

  #ifdef EN_SW
    pinMode(EN_SW_PIN, INPUT);
  #endif
  
  initializePreferences();
  initializeWiFi();
  initializeWebserver();
  initializeWebsocket();
  initializeCommunication();
  initializeBatteryManagement();
  initializeIrTracker();
  initializeRmt();
  initializeActivityMonitor();

  /* Disable bluetooth radio */
  btStop();

  /* Set to true so other functions know initialization is done */
  started = true;
  
  Serial.printf("------------------------------------\nInitialization done\n------------------------------------------------------------------------\n\n");

  #ifdef BATTERY_LED
    /* Fade-out red LED */
    for (int i=LED_R_MAX; i>=0; i--) {
      ledcWrite(BATTERY_LED_RED, i);
      delay(10);
    }
  #endif
}
