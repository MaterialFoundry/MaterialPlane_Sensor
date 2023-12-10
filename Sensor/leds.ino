/**
 * Handles LEDs
 */

#include "definitions.h"

unsigned long ledTimer = 0;
uint8_t ledTimerCount = 0;
float chargingLedState = 0;
bool chargingLedDir = 1;
float batteryPercentage;

/**
 * Initialize LEDs by attaching them to the ledc peripheral
 */
void initializeLeds() {
  #ifdef BATTERY_LED
    ledcAttachPin(BATTERY_LED_RED_PIN, BATTERY_LED_RED);
    ledcAttachPin(BATTERY_LED_GREEN_PIN, BATTERY_LED_GREEN);
    ledcSetup(BATTERY_LED_RED, LED_FREQ, 8);
    ledcSetup(BATTERY_LED_GREEN, LED_FREQ, 8);
  #endif
  #ifdef CONNECTION_LED
    ledcAttachPin(CONNECTION_LED_RED_PIN, CONNECTION_LED_RED);
    ledcAttachPin(CONNECTION_LED_GREEN_PIN, CONNECTION_LED_GREEN);
    ledcSetup(CONNECTION_LED_RED, LED_FREQ, 8);
    ledcSetup(CONNECTION_LED_GREEN, LED_FREQ, 8);
  #endif

  #ifdef TINYPICO_BATTERY_MONITOR
    //Disable TinyPico on-board led
    tp.DotStar_SetPower(false);
  #endif
}

uint8_t ledR, ledG;
/**
 * Main LED loop
 */
void ledLoop() {
  /* Do every 50ms */
  if (millis() - ledTimer >= 50) {
    ledTimer = millis();
    #if defined(PRODUCTION_BATTERY_MONITOR) || defined(TINYPICO_BATTERY_MONITOR)
    
      if (ledTimerCount == 0) {
      
        #ifdef PRODUCTION_BATTERY_MONITOR
          chargingStatus = MCP73871.getStatus();
          batteryPercentage = MAX17260.getPercentage();
        #else
          batteryPercentage = tpBatteryPercentage;
        #endif
      }
      ledTimerCount++;
      if (ledTimerCount >= 20) ledTimerCount = 0;

      #ifdef PRODUCTION_BATTERY_MONITOR
        if (chargingStatus == STAT_CHARGING) ledLoopCharging();
        else if (chargingStatus == STAT_CHARGED) ledLoopCharged();
        else if (chargingStatus == STAT_SHUTDOWN) ledLoopNotCharging();
      #endif

      #ifdef TINYPICO_BATTERY_MONITOR
        if (chargingStatus == "Charging") ledLoopCharging();
        else if (chargingStatus == "Charged") ledLoopCharged();
        else if (chargingStatus == "USB Not Connected") ledLoopNotCharging();
      #endif
       
      else if (ledTimerCount >= 8){
        ledR = 0;
        ledG = 0;
        ledTimerCount = 0;
      }
      else if (ledTimerCount >= 4) {
        ledR = LED_R_MAX;
        ledG = 0;
      }
      ledcWrite(BATTERY_LED_RED, ledR);
      ledcWrite(BATTERY_LED_GREEN, ledG);
    #endif
    
  }
}

#if defined(PRODUCTION_BATTERY_MONITOR) || defined(TINYPICO_BATTERY_MONITOR)
void ledLoopCharging() {
  if (chargingLedDir) {
    chargingLedState += LED_FADE_STEP;
    if (chargingLedState > LED_FADE_MAX) {
      chargingLedDir = false;
      chargingLedState = LED_FADE_MAX;
    }
  }
  else {
    chargingLedState -= LED_FADE_STEP;
    if (chargingLedState < LED_FADE_MIN) {
      chargingLedDir = true;
      chargingLedState = LED_FADE_MIN;
    }
  }
  ledR = chargingLedState * LED_R_MAX;
  ledG = 0;
}

void ledLoopCharged() {
  ledR = 0;
  ledG = LED_G_MAX;
}

void ledLoopNotCharging() {
  if (batteryPercentage < 10) {
    ledR = LED_R_MAX;
    ledG = 0;
  }
  else {
    ledR = (uint8_t)(LED_R_MAX-(batteryPercentage*LED_R_MAX/100));
    ledG = (uint8_t)(batteryPercentage*LED_G_MAX/100);
  }
}
#endif
