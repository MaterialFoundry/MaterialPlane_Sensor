#ifndef ESP_RMT_H
#define ESP_RMT_H

#include "Arduino.h"
#include "driver/rmt.h"
#include "protocols.h"

#define DEBUG   false
#define MIN_PERIOD 300

/**
 * Registers
 */
#define RMT_BASE_REG                  0x60016000
#define RMT_CH4CONF0_REG              RMT_BASE_REG + 0x0030
#define RMT_CH5CONF0_REG              RMT_BASE_REG + 0x0038
#define RMT_CH6CONF0_REG              RMT_BASE_REG + 0x0040
#define RMT_CH7CONF0_REG              RMT_BASE_REG + 0x0048
#define RMT_CH4_RX_CARRIER_RM_REG     RMT_BASE_REG + 0x0090
#define RMT_CH5_RX_CARRIER_RM_REG     RMT_BASE_REG + 0x0094
#define RMT_CH6_RX_CARRIER_RM_REG     RMT_BASE_REG + 0x0098
#define RMT_CH7_RX_CARRIER_RM_REG     RMT_BASE_REG + 0x009C

/**
 * Other definitions
 */
#define RMT_BUFF_SIZE       4000
#define RMT_RX_MARGIN       200

#define RMT_IDLE_THR        2000    //10ms
#define RMT_FILTER_TICKS    100    

#define RMT_CLK_DIV         100
#define RMT_TICK            (80000000/RMT_CLK_DIV/100000)

enum rmt_err {
    ok,
    protocol_not_found,
    invalid_data_length,
    could_not_write,
    invalid_config,
    invalid_install
};

typedef struct {
    String protocol;
    uint16_t address;
    uint16_t command;
    uint32_t code;
} rmt_rx_data_t;

class RMT_TX
{
    public:
        RMT_TX(uint8_t pin, uint8_t ch = 0, bool carrier = true);
        rmt_err transmitRaw(String protocol, uint32_t code);
        rmt_err transmit(String protocol, uint16_t address, uint16_t command, bool toggle = false);
        void registerProtocol(rmt_protocol_t protocol);
        String getErr(rmt_err err);

    private:
        uint8_t buildCode(rmt_protocol_t protocol, uint32_t code, bool toggle = false);
        void setItemLevel(rmt_item32_t* item, uint16_t duration0, uint16_t duration1, bool level0, bool level1);
        rmt_protocol_t getProtocol(String name);
        rmt_channel_t _channel;
        rmt_config_t _config;
        rmt_item32_t _itemArray[36];
        uint8_t _counter = 0;
};

class RMT_RX
{
    public:
        RMT_RX(uint8_t pin, uint8_t ch = 0, bool carrier = true, uint16_t low_thr = 30000, uint16_t high_thr = 45000);
        void start();
        int8_t available();
        rmt_rx_data_t read(String protocolLabel = "");
        uint32_t readRaw(String protocolLabel = "");
        void registerProtocol(rmt_protocol_t protocol);
        String getErr(rmt_err err);

    private:
        int64_t readData(String protocolLabel = "");
        int64_t parseIrItem(rmt_item32_t* item, size_t item_size, rmt_protocol_t protocol);
        bool inMargin(uint16_t val1, uint16_t val2);
        rmt_channel_t _channel;
        rmt_config_t _config;
        RingbufHandle_t _ringbuffer;
        rmt_protocol_t _protocol;
};

#endif /* ESP_RMT_H */