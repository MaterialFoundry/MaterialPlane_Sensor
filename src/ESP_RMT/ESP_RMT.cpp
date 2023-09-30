#include "ESP_RMT.h"
#include "protocols.h"
#include "Arduino.h"
#include "driver/rmt.h"
#include "protocols.h"

RMT_PROTOCOL protocols;

/////////////////////////////////////////////////////////////////////////
// TX
/////////////////////////////////////////////////////////////////////////

RMT_TX::RMT_TX(uint8_t pin, uint8_t ch, bool carrier) {
    switch(ch) {
        case 0:
            _channel = RMT_CHANNEL_0;
            break;
        case 1:
            _channel = RMT_CHANNEL_1;
            break;
        case 2:
            _channel = RMT_CHANNEL_2;
            break;
        case 3:
            _channel = RMT_CHANNEL_3;
            break;
        default:
            return;
    }

    rmt_tx_config_t txConfig;
    txConfig.loop_en = false;
    txConfig.carrier_freq_hz = (int32_t)38000;
    txConfig.carrier_duty_percent = 33;
    txConfig.carrier_level = RMT_CARRIER_LEVEL_HIGH;
    txConfig.carrier_en = carrier;
    txConfig.idle_level = RMT_IDLE_LEVEL_LOW;
    txConfig.idle_output_en = true;

    _config.rmt_mode = RMT_MODE_TX;
    _config.channel = _channel;
    _config.clk_div = RMT_CLK_DIV;
    _config.gpio_num = (gpio_num_t)pin;
    _config.mem_block_num = 1;
    _config.tx_config = txConfig;

    rmt_config(&_config);
    rmt_driver_install(_channel, 0, 0);
}

rmt_protocol_t RMT_TX::getProtocol(String label) {
    rmt_protocol_t protocol;
    bool protocolFound = false;

    //check custom protocols
    for (int i=0; i<protocols.activeCustomProtocols; i++)
        if (protocols.customProtocols[i].label == label) {
            protocol = protocols.customProtocols[i];
            protocolFound = true;
            break;
        }

    //check default protocols
    if (!protocolFound) {
        for (int i=0; i<sizeof(defaultProtocols)/sizeof(protocol); i++) {
            if (defaultProtocols[i].label == label) {
                protocol = defaultProtocols[i];
                break;
            }
        }
    }
    return protocol;
}

rmt_err RMT_TX::transmit(String protocolLabel, uint16_t address, uint16_t command, bool toggle) {
    rmt_protocol_t protocol = getProtocol(protocolLabel);
    if (protocol.label == "") return protocol_not_found;
    
    uint32_t code = 0;
    if (protocol.mode == 0)
        code = address << 8 | command;
    else if (protocol.mode == 1) 
        code = address << 24 | (uint8_t)(~address) << 16 | command << 8 | (uint8_t)(~command);
    else if (protocol.mode == 2)
        code = address << 24 | address << 16 | command << 8 | (uint8_t)(~command);
    else if (protocol.mode == 3) {
        uint8_t checksum = (address&0x0F) + ((address>>4)&0x0F) + ((command>>12)&0x0F) + ((command>>8)&0x0F) + ((command>>4)&0x0F) + (command&0x0F);
        code = address << 20 | command << 4 | checksum&0x0F;
    }   

    uint8_t dataLength = buildCode(protocol, code, toggle);
    if (DEBUG) {
        Serial.print("Send: " + (String)dataLength + " ");
        Serial.println(code,BIN);
    }
    if (dataLength > 0) {
        if (protocol.carrier_freq != _config.tx_config.carrier_freq_hz || protocol.carrier_duty != _config.tx_config.carrier_duty_percent) {
            _config.tx_config.carrier_freq_hz = protocol.carrier_freq;
            _config.tx_config.carrier_duty_percent = protocol.carrier_duty;
            if (rmt_config(&_config) != ESP_OK)
                return invalid_config;
        }
        if (rmt_write_items(_channel, &_itemArray[0], dataLength, true) == ESP_OK)
            return ok;
        else return could_not_write;
    }  
    else return invalid_data_length;
}

rmt_err RMT_TX::transmitRaw(String protocolLabel, uint32_t code) {
    rmt_protocol_t protocol = getProtocol(protocolLabel);
    if (protocol.label == "") return protocol_not_found;

    uint8_t dataLength = buildCode(protocol, code);
    if (dataLength > 0) {
        if (protocol.carrier_freq != _config.tx_config.carrier_freq_hz || protocol.carrier_duty != _config.tx_config.carrier_duty_percent) {
            _config.tx_config.carrier_freq_hz = protocol.carrier_freq;
            _config.tx_config.carrier_duty_percent = protocol.carrier_duty;
            if (rmt_config(&_config) != ESP_OK)
                return invalid_config;
        }
        if (rmt_write_items(_channel, &_itemArray[0], dataLength, true) == ESP_OK)
            return ok;
        else return could_not_write;
    }  
    else return invalid_data_length;
}

void RMT_TX::setItemLevel(rmt_item32_t* item, uint16_t duration0, uint16_t duration1, bool level0, bool level1) {
    item -> level0 = level0;
    item -> level1 = level1;
    item -> duration0 = duration0 / 10 * RMT_TICK;
    item -> duration1 = duration1 / 10 * RMT_TICK;
    _itemArray[_counter] = *item;
    _counter++;
}

uint8_t RMT_TX::buildCode(rmt_protocol_t protocol, uint32_t code, bool toggle) {
    _counter = 0;
    rmt_item32_t item;

    setItemLevel(&item, protocol.start_mark, protocol.start_space, 1, 0);
    
    for (int i = protocol.length-1; i>=0; i--) {
        if ((code >> i) & 1) 
            setItemLevel(&item, protocol.high_mark, protocol.high_space, 1, 0);
        else 
            setItemLevel(&item, protocol.low_mark, protocol.low_space, 1, 0);
    }

    if (protocol.stop_space > 0)
        setItemLevel(&item, protocol.stop_space, 0, 1, 0);
    
    return _counter;
}

void RMT_TX::registerProtocol(rmt_protocol_t protocol) {
    protocols.registerProtocol(protocol);
}

String RMT_TX::getErr(rmt_err err) {
    if (err == ok) return "Ok";
    else if (err == protocol_not_found) return "Protocol not found";
    else if (err == invalid_data_length) return "Invalid data length";
    else if (err == could_not_write) return "Could not write data";
    else if (err == invalid_config) return "Invalid config";
    else if (err == invalid_install) return "Invalid install";
    else return "Invalid error code";
}

/////////////////////////////////////////////////////////////////////////
// RX
/////////////////////////////////////////////////////////////////////////

RMT_RX::RMT_RX(uint8_t pin, uint8_t ch, bool carrier, uint16_t low_thr, uint16_t high_thr) {
    switch(ch) {
        case 0:
            _channel = RMT_CHANNEL_4;
            break;
        case 1:
            _channel = RMT_CHANNEL_5;
            break;
        case 2:
            _channel = RMT_CHANNEL_6;
            break;
        case 3:
            _channel = RMT_CHANNEL_7;
            break;
        default:
            return;
    }

    uint32_t RMT_CHnCONF0_REG = RMT_CH4CONF0_REG + 8*ch;
    uint32_t RMT_CHn_RX_CARRIER_RM_REG = RMT_CH4_RX_CARRIER_RM_REG + 4*ch;
    _ringbuffer = xRingbufferCreate(RMT_BUFF_SIZE, RINGBUF_TYPE_NOSPLIT);

    rmt_rx_config_t rxConfig;
    rxConfig.filter_en = true;
    rxConfig.filter_ticks_thresh = RMT_FILTER_TICKS;
    rxConfig.idle_threshold = RMT_IDLE_THR;

    _config.rmt_mode = RMT_MODE_RX;
    _config.channel = _channel;
    _config.clk_div = RMT_CLK_DIV;
    _config.gpio_num = (gpio_num_t)pin;
    _config.mem_block_num = 1;
    _config.rx_config = rxConfig;

    rmt_config(&_config);
    rmt_driver_install(_channel, RMT_BUFF_SIZE, 0);

    if (carrier) {
        //enable carrier demodulation
        uint32_t reg = REG_READ(RMT_CHnCONF0_REG) | 1<<28;
        REG_WRITE(RMT_CHnCONF0_REG, reg);

        //set low and high threshold for carrier demodulation
        REG_WRITE(RMT_CHn_RX_CARRIER_RM_REG, (uint16_t)(1000000/low_thr)<<16 | (uint16_t)(1000000/high_thr));
    }
}

void RMT_RX::start() {
    rmt_rx_start(_channel, 1);
    rmt_get_ringbuf_handle(_channel, &_ringbuffer);
}

void RMT_RX::registerProtocol(rmt_protocol_t protocol) {
    protocols.registerProtocol(protocol);
}

int8_t RMT_RX::available() {
    UBaseType_t itemsWaiting;
    vRingbufferGetInfo(_ringbuffer, NULL, NULL, NULL, NULL, &itemsWaiting);
    //if (DEBUG) {
    //    Serial.print("Items waiting: ");
    //    Serial.println(itemsWaiting);
    //}
    return itemsWaiting;
}

uint32_t RMT_RX::readRaw(String protocolLabel) {
    int64_t code = readData(protocolLabel);
    if (code < 0) code = 0;
    return (uint32_t)code;
}

rmt_rx_data_t RMT_RX::read(String protocolLabel) {
    int64_t code = readData(protocolLabel);

    rmt_rx_data_t data;
    data.protocol = "Error";
    data.address = 0;
    data.command = 0;
    data.code = 0;

    if (code >= 0) {
        data.protocol = _protocol.label;

        uint8_t address, command;
        if (_protocol.mode == 0) {
            address = (code >> 8) & 0xFF;
            command = code & 0xFF;
        }
        else if (_protocol.mode == 1) {
            address = (code >> 24) & 0xFF;
            uint8_t addressCheck = (~(code >> 16)) & 0xFF;
            command = (code >> 8) & 0xFF;
            uint8_t commandCheck = (~code) & 0xFF;
            if (address != addressCheck || command != commandCheck) {
                address = 0;
                command = 0;
                data.protocol = "Error";
            }
        }
        else if (_protocol.mode == 2) {
            address = (code >> 24) & 0xFF;
            uint8_t addressCheck = (code >> 16) & 0xFF;
            command = (code >> 8) & 0xFF;
            uint8_t commandCheck = (~code) & 0xFF;
            if (address != addressCheck || command != commandCheck) {
                address = 0;
                command = 0;
                data.protocol = "Error";
            }
        }
        else if (_protocol.mode == 3) {
            address = (code >> 20) & 0xFF;
            command = (code >> 4) & 0xFFFF;
            uint8_t checksum = (address&0x0F) + ((address>>4)&0x0F) + ((command>>12)&0x0F) + ((command>>8)&0x0F) + ((command>>4)&0x0F) + (command&0x0F);
            if ((checksum&0x0F) != (code&0x0F)) {
                address = 0;
                command = 0;
                data.protocol = "Error";
            }
        }

        data.address = address;
        data.command = command;
        data.code = code;
    }
    return data;
}

int64_t RMT_RX::readData(String protocolLabel) {
    size_t item_size;
    
    rmt_item32_t *item = (rmt_item32_t *)xRingbufferReceive(_ringbuffer, &item_size, 10);
    rmt_item32_t itemFiltered[item_size/4];
    
    if (item == NULL) {
        if (DEBUG) Serial.println("Failed to receive item");
    }
    else {
        vRingbufferReturnItem(_ringbuffer, (void *)item);
        if (DEBUG) Serial.println("\nData received: ");
        uint8_t codeLength = item_size/4;
        //Serial.println("codeLength: " + (String)codeLength);
        uint8_t count = 0;
        bool firstValid = false;
        for (int i=0; i<codeLength; i++) {
            if (DEBUG) Serial.print((String)i + "\td0 " + (String)(item[i].duration0 * 10 / RMT_TICK) + "\td1 " + (String)(item[i].duration1 * 10 / RMT_TICK) + "\tl0 " + (String)item[i].level0 + "\tl1 " + (String)item[i].level1);
            //Filter out invalid values
            if (i<codeLength-1 && item[i].duration0 * 10 / RMT_TICK < MIN_PERIOD) {
                if (DEBUG) Serial.print("\tErr");
                if (!firstValid) {
                    if (DEBUG) Serial.println();
                    continue;
                }
                itemFiltered[count-1].duration1 += (item[i].duration0 + item[i].duration1) * 10 / RMT_TICK;
            }
            else firstValid = true;
            if (DEBUG) Serial.println();

            //Store values
            itemFiltered[count].duration0 = item[i].duration0 * 10 / RMT_TICK;
            itemFiltered[count].duration1 = item[i].duration1 * 10 / RMT_TICK;
            itemFiltered[count].level0 = item[i].level0;
            itemFiltered[count].level1 = item[i].level1;

            count++;
        }
        codeLength = count+1;
        if (DEBUG) {
            Serial.println("\nCount: " + (String)count);
            for (int i=0; i<count; i++) {
                Serial.println((String)i + "\td0 " + (String)itemFiltered[i].duration0 + "\td1 " + (String)itemFiltered[i].duration1 + "\tl0 " + (String)itemFiltered[i].level0 + "\tl1 " + (String)itemFiltered[i].level1);
            }
        }

        int64_t code = -1;

        //if protocolLabel is defined
        if (protocolLabel != "") {
            //check custom protocols
            for (int i=0; i<protocols.activeCustomProtocols; i++)
                if (protocols.customProtocols[i].label == protocolLabel) {
                    _protocol = protocols.customProtocols[i];
                    return parseIrItem(itemFiltered, codeLength, _protocol);
                }

            //check default protocols
            for (int i=0; i<sizeof(defaultProtocols)/sizeof(_protocol); i++) {
                if (defaultProtocols[i].label == protocolLabel) {
                    _protocol = defaultProtocols[i];
                    return parseIrItem(itemFiltered, codeLength, _protocol);
                }
            }
        }
        else {
            //check custom protocols
            for (int i=0; i<protocols.activeCustomProtocols; i++) {
                _protocol = protocols.customProtocols[i];
                code = parseIrItem(itemFiltered, codeLength, _protocol);
                if (code >= 0) break;
            }

            //check default protocols
            if (code == -1) {
                for (int i=0; i<sizeof(defaultProtocols)/sizeof(_protocol); i++) {
                    _protocol = defaultProtocols[i];
                    code = parseIrItem(itemFiltered, codeLength, _protocol);
                    if (code >= 0) break;
                    if (DEBUG) Serial.println("Incorrect protocol");
                }
            }
        }
        return code;
    }
  
    if (DEBUG) Serial.println();
    return -1;
}

bool RMT_RX::inMargin(uint16_t val1, uint16_t val2) {
    return (abs(val1 - val2) <= RMT_RX_MARGIN);
}

int64_t RMT_RX::parseIrItem(rmt_item32_t* item, size_t item_size, rmt_protocol_t protocol) {
    if (DEBUG) Serial.println("Protocol: " + protocol.label);

    uint8_t counter = 0;
    
    if (DEBUG) Serial.println("Length: " + (String)item_size + '\t' + (String)protocol.length);
    //Check code length
    if (item_size != protocol.length + 2 && item_size != protocol.length + 3) {
        if (DEBUG) Serial.println("Code length incorrect");
        return -1;
    }
    if (DEBUG) Serial.println("Code length correct");

    //Check start bits
    if (!inMargin(item[0].duration0, protocol.start_mark) || !inMargin(item[0].duration1, protocol.start_space) || item[0].level0 || !item[0].level1) {
        if (DEBUG) Serial.println("Start bits incorrect: " + (String)(!inMargin(item[0].duration0, protocol.start_mark)) + '\t' + (String)(!inMargin(item[0].duration1, protocol.start_space)));
        return -1;
    }
    if (DEBUG) Serial.println("Start bits correct");
    counter++;

    uint32_t code = 0;
    bool stopBitFound = false;

    for (int i=protocol.length; i>=0; i--) {
        //Check for 1
        if (inMargin(item[counter].duration0, protocol.high_mark) && inMargin(item[counter].duration1, protocol.high_space) && !item[counter].level0 && item[counter].level1) {
            //this is a 1
            code |= 1 << (i-1);
            if (DEBUG) Serial.print("1");
        }
        //Check for 0
        else if (inMargin(item[counter].duration0, protocol.low_mark) && inMargin(item[counter].duration1, protocol.low_space) && !item[counter].level0 && item[counter].level1) {
            //this is a 0
            if (DEBUG) Serial.print("0");
        }
        //Check for stop bit
        else if (inMargin(item[counter].duration0, protocol.stop_space)) {
            if (DEBUG) Serial.println("\nStop bit found");
            stopBitFound = true;
            break;
        }
        else {
            if (DEBUG) Serial.print("x");
        }
        counter++;
    }
    if (DEBUG) Serial.println();

    if (!stopBitFound) {
        if (DEBUG) Serial.println("Stop bit NOT found");
        return -1;
    }
    if (DEBUG) {
        Serial.print("Code: " + (String)code);
        Serial.print("\tBin: ");
        Serial.println(code,BIN);
    }
    return (int64_t)code;
}

String RMT_RX::getErr(rmt_err err) {
    if (err == ok) return "Ok";
    else if (err == protocol_not_found) return "Protocol not found";
    else if (err == invalid_data_length) return "Invalid data length";
    else if (err == could_not_write) return "Could not write data";
    else if (err == invalid_config) return "Invalid config";
    else if (err == invalid_install) return "Invalid install";
    else return "Invalid error code";
}