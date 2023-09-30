/**
 * Handles the native USB port on compatible sensors
 */

#ifdef NATIVE_USB
  /**
   * Callback function when an event occurs on the native USB port
   * Used to detect if a PC is connected to the USB port or if the serial port has been opened
   */
  static void usbEventCallback(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
    if(event_base == ARDUINO_USB_EVENTS){
      arduino_usb_event_data_t * data = (arduino_usb_event_data_t*)event_data;
    
      switch (event_id){
        case ARDUINO_USB_STARTED_EVENT:
          if (started) debug("STATUS - USB - Plugged In");
          usbConnected = true;
          statusTimer = millis() - STATUS_PERIOD;
          break;
        case ARDUINO_USB_STOPPED_EVENT:
          if (started) debug("STATUS - USB - Unplugged");
          usbConnected = false;
          statusTimer = millis() - STATUS_PERIOD;
          break;
        case ARDUINO_USB_SUSPEND_EVENT:
          if (started) debug("STATUS - USB  - Suspended");
          break;
        case ARDUINO_USB_RESUME_EVENT:
          if (started) debug("STATUS - USB - Resumed");
          break;
        default:
          break;
      }
    } 
    else if(event_base == ARDUINO_USB_CDC_EVENTS) {
      arduino_usb_cdc_event_data_t * data = (arduino_usb_cdc_event_data_t*)event_data;
      switch (event_id){
        case ARDUINO_USB_CDC_CONNECTED_EVENT:
          if (started) debug("STATUS - USB - CDC Connected");
          serialConnected = true;
          statusTimer = millis() - STATUS_PERIOD;
          break;
        case ARDUINO_USB_CDC_DISCONNECTED_EVENT:
          if (started) debug("STATUS - CDC Disconnected");
          serialConnected = false;
          statusTimer = millis() - STATUS_PERIOD;
          break;
        case ARDUINO_USB_CDC_LINE_STATE_EVENT:
          if (started) debug("STATUS - USB - CDC Line State - dtr: " + (String)data->line_state.dtr + " rts: " + (String)data->line_state.rts);
          break;
        case ARDUINO_USB_CDC_LINE_CODING_EVENT:
          //if (started) debug("STATUS - CDC LINE CODING: bit_rate: %u, data_bits: %u, stop_bits: %u, parity: %u\n", data->line_coding.bit_rate, data->line_coding.data_bits, data->line_coding.stop_bits, data->line_coding.parity);
          break;
        case ARDUINO_USB_CDC_RX_EVENT:
          break;
         case ARDUINO_USB_CDC_RX_OVERFLOW_EVENT:
          if (started) debug("ERR - USB - CDC RX Overflow of " + (String)data->rx_overflow.dropped_bytes + " bytes");
          break;
        default:
          break;
      }
    }
  }
#endif

/**
 * Initialize the native USB port
 */
void initializeUSB() {
  #ifdef NATIVE_USB
    Serial.printf("------------------------------------\nInitializing USB\n");
    USB.onEvent(usbEventCallback);
    #ifndef SERIAL_DEBUG
      Serial.onEvent(usbEventCallback);
    #endif
    Serial.printf("\nProduct Name:\t\t'%s'\nManufacturer Name:\t'%s'\nSerial Number:\t\t'%s'\nVID:\t\t\t0x%X\nPID:\t\t\t0x%X\nUSB connected:\t\t%s\nSerial port open:\t%s\n\n", USB_PRODUCT_NAME, USB_MANUFACTURER_NAME, USB.serialNumber(), USB_VID, USB_PID, usbConnected?"true":"false", serialConnected?"true":"false");
  #endif
}
