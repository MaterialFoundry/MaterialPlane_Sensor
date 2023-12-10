#if defined(PRODUCTION_HW)

void startTest() {
  Serial0.println("Starting test initialization");

  pinMode(BATTERY_LED_RED_PIN, OUTPUT);
  pinMode(BATTERY_LED_GREEN_PIN, OUTPUT);
  pinMode(LM66200_POWER_STATE_PIN, INPUT_PULLUP);
  digitalWrite(BATTERY_LED_RED_PIN, HIGH);

  Serial0.println("Initialization done");
  
  while(1) {
    if (Serial0.available()) {
      String msg = Serial0.readStringUntil('\n');
      Serial0.println(msg);
      
      if (msg == "TEST") Serial0.print("OK\n");
      else if (msg == "GET LM66200 STATE") Serial0.print((String)digitalRead(LM66200_POWER_STATE_PIN) + '\n');
      else if (msg == "GREEN 1") digitalWrite(BATTERY_LED_GREEN_PIN, HIGH);
      else if (msg == "GREEN 0") digitalWrite(BATTERY_LED_GREEN_PIN, LOW);
      else if (msg == "RED 1") digitalWrite(BATTERY_LED_RED_PIN, HIGH);
      else if (msg == "RED 0") digitalWrite(BATTERY_LED_RED_PIN, LOW);
      else if (msg == "PAJ") testPaj();
      else if (msg == "GAUGE") testGauge();
      else if (msg == "GAUGE_ALERT") testGaugeAlert();
      else if (msg == "CHARGER_PG") Serial0.print((String)digitalRead(MCP73871_PG_PIN) + '\n');
      else if (msg == "CHARGER_STAT1") Serial0.print((String)digitalRead(MCP73871_STAT1_PIN) + '\n');
      else if (msg == "CHARGER_STAT2") Serial0.print((String)digitalRead(MCP73871_STAT2_PIN) + '\n');
      else if (msg == "RMT") testRMT();
    }
  }
}

void testRMT() {
  initializeRmt();
  unsigned long t = millis();
  while(1) {
    while(rmt.available()){
      rmt_rx_data_t received = rmt.read();
      Serial0.print((String)received.protocol + '\n');
      if (received.protocol == "MP") {
        Serial0.print("RMT OK\n");
        break;
      }
      if (millis() - t >= 2500) {
        Serial0.print("RMT NOK\n");
        break;
      }
    }
    if (millis() - t >= 2500) {
      Serial0.print("RMT NOK\n");
      break;
    }
  }
}

void testGauge() {
  initializeBatteryManagement(true);
  MAX17260.registerInterrupt(MAX17260_ALERT_PIN);
  Serial0.print((String)MAX17260.getAverageCurrent() + '\n');
}

void testGaugeAlert() {
  MAX17260.setCurrentThreshold(-1, 1);
  unsigned long t = millis();
  while(1) {
    if (MAX17260.getInterruptState()) {
      Serial0.print("ALERT OK\n");
      break;
    }
    if (millis() - t >= 2500) {
      Serial0.print("ALERT NOK\n");
      break;
    }
  }
  MAX17260.setCurrentThreshold(0, 0);
}

void testPaj() {
  pinMode(PAJ_PWR_PIN, OUTPUT);
  digitalWrite(PAJ_PWR_PIN, HIGH);
  IRsensor.registerInterrupt();

  bool irSensorConnected = IRsensor.begin();
    if (!irSensorConnected) {
      Serial0.print("PAJ_CON NOK\n");
    }
    else {
      Serial0.print("PAJ_CON OK\n");
      IRsensor.setDebugMode(0x06);
      unsigned long timeoutTimer = millis();
      while (1) {
        if (IRsensor.getInterruptState()) {
          Serial0.print("PAJ_REC OK\n");
          break;
        }
        if (millis() - timeoutTimer >= 2500) {
          Serial0.print("PAJ_REC NOK\n");
          break;
        }
      }
    }
}

#endif
