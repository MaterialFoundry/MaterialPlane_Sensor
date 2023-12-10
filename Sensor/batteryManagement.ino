/**
 * Handles battery management. This includes checking the charger and fuel gauge for faults and battery status (for production sensor) or calculating the battery level (for full diy sensors)
 */

unsigned long printTimer = 0;
unsigned long maxTimer = 0;
uint16_t MAX17260alertFlags = 0;
uint8_t socOld = 255;
uint8_t socCurrent;
bool checkMCP = false;
unsigned long checkMCPtimer = 0;
SOC_TH socState = SOC_NORMAL;
bool powerSource = 0;

#ifdef TINYPICO_BATTERY_MONITOR
  float tpBatStorage = 0;
  uint8_t chargeCounter = 0;
  uint8_t chargeSum = 0;
#endif

/* prototype function */
void initializeBatteryManagement(bool forceReset = false);

/**
 * Clear battery preferences and reset the battery config
 */
void clearBatteryPreferences() {
  debug("STATUS - BATT - Resetting battery settings");
  Serial.println("Resetting battery settings");
  preferences.begin("battery", false);
  preferences.putUShort("fullCap", BAT_CAP);
  preferences.putUShort("fullCapNom", BAT_CAP);
  preferences.putUShort("cycles", 0);
  preferences.putUShort("rComp0", BAT_RCOMP_0);
  preferences.putUShort("tempCo", BAT_TEMP_CO);
  preferences.end();
  if (started) initializeBatteryManagement(true);
}

/********************************* Production sensor ***************************************/

/**
 * Print battery status
 */
void printBatteryStatus() {
  #ifdef PRODUCTION_BATTERY_MONITOR
      Serial.printf("Charging State:\t\t%s\nPercentage:\t\t%d%%\n", MCP73871.getStatusString(), (uint8_t)MAX17260.getPercentage());
      if (MCP73871.getStatus() == STAT_CHARGING) Serial.printf("Time to Full:\t\t%s\n",secondsToTime(MAX17260.getTimeToFull()));
      else if (MCP73871.getStatus() == STAT_SHUTDOWN) Serial.printf("Time to Empty:\t\t%s\n",secondsToTime(MAX17260.getTimeToEmpty()));
      Serial.printf("Voltage:\t\t%.3f V\nCurrent:\t\t%d mA\nCapacity:\t\t%d/%d mAh\n\n", ((uint16_t)MAX17260.getAverageVoltage())*0.001, (int16_t)MAX17260.getCurrent(), (uint16_t)MAX17260.getCapacity(), (uint16_t)MAX17260.getFullCapacity());
  #endif
}

/**
 * Initialize battery management
 */
void initializeBatteryManagement(bool forceReset) {
  Serial.printf("------------------------------------\nInitializing battery management\n\n");
  #ifdef PRODUCTION_BATTERY_MONITOR
    /* Set pinmode to detect the input power source */
    pinMode(LM66200_POWER_STATE_PIN, INPUT_PULLUP);
    
    /* Start MCP73871 battery charger */
    MCP73871.begin();
  
    /* Start MAX17260 battery fuel gauge */
    MAX17260.begin(); 
  
    /* Configure fuel gauge */
    MAX17260.setSenseResistor(BAT_R_SENSE);    //Configure the current sense resistor in mOhm (uint16_t) (default: 20)
  
    /*
     * Check the power-on reset bit. 
     * If this return true it means that the fuel gauge has been reset and a new configuration must be loaded and everything must be reconfigured.
     * If only the microcontroller has been reset, reconfiguring the fuel gauge is not necessary (the fuel gauge is probably always connected to the battery, and should therefore only reset if the battery is completely empty).
     * For optimal performance the fuel gauge should not be reconfigured unless necessary, because performance improves as it learns how the battery reacts. 
     * MAX17260.begin() and MAX17260.setSenseResistor() must be called before this
     */
    if (forceReset || MAX17260.getPOR()) {
      Serial.printf("Loading fuel gauge model config\n");
      setModelConfig();
    }

    powerSource = digitalRead(LM66200_POWER_STATE_PIN);
    
    /* Set the max charging current */
    MCP73871.setCurrentMode(CURR_MAX);
    //if (powerSource) MCP73871.setCurrentMode(CURR_MAX);
    //else MCP73871.setCurrentMode(CURR_500MA);
      
    /* Set the max charging current */
    //MCP73871.setCurrentMode(CURR_500MA);
  
    /* Store the state of charge for future calculations */
    socOld = MAX17260.getPercentageInt();
  
    /* Register interrupt system for fuel gauge to detect pulses on the alert pin */
    MAX17260.registerInterrupt(MAX17260_ALERT_PIN);    
  
    /* Register interrupt system for battery charger to detect changes on status pins */
    MCP73871.registerInterrupts();
  
    /* Check current fuel gauge alerts */
    checkAlerts();
    
  #endif

  #ifdef TINYPICO_BATTERY_MONITOR
    tpAverageVoltage.setNrOfReadings(100);
  #endif
  
  /* Print status */
  printBatteryStatus();
}
  
/**
 * Main battery management loop
 */
void batteryManagementLoop() {
  #ifdef PRODUCTION_BATTERY_MONITOR

    if (powerSource != digitalRead(LM66200_POWER_STATE_PIN)) {
      powerSource = digitalRead(LM66200_POWER_STATE_PIN);
       /* Set the max charging current */
      //if (powerSource) MCP73871.setCurrentMode(CURR_MAX);
      //else MCP73871.setCurrentMode(CURR_500MA);
      String pSource = powerSource ? "Dock" : "USB";
      debug("STATUS - POWER - Source changed " + pSource);
      statusTimer = millis() - STATUS_PERIOD; //Then transmit status
    }
    /* If fuel gauge interrupt occurs check alerts then transmit status */
    if (MAX17260.getInterruptState()) {
      checkAlerts();    //If an interrupt has occured, check alerts
      debug("STATUS - FUEL - Int Trig " + intToBinString(MAX17260alertFlags,11));
      statusTimer = millis() - STATUS_PERIOD; //Then transmit status
    }
  
    /* If charger interrupt occurs wait 100ms then transmit status */
    uint8_t McpInt = MCP73871.getInterruptState();
    if (McpInt) {
      checkMCP = true;
      debug("STATUS - CHARG - Int trig " + intToBinString(MCP73871.getStatusRaw(),4));
      checkMCPtimer = millis();
    }
    if (checkMCP && millis() - checkMCPtimer >= 100) {
      checkMCP = false;
      statusTimer = millis() - STATUS_PERIOD;
    }
  
    /* Check for fuel gauge alerts every 30 seconds */
    if (millis() - maxTimer >= 30000) {
      maxTimer = millis();
      checkAlerts();                                  //Check alerts periodically in case an interrupt has been missed
      if (MAX17260.getPOR()) setModelConfig();        //Check POR occasionally in case the fuel gauge has been reset
    }
  #endif

  #ifdef TINYPICO_BATTERY_MONITOR
    tpVoltage = tpAverageVoltage.getAverage(tp.GetBatteryVoltage()*1000);
    tpBatteryPercentage = getBatteryPercentage(tpVoltage);
    tpUsbActive = analogRead(USB_ACTIVE_PIN)>1000 ? true : false;
    if (tpUsbActive) {
      chargeSum += (int)tp.IsChargingBattery();
      chargeCounter++;
      if (chargeCounter >= 100) {
        chargingStatus = "Error";
        tpChargeSum = chargeSum;
        if (chargeSum < 100) { //full battery
          chargingStatus = "Charged";
        }
        else {                  //charging
          chargingStatus = "Charging";
        }
        //Serial.println("ChargeSum: " + (String)chargeSum);
        chargeSum = 0;
        chargeCounter = 0;
      }
    }
    else {
      chargingStatus = "USB Not Connected";
    }

    if (tpUsbActive != tpUsbActiveOld) {
      if (tpUsbActive) debug("STATUS - USB - Plugged In");
      else debug("STATUS - USB - Unplugged");
      tpUsbActiveOld = tpUsbActive;
      statusTimer = millis() - STATUS_PERIOD;
    }
  #endif
}

#ifdef TINYPICO_BATTERY_MONITOR
  /**
   * Get a very rough estimate of the battery voltage.
   */
  uint8_t getBatteryPercentage(float v) {
    if (v > 4.2) v = 4.20;
    int8_t percentage = 0;
    if (v < 3.25) percentage =  round(80*(v-3.00));
    else if (v >= 3.25 && v < 3.75) percentage =  round(20 + 120*(v-3.25));
    else if (v >= 3.75 && v < 4.00) percentage =  round(80 + 60*(v-3.75));
    else if (v >= 4.00) percentage =  round(95 + 50*(v-4.00));
    if (percentage < 0) percentage = 0;
    else if (percentage > 100) percentage = 100;
    return percentage;
  }
#endif

#ifdef PRODUCTION_BATTERY_MONITOR
  /**
   * Configure the parameters and load them into the fuel gauge
   */
  void setModelConfig() {
    if (serialConnected && settings.debug) Serial.println("Configuring MAX17260 model config");
    preferences.begin("battery", true);
    
    /* Configure the design capacity of the battery in mAh (uint16_t) (default: 1200mAh) */
    MAX17260.setDesignCap(BAT_CAP);
  
    /* Configure the charge termination current in mA (uint16_t) (default: 150mAh) */
    MAX17260.setChargeTerminationCurrent(BAT_TERM_CURR);
  
    /* Configure the voltage at which the IC considers the battery empty (in mV). Rounded to 10mV (uint16_t) (default: 3100mV) */
    MAX17260.setEmptyVoltage(BAT_EMPTY_VOLTAGE);
  
    /* Configure the voltage at which the IC considers the battery to not be empty any more (in mV). Rounded 40mV (uint16_t) (default: 3500mV) */
    MAX17260.setEmptyRecoveryVoltage(BAT_EMPTY_RECOV_VOLTAGE);
  
    /* Configure the percentage threshold at which the IC should consider the battery full in percent (default: 95%) */
    MAX17260.setFullSOCThreshold(BAT_FULL_SOC_THR);
    
    /*
     * Configure the battery model:
     * MODEL_LICOO - lithium cobalt oxide variants (most lithium batteries) (default)
     * MODEL_NCRNCA - lithium NCR or NCA battery
     * MODEL_LIFEPO4 - lithium iron phosphate battery
     */
    MAX17260.setBatteryModel(MODEL_LICOO);
  
    /*
     * Configure NTC type:
     * NTC_10K: 10kOhm (default)
     * NTC_100K: 100kOhm
     */
    MAX17260.setNtcType(NTC_10K);
  
    /*
     * Configure threshold values to trigger the alert system. Disable any threshold by writing 0 to both arguments.
     * setVoltageThreshold - Set the minimum (1st argument) and maximum (2nd argument) voltage in mV (both uint16_t)
     * setCurrentThreshold - Set the minimum (1st argument) and maximum (2nd argument) current in mA (both uint16_t)
     * setTemperatureThreshold - Set the minimum (1st argument) and maximum (2nd argument) temperature in degrees C (both uint8_t)
     * setSocThreshold - Set the minimum (1st argument) and maximum (2nd argument) state of charge in percent (both uint8_t)
     */
    MAX17260.setVoltageThreshold(3100, 4250);
    MAX17260.setCurrentThreshold(0, 0);
    MAX17260.setTemperatureThreshold(0, 0);
    MAX17260.setSocThreshold(5, 95);
  
    /*
     * Configure alert system. Enable or disable the following situations to trigger the alert pin (used by interrupt system)
     * setStickyAlerts - Threshold alerts only. If true, the alerts stay on until reset by software. If false, the alerts clear when no longer valid
     * setThresholdAlert - Enables alerts when temperature, voltage, current or state of charge is outside of the configured thresholds
     * setBatteryInsertAlert - Enables alerts when battery is inserted
     * setBatteryRemoveAlert - Enables alerts when battery is removed
     * setSocAlert - Enables alerts when the state of charge changes by a full percentage
     * setTemperatureAlert - Enables temperature based alerts
     */
    MAX17260.setStickyAlerts(false);
    MAX17260.setThresholdAlert(false);
    MAX17260.setBatteryInsertAlert(false);
    MAX17260.setBatteryRemoveAlert(false);
    MAX17260.setSocAlert(true);
    MAX17260.setTemperatureAlert(false);
    
    /* set registers from preferences to set the battery configuration */
    MAX17260.setRComp0(preferences.getUShort("rComp0", 0x004d));
    MAX17260.setTempCo(preferences.getUShort("tempCo", 0x223e));
    MAX17260.setFullCapacity(preferences.getUShort("fullCap", BAT_CAP));
    MAX17260.setCycles(preferences.getUShort("cycles", 0));
    MAX17260.setFullCapNom(preferences.getUShort("fullCapNom", BAT_CAP));
  
    /* Initialize the model using the configured parameters. If it returns errors, initialization has failed */
    MAX17260_ERR maxError = MAX17260.initializeModel();
    if (serialConnected && settings.debug) {
      if (maxError == ERR_DNR) debug("ERR - FUEL - Not Ready");
      else if (maxError == ERR_MODEL) debug("ERR - FUEL - Failed Model Load");
    }
    preferences.end();
  }
  
  /**
   * Check fuel gauge alerts
   */
  void checkAlerts() {
    MAX17260alertFlags = MAX17260.getAlertFlags();
    if (MAX17260alertFlags == 0) return;    //No alert flags
  
    socState = SOC_NORMAL;
    debug("STATUS - FUEL - Alert Flags " + (String)MAX17260alertFlags);
    if (serialConnected && settings.debug) {
      Serial.println("----------------------");
      Serial.println("Alerts: ");
      if (MAX17260alertFlags & 1) Serial.println("Current below threshold");
      if (MAX17260alertFlags>>1 & 1) Serial.println("Current above threshold");
      if (MAX17260alertFlags>>2 & 1) Serial.println("Voltage below threshold");
      if (MAX17260alertFlags>>3 & 1) Serial.println("Voltage above threshold");
      if (MAX17260alertFlags>>4 & 1) Serial.println("Temperature below threshold");
      if (MAX17260alertFlags>>5 & 1) Serial.println("Temperature above threshold");
    }
    
    if (MAX17260alertFlags>>6 & 1) {
      debug("STATUS - FUEL - SoC Below Thr");
      socState = SOC_BELOW;
    }
    if (MAX17260alertFlags>>7 & 1) {
      debug("STATUS - FUEL - SoC Above Thr");
      socState = SOC_ABOVE;
    }
    if (MAX17260alertFlags>>8 & 1) {
      socCurrent = MAX17260.getPercentageInt();
      /* if SoC has changed enough (40%), save registers to preferences so they can be restored when the fuel gauge resets */
      if (abs(socCurrent - socOld) >= 40) {
        socOld = socCurrent;
        /* save registers to preferences */
        preferences.begin("battery", false);
        preferences.putUShort("fullCap", MAX17260.getFullCapacity());
        preferences.putUShort("fullCapNom", MAX17260.getFullCapNom());
        preferences.putUShort("cycles", MAX17260.getCycles());
        preferences.putUShort("rComp0", MAX17260.getRComp0());
        preferences.putUShort("tempCo", MAX17260.getTempCo());
        preferences.end();
      }
      debug("STATUS - FUEL - SoC Changed " + (String)socCurrent + "% " + (String)MAX17260.getPercentagePerHour() + "%/h");
    }
    if (serialConnected && settings.debug) {
      if (MAX17260alertFlags>>9 & 1) Serial.println("Battery inserted");
      if (MAX17260alertFlags>>10 & 1) Serial.println("Battery removed");
      Serial.println("----------------------");
    }
    
    /* Clear the alert flags so the interrupt can trigger again. For threshold triggers this can result in continual triggering. Either solve the issue (e.g. reduce the current) or change the threshold values */
    MAX17260.clearAlertFlags();
  }
/********************************* DIY Basic sensor ***************************************/
#endif
