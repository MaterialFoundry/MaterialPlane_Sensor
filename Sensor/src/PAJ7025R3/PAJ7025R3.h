#ifndef PAJ7025R3_H
#define PAJ7025R3_H

#include "Arduino.h"
#include "../IrPoint/IrPoint.h"

#define SPI_CLK                             14000000

#define MAXRECONNECTS                       5

/**
 * Registers
 */
#define Bank_Select                         0xEF

//Bank 0x00
#define Bank0_Sync_Updated_Flag             0x01  
#define Product_ID                          0x02  
#define Cmd_oahb                            0x0B  
#define Cmd_nthd                            0x0F
#define Cmd_orientation_ratio               0x10
#define Cmd_dsp_operation_mode              0x12
#define Cmd_max_objects_num                 0x19
#define Cmd_FrameSubstraction_On            0x28
#define Cmd_Manual_PowerControl             0x2F
#define Cmd_Manual_PowerControl_Update_Req  0x30
#define Cmd_OtherGPO_13                     0x4F
#define Cmd_Global_RESETN                   0x64

//Bank 0x01
#define Bank1_Sync_Update_Flag              0x01
#define B_global_R                          0x05
#define B_ggh_r                             0x06
#define B_expo_R                            0x0E  
#define B_tg_outgen_DebugMode               0x2B

//Bank 0x0C
#define Cmd_frame_period                    0x07  
#define B_global                            0x0B
#define B_ggh                               0x0C
#define B_expo                              0x0F  
#define Cmd_oalb                            0x46
#define Cmd_thd                             0x47
#define Cmd_scale_resolution_x              0x60 
#define Cmd_scale_resolution_y              0x62 
#define Cmd_IOMode_GPIO_06                  0x6A
#define Cmd_IOMode_GPIO_08                  0x6C
#define Cmd_IOMode_GPIO_13                  0x71

/**
 * PAJ7025R3 class
 */

class PAJ7025R3
{
  public:
    PAJ7025R3(int cs, int mosi, int miso, int sck, int ledSide, int vSync, int fod);
    bool begin();
    void powerOn(bool en=true);
    uint8_t getPowerState();
    void resetSensor();
    bool getFrameSubstration();
    void setFrameSubstraction(bool val);
    float getFramePeriod();
    void setFramePeriod(float period);
    float getExposureTime();
    void setExposureTime(float val);
    float getGain();
    void setGain(float gainTemp);
    uint8_t getPixelBrightnessThreshold();
    void setPixelBrightnessThreshold(uint8_t value);
    uint8_t getPixelNoiseTreshold();
    void setPixelNoiseTreshold(uint8_t value);
    uint16_t getMaxAreaThreshold();
    void setMaxAreaThreshold(uint16_t val);
    uint8_t getMinAreaThreshold();
    void setMinAreaThreshold(uint8_t val);
    uint16_t getXResolutionScale();
    uint16_t getYResolutionScale();
    void setResolutionScale(uint16_t x, uint16_t y);
    bool getObjectLabelingMode();
    void setObjectLabelingMode(bool val);
    uint8_t getObjectNumberSetting();
    void setObjectNumberSetting(uint8_t val);
    uint8_t getBarOrientationRatio();
    void setBarOrientationRatio(uint8_t val);
    bool checkProductId();
    bool getOutput(IrPoint *irPoints);
    bool getVsync();
    void setVsync(bool enable);
    bool getExposureSignal();
    void setExposureSignal(bool enable);
    void setDebugMode(uint8_t mode);

    void registerInterrupt();
    void interruptHandler();
    bool getInterruptState();

    uint8_t detectedPoints;
    bool connected = false;
  
  private:
    uint32_t readRegister(uint8_t reg, uint8_t bytesToRead = 1);
    void writeRegister(uint8_t reg, uint32_t val, uint8_t bytesToWrite = 1);
    void switchBank(uint8_t bank);
    unsigned long _framePeriod = 16;
    unsigned long _exposureTime = 2;
    float _gain = 1;
    uint8_t _pixelBrightnessThreshold = 10;
    uint8_t _pixelNoiseThreshold = 5;
    uint16_t _maxAreaThreshold = 20;
    uint8_t _minAreaThreshold = 0;
    uint8_t _objectNumber = 16;
    volatile bool _interruptTriggered = false;
    int _cs;
    int _mosi;
    int _miso;
    int _sck;
    int _ledSide;
    int _vSync;
    int _fod;
    uint8_t reconnectCount = 0;
};
#endif /* PAJ7025R3_H */
