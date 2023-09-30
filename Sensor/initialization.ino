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
  pinMode(TEST_PIN, INPUT_PULLUP);
  if (!digitalRead(TEST_PIN)) startTest();
  Serial.begin(115200);
  Serial.printf("Starting initialization\n");
  Serial0.begin(115200);
  Serial0.printf("Starting");
  printInitStatus();
  initializeUSB();
  initializeLeds();

  #ifdef BATTERY_LED
    /* Switch green LED off */
    ledcWrite(BATTERY_LED_GREEN, 0);
  
    /* Fade-in red LED */
    for (int i=0; i<LED_R_MAX; i++) {
      ledcWrite(BATTERY_LED_RED, i);
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
