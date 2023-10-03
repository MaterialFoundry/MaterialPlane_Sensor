#include "IrPoint.h"
#include "../RunningAverage/RunningAverage.h"
#include "Arduino.h"
#include "../Homography/homography.h"

/**
 * @brief Construct a new Ir Point:: Ir Point object
 * 
 */
IrPoint::IrPoint() {
  _xAverage.setNrOfReadings(20);
  _yAverage.setNrOfReadings(20);
  _brightnessAverage.setNrOfReadings(10);
  _maxBrightnessAverage.setNrOfReadings(10);
}

bool IrPoint::updateData() {
    //If invalidCount is too large
    if (invalidCount > ENDREPEATS) {
        invalidCount = ENDREPEATS+1;
        return false;
    }
    //If invalidCount is equal to ENDREPEATS, allow a last data transfer to indicate the point is removed
    if (invalidCount == ENDREPEATS) {
        xRaw = 4095;
        yRaw = 4095;
        x = -9999;
        y = -9999;
        avgBrightness = 0;
        maxBrightness = 0;
        area = 0;
        return true;
    }
    //If invalidCount is bigger than 1, it means there was no point detected. Don't update anything but allow data transfer
    if (invalidCount > 1) return true; 

    valid = true;

    x = (float)_xAverage.getAverage(xRaw*16)/16;
    y = (float)_yAverage.getAverage(yRaw*16)/16;
   
    
    if (_calibration) {
        _cal.calculateCoordinates(x, y);
        x = _cal.getX();
        y = _cal.getY();
    }
    /*
    if (_offsetCalibration) {
        _offsetCal.calculateCoordinates(x, y);
        x = _offsetCal.getX();
        y = _offsetCal.getY();
    }
    */
    
    
    
    float xTemp, yTemp;

    //Set rotation and mirror
    if (_rotation) {
        xTemp = _mirrorX ? 4096 - y : y;
        yTemp = _mirrorY ? 4096 - x : x;
    }
    else {
        xTemp = _mirrorX ? 4096 - x : x;
        yTemp = _mirrorY ? 4096 - y : y;
    }
  
    //Add offset
    xTemp += _offsetX;
    yTemp += _offsetY;

    //Scale coordinates
    xTemp = (xTemp-2048)*_scaleX+2048;
    yTemp = (yTemp-2048)*_scaleY+2048;

    //Make sure coordinates are within limits
    //if (xTemp < 0) xTemp = 0;
    //if (yTemp < 0) yTemp = 0;
    x = xTemp;
    y = yTemp;
    return true;
}

void IrPoint::setAverageCount(uint8_t averageCount) {
    if (averageCount < 1) averageCount = 1;
    _averageCount = averageCount;
    _xAverage.setNrOfReadings(averageCount);
    _yAverage.setNrOfReadings(averageCount);
    _brightnessAverage.setNrOfReadings(averageCount/2);
    _maxBrightnessAverage.setNrOfReadings(averageCount/2);
}

uint8_t IrPoint::getAverageCount() {
  return _averageCount;
}

void IrPoint::reset() {
  _xAverage.reset();
  _yAverage.reset();
  _brightnessAverage.reset();
  _maxBrightnessAverage.reset();
  valid = false;
}

void IrPoint::clearInvalidPoint() {
    if (valid && invalidCount == ENDREPEATS+1) reset();
}

void IrPoint::setArea(uint16_t a) {
    area = a;
}

void IrPoint::setXRaw(uint16_t x) {
    xRaw = x;
}

void IrPoint::setYRaw(uint16_t y) {
    yRaw = y;
}

void IrPoint::setAvgBrightness(uint8_t brightness) {
    avgBrightness = _brightnessAverage.getAverage(brightness);
}

void IrPoint::setMaxBrightness(uint8_t brightness) {
    maxBrightness = _maxBrightnessAverage.getAverage(brightness);
}

void IrPoint::setMirrorX(bool mirrorX) {
    _mirrorX = mirrorX;
}

void IrPoint::setMirrorY(bool mirrorY) {
    _mirrorY = mirrorY;
}

void IrPoint::setRotation(bool rotation) {
    _rotation = rotation;
}

void IrPoint::setOffset(int16_t offsetX, int16_t offsetY) {
    _offsetX = offsetX;
    _offsetY = offsetY;
}

void IrPoint::setOffsetX(int16_t offset) {
    _offsetX = offset;
}

void IrPoint::setOffsetY(int16_t offset) {
    _offsetY = offset;
}

void IrPoint::setScale(float scaleX, float scaleY) {
    _scaleX = scaleX;
    _scaleY = scaleY;
}

void IrPoint::setScaleX(float scale) {
    _scaleX = scale;
}

void IrPoint::setScaleY(float scale) {
    _scaleY = scale;
}

void IrPoint::setCalibration(bool cal) {
    _calibration = cal;
}

void IrPoint::setOffsetCalibration(bool offsetCal) {
    _offsetCalibration = offsetCal;
}

void IrPoint::setCalObjects(homography &cal, homography &offsetCal) {
    _cal = cal;
    _offsetCal = offsetCal;
}