#include "wiiCam.h"
#include "../IrPoint/IrPoint.h"
#include "Arduino.h"
#include <Wire.h>

//I2C address
#define IR_ADDRESS 0x58

//I2C message size
#define MSGSIZE 40

#define I2C_CLK 400000

wiiCam::wiiCam(uint8_t sda, uint8_t scl){
  _sda = sda;
  _scl = scl;
  Wire.begin(_sda, _scl, I2C_CLK);
  for (int i=0; i<9; i++) sensitivityBlock1[i] = 0;
  for (int i=0; i<2; i++) sensitivityBlock2[i] = 0;
}

bool wiiCam::begin(){
  writeRegister(0x30, 0x01);
  writeRegister(0x33, 0x05);
  writeRegister(0x06, 0xFF); //Set maximum intensity threshold
  writeRegister(0x1A, 0x00); //Set maximum brightness
  writeRegister(0x1B, 0x00); //Set minimum brightness threshold
  writeRegister(0x30, 0x08);
  _framePeriodTimer = micros();
  return true;
}

uint32_t wiiCam::readRegister(uint8_t reg, uint8_t bytesToRead){
  Wire.beginTransmission(IR_ADDRESS);
  Wire.write(reg);
  Wire.endTransmission();

  Wire.requestFrom(IR_ADDRESS, bytesToRead);
  return Wire.read();
}

/*
 * Write to the sensor register
 */
void wiiCam::writeRegister(uint8_t reg, uint32_t val, uint8_t bytesToWrite){
  Wire.beginTransmission(IR_ADDRESS);
  Wire.write(reg); 
  Wire.write(val);
  Wire.endTransmission();
}

void wiiCam::writeRegister2(uint8_t reg, uint32_t val, uint8_t bytesToWrite){
  Wire.beginTransmission(IR_ADDRESS);
  Wire.write(reg); 
  Wire.write(val);
  Wire.endTransmission();

  delay(10);
  
  Wire.beginTransmission(IR_ADDRESS);
  Wire.write(0x30); Wire.write(0x08);
  Wire.endTransmission();
}

void wiiCam::setSensitivity(uint8_t val) {
  uint8_t brightness = 200*pow(0.9705,val);
  writeRegister(0x08, brightness);
}

/*
 * Get the frame period of the sensor
 */
float wiiCam::getFramePeriod(){
  return 0;
}

void wiiCam::setFramePeriod(float period){
  
}

float wiiCam::getExposureTime(){
  return 0;
}

void wiiCam::setExposureTime(float val){
  
}

float wiiCam::getGain(){
  return 0;
}

void wiiCam::setGain(float gainTemp){

}

uint8_t wiiCam::getPixelBrightnessThreshold(){
  return readRegister(0x1B);
}

void wiiCam::setPixelBrightnessThreshold(uint8_t value){
  writeRegister(0x1B,value);
}

bool wiiCam::getInterruptState() {
  if (micros() - _framePeriodTimer >= _framePeriod*1000) {
    _framePeriodTimer = micros();
    return true;
  }
  return false;
}

bool wiiCam::getOutput(IrPoint *irPoints){
  detectedPoints = 0;

  //IR sensor read
  Wire.beginTransmission(IR_ADDRESS);
  Wire.write(0x36);
  Wire.endTransmission();

  // Request the 2 byte heading (MSB comes first)
  uint8_t bytesToRead = Wire.requestFrom(IR_ADDRESS, MSGSIZE);
  uint8_t outputBuffer[bytesToRead];

  int i=0;
  while(Wire.available() && i < bytesToRead) {
    outputBuffer[i] = Wire.read();
    i++;
  }
  
  for (int i=0; i<4; i++) {
    uint8_t brightness = outputBuffer[9+i*9];
    if (brightness == 255) {
      irPoints[i].invalidCount++;
    }
    else {
      detectedPoints++;
      irPoints[i].invalidCount = 0;
      irPoints[i].setArea(outputBuffer[3+i*9]&&15);
      irPoints[i].setXRaw((((outputBuffer[3+i*9]>>4)&3) << 8 | outputBuffer[1+i*9])*4);
      irPoints[i].setYRaw((((outputBuffer[3+i*9]>>6)&3) << 8 | outputBuffer[2+i*9])*4);
      irPoints[i].setMaxBrightness(brightness);
      irPoints[i].setAvgBrightness(brightness);
    }
    irPoints[i].updateData();
  }
  return detectedPoints;
}