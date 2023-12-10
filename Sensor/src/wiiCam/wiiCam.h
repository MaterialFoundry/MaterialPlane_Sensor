#ifndef wiiCam_H
#define wiiCam_H

#include "Arduino.h"
#include "../IrPoint/IrPoint.h"

//Registers
#define MAX_BRIGHTNESS_THR  0x06
#define BRIGHTNESS_1        0x08
#define MAX_OBJECTS         0x10
#define HOR_RES             0x12
#define VERT_RES            0x13
#define BRIGHTNESS_2        0x17
#define MAX_BRIGHTNESS      0x1A
#define MIN_BRIGHTNESS_THR  0x1B
#define CONFIG              0x30
#define OUTPUT_MODE         0x33
#define OUTPUT              0x36

//Bitmasks
#define ENABLE_bm           0x01
#define UPDATE_bm           0x08

/**
 * Enums
 */
enum output_mode_t
{
    MODE_BASIC = 1,
    MODE_EXTENDED = 3,
    MODE_FULL = 5
};

class wiiCam
{
  public:
    wiiCam(uint8_t sda, uint8_t scl);
    bool begin();
    float getFramePeriod();
    void setFramePeriod(float period);
    void setSensitivity(uint8_t val);
    uint8_t getPixelBrightnessThreshold();
    void setPixelBrightnessThreshold(uint8_t value);
    uint8_t getPixelMaxBrightnessThreshold();
    void setPixelMaxBrightnessThreshold(uint8_t value);
    void setResolution(uint8_t x, uint8_t y);
    bool getInterruptState();
    void setOutputMode(output_mode_t mode);
    bool getOutput(IrPoint *irPoints);

    uint8_t detectedPoints;
    bool connected = false;

  private:
    uint32_t readRegister(uint8_t reg);
    void writeRegister(uint8_t reg, uint32_t val);
    void updateRegisters();
    
    
    unsigned long _framePeriod = 16;
    unsigned long _exposureTime = 2;
    float _gain = 1;
    uint8_t _pixelBrightnessThreshold = 10;
    uint8_t _pixelNoiseThreshold = 5;
    uint16_t _maxAreaThreshold = 20;
    uint8_t _minAreaThreshold = 0;
    uint8_t _objectNumber = 16;
    volatile bool _interruptTriggered = false;
    uint8_t reconnectCount = 0;
    uint8_t _sda;
    uint8_t _scl;
    uint64_t _framePeriodTimer;
};
#endif /* wiiCam_H */
