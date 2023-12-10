/**
 * Handles the IR tracker, including getting IR point data and calibration
 */

#include "./src/Homography/homography.h"

homography cal;
homography offsetCal;

unsigned long timeoutTimer = 0;
bool initInProgress = false;

CalibrationModes calMode = CAL_MODE_SINGLE;
CalibrationStatus calStatus = CAL_INACTIVE;
AutoExposureStatus autoExpStatus = AUTOEXP_INACTIVE;
float cal_currentCoordinates[4][2], cal_storedCoordinates[4][2], offset_storedCoordinates[4][2], offsetPoints[4][2]; 
uint8_t cal_count, avg_old, brightnessOld, minBrightnessOld, autoExposeBrightness, autoExposeMinBrightness = 0;                          
bool calRunning, mirrorX_old, mirrorY_old, rotation_old, calibration_old, calibrationOffset_old, calSuccess, autoExpRunning, autoExpSuccess = false;
int8_t offsetX_old, offsetY_old, calSource = 0;
float scaleX_old, scaleY_old = 1;
String calModeStr = "";
uint64_t autoExpTimer;
uint8_t autoExpCounter;
uint8_t autoExpMaxCounter;

/**
 * Clear all ir tracker preferences
 */
void clearIrPreferences() {
  debug("STATUS - IR - Resetting IR settings");
  Serial.println("Resetting IR settings");
  preferences.begin("irTracker", false);
  preferences.putBool("calEnable", CAL_EN);
  preferences.putBool("calOffsetEnable", CALOFFS_EN);
  preferences.putBool("mirrorX", MIRX);
  preferences.putBool("mirrorY", MIRY);
  preferences.putBool("rotation", ROT);
  preferences.putUShort("offsetX", OFFSETX);
  preferences.putUShort("offsetY", OFFSETY);
  preferences.putFloat("scaleX", SCALEX);
  preferences.putFloat("scaleY", SCALEY);
  preferences.putUChar("average", AVG);
  preferences.putUChar("minBrightness", MINBRIGHT);
  preferences.putUChar("updateRate", UPDATE);
  preferences.putUChar("brightness", BRIGHT);
  preferences.putFloat("calX_0", 4095);
  preferences.putFloat("calX_1", 4095);
  preferences.putFloat("calX_2", 0);
  preferences.putFloat("calX_3", 0);
  preferences.putFloat("calY_0", 4095);
  preferences.putFloat("calY_1", 0);
  preferences.putFloat("calY_2", 0);
  preferences.putFloat("calY_3", 4095);
  preferences.putFloat("offsetX_0", 4095);
  preferences.putFloat("offsetX_1", 4095);
  preferences.putFloat("offsetX_2", 0);
  preferences.putFloat("offsetX_3", 0);
  preferences.putFloat("offsetY_0", 4095);
  preferences.putFloat("offsetY_1", 0);
  preferences.putFloat("offsetY_2", 0);
  preferences.putFloat("offsetY_3", 4095);
  preferences.end();
  initializeIrTracker();
}

/**
 * Print ir tracker status
 */
void printIRStatus() {
  Serial.printf("Update Rate:\t\t%d\nBrightness:\t\t%d\nMinimum Brightness:\t%d\nAverage Count:\t\t%d\n", sensorConfig.updateRate, sensorConfig.brightness, sensorConfig.minBrightness, sensorConfig.average);
  Serial.printf("Mirror X:\t\t%s\nMirror Y:\t\t%s\nRotation:\t\t%s\n", sensorConfig.mirrorX ? "Enabled" : "Disabled", sensorConfig.mirrorY ? "Enabled" : "Disabled", sensorConfig.rotation ? "Enabled" : "Disabled");
  Serial.printf("Offset X:\t\t%d\nOffset Y:\t\t%d\nScale X:\t\t%.3f\nScale Y:\t\t%.3f\n", sensorConfig.offsetX, sensorConfig.offsetY, sensorConfig.scaleX, sensorConfig.scaleY);
  Serial.printf("Calibration:\t\t%s\nCalibration Offset:\t%s\n\n", sensorConfig.calibrationEnable ? "Enabled" : "Disabled", sensorConfig.calibrationOffsetEnable ? "Enabled" : "Disabled");
}

/**
 * Initialize ir tracker
 */
void initializeIrTracker() {
  initInProgress = true;

  /* Only run if sensor is not fully initialized (so the first time this function is called) */
  if (!started) {
    Serial.printf("------------------------------------\nInitializing IR sensor\n\n");

    #ifdef PRODUCTION_HW
      /* Set pin mode of power pin and set it high (to apply power) */
      pinMode(PAJ_PWR_PIN, OUTPUT);
      digitalWrite(PAJ_PWR_PIN, HIGH);
    #endif

    /* Load all ir tracker settings */
    preferences.begin("irTracker", false);
    Serial.println("test: " + (String)preferences.getBool("calEnable"));
    sensorConfig.calibrationEnable = preferences.getBool("calEnable", sensorConfig.calibrationEnable);
    sensorConfig.calibrationOffsetEnable = preferences.getBool("calOffsetEnable", sensorConfig.calibrationOffsetEnable);
    sensorConfig.mirrorX = preferences.getBool("mirrorX", sensorConfig.mirrorX);
    sensorConfig.mirrorY = preferences.getBool("mirrorY", sensorConfig.mirrorY);
    sensorConfig.rotation = preferences.getBool("rotation", sensorConfig.rotation);
    sensorConfig.offsetX = preferences.getUShort("offsetX", sensorConfig.offsetX);
    sensorConfig.offsetY = preferences.getUShort("offsetY", sensorConfig.offsetY);
    sensorConfig.scaleX = preferences.getFloat("scaleX", sensorConfig.scaleX);
    sensorConfig.scaleY = preferences.getFloat("scaleY", sensorConfig.scaleY);
    sensorConfig.average = preferences.getUChar("average", sensorConfig.average);
    sensorConfig.minBrightness = preferences.getUChar("minBrightness", sensorConfig.minBrightness);
    sensorConfig.updateRate = preferences.getUChar("updateRate", sensorConfig.updateRate);
    sensorConfig.brightness = preferences.getUChar("brightness", sensorConfig.brightness);
    cal_storedCoordinates[0][0] = preferences.getFloat("calX_0", 4095);
    cal_storedCoordinates[1][0] = preferences.getFloat("calX_1", 4095);
    cal_storedCoordinates[2][0] = preferences.getFloat("calX_2", 0);
    cal_storedCoordinates[3][0] = preferences.getFloat("calX_3", 0);
    cal_storedCoordinates[0][1] = preferences.getFloat("calY_0", 4095);
    cal_storedCoordinates[1][1] = preferences.getFloat("calY_1", 0);
    cal_storedCoordinates[2][1] = preferences.getFloat("calY_2", 0);
    cal_storedCoordinates[3][1] = preferences.getFloat("calY_3", 4095);

    offsetPoints[0][0] = preferences.getFloat("offsetX_0", 4095);
    offsetPoints[1][0] = preferences.getFloat("offsetX_1", 4095);
    offsetPoints[2][0] = preferences.getFloat("offsetX_2", 0);
    offsetPoints[3][0] = preferences.getFloat("offsetX_3", 0);
    offsetPoints[0][1] = preferences.getFloat("offsetY_0", 4095);
    offsetPoints[1][1] = preferences.getFloat("offsetY_1", 0);
    offsetPoints[2][1] = preferences.getFloat("offsetY_2", 0);
    offsetPoints[3][1] = preferences.getFloat("offsetY_3", 4095);
    preferences.end();

    setCalibration(cal_storedCoordinates, offsetPoints, sensorConfig.calibrationOffsetEnable);

    /* Configure all ir points */
    for (int i=0; i<OBJECT_NUM; i++) {
      irPoints[i].number = i;
      irPoints[i].setAverageCount(sensorConfig.average);
      irPoints[i].setMirrorX(sensorConfig.mirrorX);
      irPoints[i].setMirrorY(sensorConfig.mirrorY);
      irPoints[i].setRotation(sensorConfig.rotation);
      irPoints[i].setOffset(sensorConfig.offsetX, sensorConfig.offsetY);
      irPoints[i].setScale(sensorConfig.scaleX, sensorConfig.scaleY);
      irPoints[i].setCalibration(sensorConfig.calibrationEnable);
      irPoints[i].setOffsetCalibration(sensorConfig.calibrationOffsetEnable);
      irPoints[i].setCalObjects(cal, offsetCal);
    }
  }
  
  /* Begin IR tracker object */
  bool irSensorConnected = IRsensor.begin();
  #ifdef PAJ_SENSOR
    if (!irSensorConnected) {
      Serial.printf("ERROR: Could not connect to IR sensor\n");
      debug("ERR - PAJ - No connect");
      return;
    }
    debug("STATUS - PAJ - Connect");
  #endif

  //IRsensor.setCalibrationEnable(false);
  //IRsensor.setCalibrationOffsetEnable(false);

  /* Configure ir tracker */
  setIrSensorBrightness(sensorConfig.brightness);
  IRsensor.setPixelBrightnessThreshold(sensorConfig.minBrightness);
  IRsensor.setFramePeriod(framePeriodLUT[sensorConfig.updateRate]);
  #ifdef PAJ_SENSOR
    IRsensor.setPixelNoiseTreshold(NOISE);
    IRsensor.setMinAreaThreshold(MIN_AREA);
    IRsensor.setMaxAreaThreshold(MAX_AREA);
    IRsensor.setObjectNumberSetting(OBJECT_NUM);
  #endif

  if (!started) {
    #ifdef PAJ_SENSOR
      /* Register interrupt to detect when frame is finished processing */
      IRsensor.registerInterrupt();
    #endif

    /* Create ir tracker task */
    xTaskCreatePinnedToCore(
      irSensorLoop,
      "irSensor",
      IR_SENSOR_STACK_SIZE,
      NULL,
      2,
      &irSensorTask,
      0
    );

    /* Print ir tracker status */
    printIRStatus();
  }

  /* Reset timeout timer */
  timeoutTimer = millis();
  
  initInProgress = false;
}

/**
 * Main ir tracker loop called by irSensorTask
 */
void irSensorLoop(void * parameter) {
  delay(1000);
  while(1) {
    
    /* Check if calibration procedure has to be done and do it if required */
    getCal();
    
    /* Check if auto exposure procedure has to be done and do it if required */
    getAutoExposure();

    /* If PAJ sensor has made a measurement, process it */
    if (IRsensor.getInterruptState()) {
      /* Reset timeout timer */
      timeoutTimer = millis();

      /* If ir tracker is not being initialized */
      if (!initInProgress) {
        /* Get output of sensor. If points are detected, reset the activity timer */
        if (IRsensor.getOutput(irPoints)) resetIrTrackerActivityTimer();
      }
    }

    #ifdef PAJ_SENSOR
      /* Check periodically if PAJ sensor is still working, otherwise reset it */
      if (millis() - timeoutTimer >= 500) {
        timeoutTimer = millis();
        debug("ERR - PAJ - Connection lost");
        Serial.println("ERR - PAJ - Connection lost");
        initializeIrTracker();
      }
    #endif
    delay(10);
  }
}

/**
 * Sets update rate of the sensor
 */
void setUpdateRate(uint8_t val, bool save) {
  /* Set ir tracker frame period based on lookup table */
  IRsensor.setFramePeriod(framePeriodLUT[val]);
  debug("SETT - URATE - " + (String)val);
  
  /* Store value to preferences, if necessary */
  if (!save) return;
  sensorConfig.updateRate = val;
  preferences.begin("irTracker", false);
  preferences.putUChar("updateRate", val);
  preferences.end();
}

/**
 * Sets exposure and gain of ir sensor
 */
void setIrSensorBrightness(uint8_t val) {
  #ifdef PAJ_SENSOR
    /* Calculate exposure and gain settings from brightness value */
    float exposure, gain;
    if (val < 77) {
      exposure = 0.02*pow(1.089, val-1);
      gain = 1;
    }
    else {
      exposure = 12;
      gain = 0.001667*pow(1.089, val-1);
    }
  
    /* Update exposure and gain of ir tracker */
    IRsensor.setExposureTime(exposure);
    IRsensor.setGain(gain);
  #endif
  #ifdef WIIMOTE_SENSOR
    IRsensor.setSensitivity(val);
  #endif
}

/*
 * Sets the brightness
 */
void setBrightness(uint8_t val, bool save) {
  sensorConfig.brightness = constrain(val, 1, 100);
  setIrSensorBrightness(val);
  debug("SETT - BRIGHT - " + (String)sensorConfig.brightness);

  /* Store value to preferences, if necessary */
  if (!save) return;
  preferences.begin("irTracker", false);
  preferences.putUChar("brightness", val);
  preferences.end();
}

/**
 * Sets the minimum brightness
 */
void setMinBrightness(uint8_t val, bool save) {
  sensorConfig.minBrightness = constrain(val, 10, 255);
  IRsensor.setPixelBrightnessThreshold(sensorConfig.minBrightness);
  debug("SETT - MBRIGHT - " + (String)val);

  /* Store value to preferences, if necessary */
  if (!save) return;
  preferences.begin("irTracker", false);
  preferences.putUChar("minBrightness", sensorConfig.minBrightness);
  preferences.end();
}

/**
 * Sets the averaging value
 */
void setAverage(uint8_t val, bool save) {
  sensorConfig.average = val;
  for (int i=0; i<OBJECT_NUM; i++)
    irPoints[i].setAverageCount(val);
  debug("SETT - AVG - " + (String)val);

  /* Store value to preferences, if necessary */
  if (!save) return;
  preferences.begin("irTracker", false);
  preferences.putUChar("average", val);
  preferences.end();
}

void setMirrorX(bool val, bool save) {
  sensorConfig.mirrorX = val;
  for (int i=0; i<OBJECT_NUM; i++)
    irPoints[i].setMirrorX(val);
  debug("SETT - MIRX - " + (String)val);

  /* Store value to preferences, if necessary */
  if (!save) return;
  preferences.begin("irTracker", false);
  preferences.putBool("mirrorX", val);
  preferences.end();
}

void setMirrorY(bool val, bool save) {
  sensorConfig.mirrorY = val;
  for (int i=0; i<OBJECT_NUM; i++)
    irPoints[i].setMirrorY(val);
  debug("SETT - MIRY - " + (String)val);

  /* Store value to preferences, if necessary */
  if (!save) return;
  preferences.begin("irTracker", false);
  preferences.putBool("mirrorY", val);
  preferences.end();
}

void setRotation(bool val, bool save) {
  sensorConfig.rotation = val;
  for (int i=0; i<OBJECT_NUM; i++)
    irPoints[i].setRotation(val);
  debug("SETT - ROT - " + (String)val);

  /* Store value to preferences, if necessary */
  if (!save) return;
  preferences.begin("irTracker", false);
  preferences.putBool("rotation", val);
  preferences.end();
}

void setOffsetX(int16_t val, bool save) {
  sensorConfig.offsetX = val;
  for (int i=0; i<OBJECT_NUM; i++)
    irPoints[i].setOffsetX(val);
  debug("SETT - OFFX - " + (String)val);

  /* Store value to preferences, if necessary */
  if (!save) return;
  preferences.begin("irTracker", false);
  preferences.putUShort("offsetX", val);
  preferences.end();
}

void setOffsetY(int16_t val, bool save) {
  sensorConfig.offsetY = val;
  for (int i=0; i<OBJECT_NUM; i++)
    irPoints[i].setOffsetY(val);
  debug("SETT - OFFY - " + (String)val);

  /* Store value to preferences, if necessary */
  if (!save) return;
  preferences.begin("irTracker", false);
  preferences.putUShort("offsetY", val);
  preferences.end();
}

void setScaleX(float val, bool save) {
  sensorConfig.scaleX = val;
  for (int i=0; i<OBJECT_NUM; i++)
    irPoints[i].setScaleX(val);
  debug("SETT - SCLX - " + (String)val);

  /* Store value to preferences, if necessary */
  if (!save) return;
  preferences.begin("irTracker", false);
  preferences.putFloat("scaleX", val);
  preferences.end();
}

void setScaleY(float val, bool save) {
  sensorConfig.scaleY = val;
  for (int i=0; i<OBJECT_NUM; i++)
    irPoints[i].setScaleY(val);
  debug("SETT - SCLY - " + (String)val);

  /* Store value to preferences, if necessary */
  if (!save) return;
  preferences.begin("irTracker", false);
  preferences.putFloat("scaleY", val);
  preferences.end();
}

void setCalibration(bool val, bool save) {
  sensorConfig.calibrationEnable = val;
  for (int i=0; i<OBJECT_NUM; i++)
    irPoints[i].setCalibration(val);
  debug("SETT - CALEN - " + (String)val);

  /* Store value to preferences, if necessary */
  if (!save) return;
  preferences.begin("irTracker", false);
  preferences.putBool("calEnable", val);
  preferences.end();
}

void setOffsetCalibration(bool val, bool save) {
  sensorConfig.calibrationOffsetEnable = val;
  setCalibration(cal_storedCoordinates, offsetPoints, sensorConfig.calibrationOffsetEnable);
  //for (int i=0; i<OBJECT_NUM; i++)
    //irPoints[i].setOffsetCalibration(val);
  debug("SETT - CALOEN - " + (String)val);

  /* Store value to preferences, if necessary */
  if (!save) return;
  preferences.begin("irTracker", false);
  preferences.putBool("calOffsetEnable", val);
  preferences.end();
}

/**
 * Start auto exposure procedure
 */
void performAutoExposure() {
  Serial.println("Performing autoexposure");
  autoExpStatus = AUTOEXP_STARTING;
}

/**
 * Auto exposure procudure
 */
void getAutoExposure() {
  /* If inactive, do nothing */
  if (autoExpStatus == AUTOEXP_INACTIVE) return;

  /* Start autoexposure procedure */
  if (autoExpStatus == AUTOEXP_STARTING) {
    autoExpStatus = AUTOEXP_PREACTIVE;
    autoExpCounter = 0;
    autoExpMaxCounter = CAL_AVG*1.5;
    autoExpRunning = true;
    autoExpSuccess = false;
    autoExpTimer = millis();
    debug("AUTOEXP - STARTING");

    /* Store current settings and load temporary settings */
    avg_old = sensorConfig.average;
    setAverage(CAL_AVG, false);
    brightnessOld = sensorConfig.brightness;
    minBrightnessOld = sensorConfig.minBrightness;
    autoExposeBrightness = 1;
    autoExposeMinBrightness = 10;
    #ifdef PAJ_SENSOR
      autoExposeMinBrightness = 30;
    #endif
    setBrightness(autoExposeBrightness, false);
    setMinBrightness(autoExposeMinBrightness, false);
    
    broadcastWs("{\"status\":\"autoExposure\",\"state\":\"starting\"}");
  }

  /* Wait a bit for the averaging to reset */
  if (autoExpStatus == AUTOEXP_PREACTIVE) {
    autoExpCounter++;
    if (autoExpCounter >= autoExpMaxCounter) {
      autoExpStatus = AUTOEXP_ACTIVE;
    }
  }

  /* Do if auto exposure is active */
  if (autoExpStatus == AUTOEXP_ACTIVE) {
    /* Make sure there's a short delay between each run */
    autoExpStatus = AUTOEXP_PREACTIVE;
    autoExpCounter = 0;
    autoExpMaxCounter = 2;
    
    /* Gather IR data */
    uint8_t detectedPoints = 0;
    String brightness = "";
    uint8_t brightnessValues[OBJECT_NUM][2];
    for (int i=0; i<OBJECT_NUM; i++) {
      if (irPoints[i].valid) {
        brightnessValues[detectedPoints][0] = i;
        brightnessValues[detectedPoints][1] = irPoints[i].maxBrightness;
        detectedPoints++;
        brightness += (String)irPoints[i].maxBrightness + ' ';
      }
    }

    /* Order IR data based on their brightness */
    uint8_t orderedBrightnessValues[OBJECT_NUM][2];
    for (int i=0; i<detectedPoints; i++) {
      uint8_t brightestPoint;
      uint8_t brightestValue = 0;
      for (int j=0; j<detectedPoints; j++) {
        if (brightnessValues[j][1] > brightestValue) {
          brightestValue = brightnessValues[j][1];
          brightestPoint = j;
        }
      }
      brightnessValues[brightestPoint][1] = 0;
      orderedBrightnessValues[i][0] = brightestPoint;
      orderedBrightnessValues[i][1] = brightestValue;
    }

    /* If 1 point is detected but brightness too high, reduce brightness */
    if (detectedPoints == 1 && orderedBrightnessValues[0][1] > 225) {
      autoExposeBrightness--;
      if (autoExposeBrightness < 1) autoExposeBrightness = 1;
      setBrightness(autoExposeBrightness, false);
    }
    
    /* If 1 point is detected and brightness is between 200 and 225, auto exposure is done */
    else if (detectedPoints == 1 && orderedBrightnessValues[0][1] >= 200) {
      autoExpStatus = AUTOEXP_STOP;
      autoExposeMinBrightness += 0.5*abs(200-autoExposeMinBrightness);
    }

    /* If no points are detected or one point is detected but the brightness is too low, increase brightness */
    if (detectedPoints == 0 || (detectedPoints == 1 && orderedBrightnessValues[0][1] < 200)) {
      autoExposeBrightness++;
      /* If brightness is 100 it can no longer increase, so cancel auto exposure */
      if (autoExposeBrightness == 100) {
        #ifdef PAJ_SENSOR
          autoExpStatus = AUTOEXP_CANCEL;
        #endif
        #ifdef WIIMOTE_SENSOR
          autoExpStatus = AUTOEXP_STOP;
          autoExposeMinBrightness += 0.25*abs(orderedBrightnessValues[0][1]-autoExposeMinBrightness);
        #endif
      }
      setBrightness(autoExposeBrightness, false);
    }

    /* If too many points are detected, increase minimum brightness */
    else if (detectedPoints > 1) {
      autoExposeMinBrightness++;
      /* If minimum brightness is 255 it can no longer increase, so cancel auto exposure */
      if (autoExposeMinBrightness == 255) autoExpStatus = AUTOEXP_CANCEL;
      setMinBrightness(autoExposeMinBrightness, false);
    }

  /*
    Serial.print("Detected points: " + (String)detectedPoints + '\t');
    for (int i=0; i<detectedPoints; i++) {
      Serial.print((String)orderedBrightnessValues[i][0] + ": " + (String)orderedBrightnessValues[i][1] + '\t');
    }
    if (detectedPoints > 1)
      Serial.print("\tDiff: " + (String)(orderedBrightnessValues[0][1] - orderedBrightnessValues[1][1]) + '\t');

    Serial.println("\t\tBr: " + (String)autoExposeBrightness + "\tMin: " + (String)autoExposeMinBrightness);
    */
  }

  /* Do when auto exposure is successfully finished */
  if (autoExpStatus == AUTOEXP_STOP) {
    autoExpStatus = AUTOEXP_INACTIVE;

    /* Sets the new settings */
    setBrightness(autoExposeBrightness);
    setMinBrightness(autoExposeMinBrightness);
    setAverage(avg_old);
    
    debug("AUTOEXP - STOPPED - Brightness: " + (String)autoExposeBrightness + ", Min Brightness: " + (String)autoExposeMinBrightness);
    statusTimer = millis() - STATUS_PERIOD;
    autoExpRunning = false;
    autoExpSuccess = true;
    broadcastWs("{\"status\":\"autoExposure\",\"state\":\"done\"}");
    return;
  }

  /* Do if auto exposure is cancelled */
  if (autoExpStatus == AUTOEXP_CANCEL) {
    autoExpStatus = AUTOEXP_INACTIVE;

    /* Revert settings to their old values */
    setBrightness(brightnessOld);
    setMinBrightness(minBrightnessOld);
    setAverage(avg_old);
    
    autoExpRunning = false;
    autoExpSuccess = false;
    debug("AUTOEXP - CANCELLED");
    broadcastWs("{\"status\":\"autoExposure\",\"state\":\"cancelled\"}");
    return;
  }

  /* If auto exposure takes too long (20 seconds), cancel it */
  if (millis() - autoExpTimer >= 20000) {
    autoExpStatus = AUTOEXP_CANCEL;
    debug("AUTOEXP - TIMEOUT");
  }
}

/**
 * Start calibration procedure
 */
void performCalibration(String mode, uint8_t source) {
  //Serial.println("Starting calibration: " + mode);
  if (mode == "SinglePoint") calMode = CAL_MODE_SINGLE;
  else if (mode == "MultiPoint") calMode = CAL_MODE_MULTI;
  else if (mode == "Offset") calMode = CAL_MODE_OFFSET;
  else return;
  calStatus = CAL_STARTING;
  calSource = source;
}

/**
 * Calibration procedure
 */
void getCal() {
  /* If calibration is inactive, do nothing */
  if (calStatus == CAL_INACTIVE) return;

  /* Start calibration procedure */
  if (calStatus == CAL_STARTING) {
    calStatus = CAL_ACTIVE;
    calRunning = true;
    calSuccess = false;
    if (calMode == CAL_MODE_SINGLE ) calModeStr = "SinglePoint";
    else if (calMode == CAL_MODE_MULTI ) calModeStr = "MultiPoint";
    else if (calMode == CAL_MODE_OFFSET ) calModeStr = "Offset";
    else return;
    debug("CAL - START - " + calModeStr);

    /* Clear calibration values */
    cal_count = 0;
    for (int i = 0; i < 4; i++) {
      cal_currentCoordinates[i][0] = 0;
      cal_currentCoordinates[i][1] = 0;
      if (calMode == CAL_MODE_SINGLE) {
        cal_storedCoordinates[i][0] = 0;
        cal_storedCoordinates[i][1] = 0;
      }
      else if (calMode == CAL_MODE_OFFSET) {
        offset_storedCoordinates[i][0] = 0;
        offset_storedCoordinates[i][1] = 0;
      }
      
    }

    /* Store old values to recover after calibration is complete */
    mirrorX_old = sensorConfig.mirrorX;
    mirrorY_old = sensorConfig.mirrorY;
    rotation_old = sensorConfig.rotation;
    calibration_old = sensorConfig.calibrationEnable;
    calibrationOffset_old = sensorConfig.calibrationOffsetEnable;
    offsetX_old = sensorConfig.offsetX;
    offsetY_old = sensorConfig.offsetY;
    scaleX_old = sensorConfig.scaleX;
    scaleY_old = sensorConfig.scaleY;
    avg_old = sensorConfig.average;

    /* Configure sensor for calibration */
    setAverage(CAL_AVG, false);
    setMirrorX(false, false);
    setMirrorY(false, false);
    setRotation(false, false);
    setOffsetX(0, false);
    setOffsetY(0, false);
    setScaleX(1, false);
    setScaleY(1, false);
    setCalibration(false, false);
    setOffsetCalibration(false, false);

    broadcastWs("{\"status\":\"calibration\",\"mode\":\"" + calModeStr + "\",\"state\":\"starting\"}");
    debug("CAL - STARTING");
  }

  /* Gather ir data */
  else if (calStatus == CAL_ACTIVE) {
    String pointMsg = "{\"status\":\"calibration\",\"mode\":\"" + calModeStr + "\",\"state\":\"newPoint\",\"points\":[";
    uint8_t detectedPoints = 0;
    
    for (int i = 0; i < 4; i++) {
      /* If ir points are valid, store them in the calibration arrays */
      if (irPoints[i].valid && irPoints[i].x != -9999 && irPoints[i].y != -9999) {
        cal_currentCoordinates[i][0] = irPoints[i].x;
        cal_currentCoordinates[i][1] = irPoints[i].y;
        if (detectedPoints > 0) pointMsg += ',';
        pointMsg += "{\"point\":" + (String)detectedPoints + ",\"x\":" + (String)irPoints[i].x + ",\"y\":" + (String)irPoints[i].y + "}";
        detectedPoints++;
      }
      
    }

    if (calMode == CAL_MODE_MULTI) {
      pointMsg += "]}";
      broadcastWs(pointMsg);
    }
  }

  /* Move to next point */
  else if (calStatus == CAL_NEXT) {
    String msg = "";
    if (calMode != CAL_MODE_MULTI) {
      /* Set status to active to gather new points */
      calStatus = CAL_ACTIVE;
      
      /* Store the previously recorded points */
      if (calMode == CAL_MODE_SINGLE) {
        cal_storedCoordinates[cal_count][0] = cal_currentCoordinates[0][0];
        cal_storedCoordinates[cal_count][1] = cal_currentCoordinates[0][1];
      }
      else if (calMode == CAL_MODE_OFFSET) {
        offset_storedCoordinates[cal_count][0] = cal_currentCoordinates[0][0];
        offset_storedCoordinates[cal_count][1] = cal_currentCoordinates[0][1];
      }
      

      /* prepare data */
      msg = "{\"status\":\"calibration\",\"mode\":\"" + calModeStr + "\",\"state\":\"newPoint\",\"point\":" + (String)cal_count + ",\"x\":" + (String)cal_currentCoordinates[0][0] + ",\"y\":" + (String)cal_currentCoordinates[0][1] + "}";
      
      /* Increase cal_count by 1 */
      cal_count++;
    }
    else {
      /* Store the previously recorded points */
      for (int i=0; i<4; i++) {
        cal_storedCoordinates[i][0] = cal_currentCoordinates[i][0];
        cal_storedCoordinates[i][1] = cal_currentCoordinates[i][1];
      }
      cal_count = 4;
    }

    broadcastWs(msg);
    debug("CAL - NEXT - " + (String)cal_currentCoordinates[0][0] + ", " + (String)cal_currentCoordinates[0][1]);

    /* If 4 points have been found, all calibration values have been gathered */
    if (cal_count == 4) {
      broadcastWs("{\"status\":\"calibration\",\"mode\":\"" + calModeStr + "\",\"state\":\"done\"}");
      debug("CAL - DONE");
      calStatus = CAL_STOP;

      if (calMode != CAL_MODE_OFFSET) {
        /* Set the calibration values and calculate new homography matrix */

        for (int i = 0; i < 4; i++) {
          cal.setCalibrationPoint(i, 0, cal_storedCoordinates[i][0]);
          cal.setCalibrationPoint(i, 1, cal_storedCoordinates[i][1]);
          
        }
        
        cal.orderCalibrationArray();

        for (int i=0; i<4; i++) {
          cal_storedCoordinates[i][0] = cal.getCalibrationPoint(i, 0);
          cal_storedCoordinates[i][1] = cal.getCalibrationPoint(i, 1);
        }
        for (int i=0; i<OBJECT_NUM; i++) {
          irPoints[i].setCalObjects(cal, offsetCal);
        }

        setCalibration(cal_storedCoordinates, offsetPoints, sensorConfig.calibrationOffsetEnable);

        /* Store the calibration points */
        preferences.begin("irTracker", false);
        preferences.putFloat("calX_0", cal_storedCoordinates[0][0]);
        preferences.putFloat("calX_1", cal_storedCoordinates[1][0]);
        preferences.putFloat("calX_2", cal_storedCoordinates[2][0]);
        preferences.putFloat("calX_3", cal_storedCoordinates[3][0]);
        preferences.putFloat("calY_0", cal_storedCoordinates[0][1]);
        preferences.putFloat("calY_1", cal_storedCoordinates[1][1]);
        preferences.putFloat("calY_2", cal_storedCoordinates[2][1]);
        preferences.putFloat("calY_3", cal_storedCoordinates[3][1]);
        preferences.end();
        calSuccess = true;
      }
      else {
        /* Set the calibration values and calculate new homography matrix */
        for (int i = 0; i < 4; i++) {
          offsetCal.setCalibrationPoint(i, 0, offset_storedCoordinates[i][0]);
          offsetCal.setCalibrationPoint(i, 1, offset_storedCoordinates[i][1]);
        }
        offsetCal.orderCalibrationArray(false);
  
        for (int i=0; i<4; i++) {
          offsetPoints[i][0] = offsetCal.getCalibrationPoint(i, 0) - cal_storedCoordinates[i][0];
          offsetPoints[i][1] = offsetCal.getCalibrationPoint(i, 1) - cal_storedCoordinates[i][1];
        }

        setCalibration(cal_storedCoordinates, offsetPoints, sensorConfig.calibrationOffsetEnable);
        /*
        for (int i=0; i<OBJECT_NUM; i++) {
          irPoints[i].setCalObjects(cal, offsetCal);
        }
        */

        /* Store the calibration points */
        preferences.begin("irTracker", false);
        preferences.putFloat("offsetX_0", offsetPoints[0][0]);
        preferences.putFloat("offsetX_1", offsetPoints[1][0]);
        preferences.putFloat("offsetX_2", offsetPoints[2][0]);
        preferences.putFloat("offsetX_3", offsetPoints[3][0]);
        preferences.putFloat("offsetY_0", offsetPoints[0][1]);
        preferences.putFloat("offsetY_1", offsetPoints[1][1]);
        preferences.putFloat("offsetY_2", offsetPoints[2][1]);
        preferences.putFloat("offsetY_3", offsetPoints[3][1]);
        preferences.end();
        calSuccess = true;
      }
    }
  }

  /* Cancel calibration */
  else if (calStatus == CAL_CANCEL) {
    broadcastWs("{\"status\":\"calibration\",\"mode\":\"" + calModeStr + "\",\"state\":\"cancelled\"}");
    debug("CAL - CANCELLED");
    calStatus = CAL_STOP;
  }

  /* Stop calibration */
  if (calStatus == CAL_STOP) {
    calStatus = CAL_INACTIVE;

    /* Configure sensor to normal settings */
    setAverage(avg_old, false);
    setMirrorX(mirrorX_old, false);
    setMirrorY(mirrorY_old, false);
    setRotation(rotation_old, false);
    setOffsetX(offsetX_old, false);
    setOffsetY(offsetY_old, false);
    setScaleX(scaleX_old, false);
    setScaleY(scaleY_old, false);
    if (calSuccess && calMode == CAL_MODE_OFFSET) {
      setCalibration(true, true);
      setOffsetCalibration(true, true);
    }
    else if (calSuccess && calMode != CAL_MODE_OFFSET) {
      setCalibration(true, true);
      setOffsetCalibration(calibrationOffset_old, false);
    }
    else {
      setCalibration(calibration_old, false);
      setOffsetCalibration(calibrationOffset_old, false);
    }
    debug("CAL - STOPPED");
    statusTimer = millis() - STATUS_PERIOD;
  }
}

/* Go to next calibration point */
void nextCalibrationPoint() {
  if (calStatus != CAL_ACTIVE) return;
  calStatus = CAL_NEXT;
}

/* Cancel the calibratin procedure */
void cancelCalibration() {
  if (calStatus != CAL_ACTIVE) return;
  calStatus = CAL_CANCEL;
}

/* Cancel calibration when a disconnection is detected */
void cancelCalibrationOnDisconnect(uint8_t source) {
  if (calStatus != CAL_ACTIVE || source != calSource) return;
  debug("CAL - CANCELLED ON DISCONNECT");
}

void setCalibration(float calPoints[][2], float offsetPoints[][2], bool offsetEn) {

  if (offsetEn) {
    for (int i = 0; i < 4; i++) {
      cal.setCalibrationPoint(i, 0, calPoints[i][0] + offsetPoints[i][0]);
      cal.setCalibrationPoint(i, 1, calPoints[i][1] + offsetPoints[i][1]);
    }
  }
  else {
    for (int i = 0; i < 4; i++) {
      cal.setCalibrationPoint(i, 0, calPoints[i][0]);
      cal.setCalibrationPoint(i, 1, calPoints[i][1]);
    }
  }
  
  cal.calculateHomographyMatrix();
  for (int i=0; i<OBJECT_NUM; i++) {
    irPoints[i].setCalObjects(cal, offsetCal);
  }
}
