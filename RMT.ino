/**
 * Handles the detected of base data (ID, command and battery state) and IR remote controls
 */

/********************************* Production sensor ***************************************/
#ifdef ID_SENSOR
  #include "src/ESP_RMT/ESP_RMT.h"
  
  RMT_RX rmt(RMT_IN_PIN, 0, false);
  
  unsigned long rmtResetTimer = 0;
  uint32_t codeOld;


  /**
   * Initialize RMT by setting pinmodes, applying power to the sensor and starting the rmt object
   */
  void initializeRmt() {
    pinMode(RMT_PWR_PIN, OUTPUT);
    pinMode(RMT_IN_PIN, INPUT);
    digitalWrite(RMT_PWR_PIN, HIGH);
    rmt.start(); 
  }
  
  /**
   * Called in the main loop to periodically check if new data has been received
   */
  void rmtLoop() {
    /* Check if data has been received */
    while(rmt.available()){
      /* Get the received data */
      rmt_rx_data_t received = rmt.read();
      
      /* If the protocol is "MP", this means it's MP hardware (base or pen) */
      if (received.protocol == "MP") {
        
        const uint32_t code = received.code;
        if (code != 0) {
          /* Clear the reset timer */
          rmtResetTimer = millis();
  
          /* Get all data from the received code so this can be sent in transmitCoordinates() */
          baseCmd = code & 0x1F;
          baseBat = (code >> 5) & 0x07;
          baseId = (code >> 8) & 0xFFFF;
          if (code != codeOld) {
            debug("STATUS - RMT - MP Code: " + (String)code + ", ID: " + (String)baseId + ", Cmd: " + (String)baseCmd + ", vBat: " + (String)baseBat);
            codeOld = code;
          }
        }
      }
      
      /* If the code is not using the MP protocol, send as normal IR code */
      else if (received.code) {
        //debug("STATUS - RMT - IR Protocol: " + (String)received.protocol + ", Code: " + (String)received.code);
        //sendIRcode(received.protocol, received.code);
      }
    }
  
    /* Clear the code data if no new data has been received for 250ms */
    if (baseCmd != 0 && millis() - rmtResetTimer >= 250) {
        baseCmd = 0;
        baseBat = 0;
        baseId = 0;
    }
  }
/********************************* All other sensors ***************************************/
#else
  void initializeRmt() {}
  void rmtLoop() {}
#endif
