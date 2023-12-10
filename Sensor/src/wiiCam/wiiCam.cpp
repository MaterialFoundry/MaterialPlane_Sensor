#include "wiiCam.h"
#include "../IrPoint/IrPoint.h"
#include "Arduino.h"
#include <Wire.h>


#define IR_ADDRESS 0x58   //I2C address
#define MSGSIZE 40        //I2C message size
#define I2C_CLK 400000    //I2C clock frequency

wiiCam::wiiCam(uint8_t sda, uint8_t scl){
  _sda = sda;
  _scl = scl;
}

bool wiiCam::begin(){
  Wire.setPins(_sda, _scl);
  Wire.begin();
  Wire.setClock(I2C_CLK);

  while(1) {
    Wire.beginTransmission(IR_ADDRESS);
    uint8_t err = Wire.endTransmission();
    if (err == 0) break;
    else if (err == 4) Serial.println("WiiMote Sensor: Unknown error");
    else Serial.println("WiiMote Sensor: Not found");
    delay(5000);
  }
  
  writeRegister(CONFIG, ENABLE_bm);
  setOutputMode(MODE_FULL);
  setPixelMaxBrightnessThreshold(255);
  setPixelBrightnessThreshold(5);
  writeRegister(MAX_BRIGHTNESS, 0);
  updateRegisters();

  _framePeriodTimer = micros();
  return true;
}

float wiiCam::getFramePeriod(){
  return _framePeriod;
}

void wiiCam::setFramePeriod(float period){
  _framePeriod = period;
}

void wiiCam::setSensitivity(uint8_t val) {
  uint8_t brightness = 268*pow(0.95, val);
  writeRegister(BRIGHTNESS_1, brightness);
  writeRegister(BRIGHTNESS_2, brightness);
}

void wiiCam::setPixelBrightnessThreshold(uint8_t value){
  writeRegister(MIN_BRIGHTNESS_THR,value);
}

uint8_t wiiCam::getPixelBrightnessThreshold(){
  return readRegister(MIN_BRIGHTNESS_THR);
}

void wiiCam::setPixelMaxBrightnessThreshold(uint8_t value){
  writeRegister(MAX_BRIGHTNESS_THR,value);
}

uint8_t wiiCam::getPixelMaxBrightnessThreshold(){
  return readRegister(MAX_BRIGHTNESS_THR);
}

void wiiCam::setResolution(uint8_t x, uint8_t y) {
  if (x > 128) x = 128;
  if (y > 96) y = 96;
  writeRegister(HOR_RES,x);
  writeRegister(VERT_RES,y);
}

bool wiiCam::getInterruptState() {
  if (micros() - _framePeriodTimer >= _framePeriod*1000) {
    _framePeriodTimer = micros();
    return true;
  }
  return false;
}

void wiiCam::setOutputMode(output_mode_t mode) {
  writeRegister(OUTPUT_MODE, mode);
}

bool wiiCam::getOutput(IrPoint *irPoints){
  detectedPoints = 0;

  //IR sensor read
  Wire.beginTransmission(IR_ADDRESS);
  Wire.write(OUTPUT);
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

uint32_t wiiCam::readRegister(uint8_t reg){
  Wire.beginTransmission(IR_ADDRESS);
  Wire.write(reg);
  Wire.endTransmission();

  Wire.requestFrom(IR_ADDRESS, 1);
  return Wire.read();
}

/*
 * Write to the sensor register
 */
void wiiCam::writeRegister(uint8_t reg, uint32_t val){
  Wire.beginTransmission(IR_ADDRESS);
  Wire.write(reg); 
  Wire.write(val);
  Wire.endTransmission();
}

void wiiCam::updateRegisters() {
  writeRegister(CONFIG, UPDATE_bm);
}