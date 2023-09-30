#ifndef ESP_RMT_PROTOCOLS_H
#define ESP_RMT_PROTOCOLS_H

#include "Arduino.h"

/**
 * Mode explanation
 * 0: 
 *      code: address<<8 | command
 *      logic: 0 = high-low, 1 = high-low
 * 1 (NEC):  
 *      code: address<<24 | ~address<<16 | command<<8 | ~command
 *      logic: 0 = high-low, 1 = high-low
 * 2 (samsung): 
 *      code: address<<24 | address<<16 | command<<8 | ~command
 *      logic: 0 = high-low, 1 = high-low
 * 3 (LG):  
 *      code: address<<20 | address<<4 | checksum
 *      logic: 0 = high-low, 1 = high-low
 */

typedef struct {
    String label;
    uint32_t carrier_freq;
    uint8_t carrier_duty;
    uint8_t length;
    uint8_t mode;
    bool invert;
    uint16_t start_mark;
    uint16_t start_space;
    uint16_t high_mark;
    uint16_t high_space;
    uint16_t low_mark;
    uint16_t low_space;
    uint16_t stop_space;
} rmt_protocol_t;

const rmt_protocol_t defaultProtocols[] = {
    {"MP",      38000,  33,   24,   0,  0,    1000,      500,     500,    1000,     500,    500,    500},
    {"NEC",     38000,  33,   32,   1,  0,    9000,     4500,     560,    1690,     560,    560,    560},
    {"samsung", 38000,  33,   32,   2,  0,    4500,     4450,     560,    1600,     560,    560,    8950},
    {"LG",      38000,  33,   28,   3,  0,    8500,     4250,     560,    1600,     560,    560,    800}
};

class RMT_PROTOCOL
{
    public:
        RMT_PROTOCOL();
        void registerProtocol(rmt_protocol_t protocol);
        rmt_protocol_t customProtocols[10];
        uint8_t activeCustomProtocols = 0;
    private:
        
};

#endif /* ESP_RMT_PROTOCOLS_H */