/**
 * Handles activity tracking. In case of no activity it recudes the CPU speed to reduce power comsumption
 */

#include "definitions.h"

uint64_t irTrackerActivityTimer, irTrackerResetTimer, irComActivityTimer, websocketActivityTimer, serverActivityTimer, serialActivityTimer = 0;
uint32_t minCpuFreq = 10000000;
PowerModes powerMode = POWER_ACTIVE;
bool idle = false;

/**
 * Activity monitor task function
 */
void activityMonitor(void * parameter) {
  irTrackerActivityTimer = millis();
  irTrackerResetTimer = millis();
  irComActivityTimer = millis();
  websocketActivityTimer = millis();
  serialActivityTimer = millis();
  serverActivityTimer = millis();
  delay(1000);
  
  while(1) {

    /* If all activity timers have exceeded their timeout values, set setIdle to true */
    bool setIdle = false;
    if (timerToSeconds(irTrackerActivityTimer) > IDLE_TIMEOUT && timerToSeconds(irComActivityTimer) > IDLE_TIMEOUT && timerToSeconds(websocketActivityTimer) > IDLE_TIMEOUT && timerToSeconds(serverActivityTimer) > IDLE_TIMEOUT && timerToSeconds(serialActivityTimer) > IDLE_TIMEOUT)
       setIdle = true;
    else
      idle = false;

    #ifdef PAJ_SENSOR
      /* If sensor is idle, or if maxreset timeout has passed => reset ir sensor to prevent it from locking up */
      if ((idle && timerToSeconds(irTrackerResetTimer) >= IRTRACKER_RESET_TIMEOUT) || timerToSeconds(irTrackerResetTimer) >= IRTRACKER_MAXRESET_TIMEOUT) {
        irTrackerResetTimer = millis();
        debug("STATUS - IR - Safety Reset");
        initializeIrTracker();
      }
    #endif

    /* if setIdle is true and sensor is currently not idle, reduce the update rate and CPU speed to reduce power consumption */
    if (setIdle && !idle) {
      idle = true;
      setUpdateRate(0, false);
      setPowerMode(POWER_IDLE);
    }

    delay(ACTIVITY_MONITOR_DELAY);
  }
}

/**
 * Initialize the activity monitor
 */
void initializeActivityMonitor() {
  /* Get the crystal frequency to determine the minimum allowable cpu frequency */
  uint32_t freq = getXtalFrequencyMhz();
  for (int i=0; i<3; i++) {
    if (freq == xTalFreqLUT[i]) {
      minCpuFreq = minCpuFreqLUT[i];
      break;
    }
    
    /* If no valid min cpu freq is found, stick to idle freq (80MHz) */
    if (i == 2) minCpuFreq = CPU_FREQ_IDLE;
  }
  Serial.print("Xtal freq: ");
  Serial.println(freq);
  Serial.print("Min CPU freq: ");
  Serial.println(minCpuFreq);

  /* Create the activity monitor task */
  xTaskCreatePinnedToCore(
    activityMonitor,
    "activityMonitor",
    ACTIVITY_MONITOR_STACK_SIZE,
    NULL,
    5,
    &activityMonitorTask,
    1
  );
}

/**
 * Set the power mode
 */
void setPowerMode(PowerModes pMode) {
    if (pMode == powerMode) return;
    #if defined(PRODUCTION_HW)
      String msg = "";
      if (pMode == POWER_ACTIVE) {
        setCpuFrequencyMhz(CPU_FREQ_MAX);
        msg = "STATUS - CPU - Active - " + (String)CPU_FREQ_MAX + " MHz";
      }
      else if (pMode == POWER_IDLE) {
        setCpuFrequencyMhz(CPU_FREQ_IDLE);
        msg = "STATUS - CPU - Idle - " + (String)CPU_FREQ_IDLE + " MHz";
      }
      else if (pMode == POWER_SAVE) {
        setCpuFrequencyMhz(minCpuFreq);
        msg = "STATUS - CPU - Power Saving - " + (String)minCpuFreq + " MHz";
        //disable everything
      }
      else if (pMode == POWER_SUPERSAVE) {
        setCpuFrequencyMhz(2);
        msg = "STATUS - CPU - Power Super Saving - " + (String)minCpuFreq + " MHz";
      }
      debug(msg);
    #endif
    powerMode = pMode;
}

/**
 * Reset the ir tracker activity timer
 */
void resetIrTrackerActivityTimer() {
  irTrackerActivityTimer = millis();
  if (powerMode != POWER_ACTIVE) {
    setPowerMode(POWER_ACTIVE);
    setUpdateRate(sensorConfig.updateRate, false);
  }
}

/**
 * Reset the id sensor activity timer
 */
void resetIrComActivityTimer() {
  irComActivityTimer = millis();
  if (powerMode != POWER_ACTIVE) setPowerMode(POWER_ACTIVE);
}

/**
 * Reset the websocket activity timer
 */
void resetWebsocketActivityTimer() {
  websocketActivityTimer = millis();
  if (powerMode != POWER_ACTIVE) setPowerMode(POWER_ACTIVE);
}

/**
 * Reset the webserver activity timer
 */
void resetServerActivityTimer() {
  serverActivityTimer = millis();
  if (powerMode != POWER_ACTIVE) setPowerMode(POWER_ACTIVE);
}

/**
 * Reset the serial activity timer
 */
void resetSerialActivityTimer() {
  serialActivityTimer = millis();
  if (powerMode != POWER_ACTIVE) setPowerMode(POWER_ACTIVE);
}
