#ifndef wiiCam_H
#define wiiCam_H

#include "Arduino.h"
#include "../IrPoint/IrPoint.h"

class wiiCam
{
  public:
    wiiCam(uint8_t sda, uint8_t scl);
    bool begin();
    float getFramePeriod();
    void setFramePeriod(float period);
    float getExposureTime();
    void setExposureTime(float val);
    float getGain();
    void setGain(float gainTemp);
    void setSensitivity(uint8_t val);
    uint8_t getPixelBrightnessThreshold();
    void setPixelBrightnessThreshold(uint8_t value);
    bool getOutput(IrPoint *irPoints);
    bool getInterruptState();
    void writeRegister(uint8_t reg, uint32_t val, uint8_t bytesToWrite = 1);
    void writeRegister2(uint8_t reg, uint32_t val, uint8_t bytesToWrite = 1);

    uint8_t detectedPoints;
    bool connected = false;

  private:
    uint32_t readRegister(uint8_t reg, uint8_t bytesToRead = 1);
    
    
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
    uint8_t sensitivityBlock1[9];
    uint8_t sensitivityBlock2[2];
    uint64_t _framePeriodTimer;
};
#endif /* wiiCam_H */
