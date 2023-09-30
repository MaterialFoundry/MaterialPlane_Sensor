#ifndef IRPOINT_H
#define IRPOINT_H

#include "Arduino.h"
#include "../RunningAverage/RunningAverage.h"
#include "../Homography/homography.h"

#define ENDREPEATS  10

class IrPoint 
{
  public:
    IrPoint();
    bool updateData();
    void setAverageCount(uint8_t averageCount);
    uint8_t getAverageCount();
    void reset();
    void clearInvalidPoint();
    void setArea(uint16_t area);
    void setXRaw(uint16_t x);
    void setYRaw(uint16_t y);
    void setAvgBrightness(uint8_t brightness);
    void setMaxBrightness(uint8_t brightness);
    void setMirrorX(bool mirrorX);
    void setMirrorY(bool mirrorY);
    void setRotation(bool rotation);
    void setOffset(int16_t offsetX, int16_t offsetY);
    void setOffsetX(int16_t offset);
    void setOffsetY(int16_t offset);
    void setScale(float scaleX, float scaleY);
    void setScaleX(float scale);
    void setScaleY(float scale);
    void setCalibration(bool cal);
    void setOffsetCalibration(bool offsetCal);
    void setCalObjects(homography &cal, homography &offsetCal);
    bool valid;
    uint8_t number;
    uint8_t invalidCount = ENDREPEATS+1;
    float x = -9999;
    uint16_t xRaw = 4095;
    float y = -9999;
    uint16_t yRaw = 4095;
    float avgBrightness = 0;
    float maxBrightness = 0;
    uint16_t area;
    uint8_t range;
    uint8_t radius;
    uint8_t boundaryLeft;
    uint8_t boundaryRight;
    uint8_t boundaryUp;
    uint8_t boundaryDown;
    uint8_t aspectRatio;
    uint8_t Vx;
    uint8_t Vy;
  private:
    uint8_t _averageCount;
    bool _mirrorX = false;
    bool _mirrorY = false;
    bool _rotation = false;
    int16_t _offsetX = 0;
    int16_t _offsetY = 0;
    float _scaleX = 1;
    float _scaleY = 1;
    bool _calibration = false;
    bool _offsetCalibration = false;
    RunningAverage _xAverage;
    RunningAverage _yAverage;
    RunningAverage _brightnessAverage;
    RunningAverage _maxBrightnessAverage;
    homography _cal;
    homography _offsetCal;
};

#endif /* IRPOINT_H */