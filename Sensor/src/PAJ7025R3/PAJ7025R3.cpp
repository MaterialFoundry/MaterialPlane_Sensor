#include "PAJ7025R3.h"
#include "../IrPoint/IrPoint.h"
#include "Arduino.h"
#include <SPI.h>

PAJ7025R3 *pointerToPAJ7025R3Class;

/**
 * @brief Construct a new PAJ7025R3::PAJ7025R3 object
 * 
 * @param cs 
 * @param mosi 
 * @param miso 
 * @param sck 
 * @param ledSide 
 * @param vSync 
 * @param fod 
 */
PAJ7025R3::PAJ7025R3(int cs, int mosi, int miso, int sck, int ledSide, int vSync, int fod){
  pointerToPAJ7025R3Class = this;
  _cs = cs;
  _mosi = mosi;
  _miso = miso;
  _sck = sck;
  _ledSide = ledSide;
  _vSync = vSync;
  _fod = fod;
  SPI.begin(_sck, _miso, _mosi);
  pinMode(_cs,OUTPUT); //set chip select pin
  digitalWrite(_cs,HIGH);
  //SPI.beginTransaction(SPISettings(SPI_CLK , LSBFIRST, SPI_MODE3));
}

bool PAJ7025R3::begin(){
  //powerOn(false);
  resetSensor();
  delay(1);
  powerOn(true);
  delay(1);
  if (checkProductId() == false){
    reconnectCount++;
    connected = false;
    if (reconnectCount >= MAXRECONNECTS) {
      Serial.println("Error, could not connect");
      return false;
    }
    Serial.println("Error connecting to sensor, retrying");
    delay(500);
    begin();
    return false;
  }
  else
    Serial.println("Connected to sensor");

  setFrameSubstraction(0);
  setResolutionScale(4095,4095);
  setObjectLabelingMode(1);
  setBarOrientationRatio(3);
  setExposureSignal(true);

  setGain(_gain);
  setPixelBrightnessThreshold(_pixelBrightnessThreshold);
  setFramePeriod(_framePeriod);
  setExposureTime(_exposureTime);
  setPixelNoiseTreshold(_pixelNoiseThreshold);
  setMinAreaThreshold(_minAreaThreshold);
  setMaxAreaThreshold(_maxAreaThreshold);
  setObjectNumberSetting(_objectNumber);
  connected = true;
  return true;
}

uint32_t PAJ7025R3::readRegister(uint8_t reg, uint8_t bytesToRead){
  SPI.beginTransaction(SPISettings(SPI_CLK , LSBFIRST, SPI_MODE3));
  uint32_t val = 0;
  digitalWrite(_cs,LOW);
  if (bytesToRead == 1) SPI.transfer(0x80);
  else SPI.transfer(0x81);
  SPI.transfer(reg);
  for (int i=0; i<bytesToRead; i++)
    val |= SPI.transfer(0x00) << (8*i);
  digitalWrite(_cs,HIGH);
  SPI.endTransaction();
  return val;
}

/*
 * Write to the sensor register
 */
void PAJ7025R3::writeRegister(uint8_t reg, uint32_t val, uint8_t bytesToWrite){
  SPI.beginTransaction(SPISettings(SPI_CLK , LSBFIRST, SPI_MODE3));
  digitalWrite(_cs,LOW);
  if (bytesToWrite == 1) SPI.transfer(0x00);
  else SPI.transfer(0x01);
  SPI.transfer(reg);
  //SPI.transfer(val);
  for (int i=0; i<bytesToWrite; i++)
    SPI.transfer((val>>(i*8)) & 0xFF);
  digitalWrite(_cs,HIGH);
  SPI.endTransaction();
}

void PAJ7025R3::switchBank(uint8_t bank){
  writeRegister(Bank_Select, bank); //switch to register bank
}

/*
 * Set the power state of the sensor
 */
void PAJ7025R3::powerOn(bool en){
  getPowerState();
  switchBank(0x00); //switch to register bank 0x00
  if (en)
    writeRegister(Cmd_Manual_PowerControl,0x05); //cmd_manual_powercontrol_power && cmd_manual_powercontrol_sensor On = 1
  else 
    writeRegister(Cmd_Manual_PowerControl,0x00); //cmd_manual_powercontrol_power && cmd_manual_powercontrol_sensor On = 0
  writeRegister(Cmd_Manual_PowerControl_Update_Req,0x00); //make the cmd_manual_powercontrol setting effective
  writeRegister(Cmd_Manual_PowerControl_Update_Req,0x01); //make the cmd_manual_powercontrol setting effective
}

uint8_t PAJ7025R3::getPowerState() {
  switchBank(0x00); //switch to register bank 0x00
  return readRegister(Cmd_Manual_PowerControl);
}

/*
 * Reset the sensor
 */
void PAJ7025R3::resetSensor(){
  switchBank(0x00);
  writeRegister(Cmd_Global_RESETN,0x00); //cmd_global_reset
  delay(2);
}

bool PAJ7025R3::getFrameSubstration(){
  switchBank(0x00);
  return readRegister(Cmd_FrameSubstraction_On);
}

void PAJ7025R3::setFrameSubstraction(bool val){
  switchBank(0x00);
  writeRegister(Cmd_FrameSubstraction_On,val);
}

/*
 * Get the frame period of the sensor
 */
float PAJ7025R3::getFramePeriod(){
  switchBank(0x0C);
  _framePeriod = readRegister(Cmd_frame_period, 3);
  return 0.0001*_framePeriod;
}

void PAJ7025R3::setFramePeriod(float period){
  if (period > 100) period = 100;
  if (period < 5) period = 5;
  _framePeriod = 10000*period;
  switchBank(0x0C);
  writeRegister(Cmd_frame_period,_framePeriod, 3);
}

float PAJ7025R3::getExposureTime(){
  switchBank(0x01);
  _exposureTime = readRegister(B_expo_R, 2);
  return _exposureTime * 0.0002;
}

void PAJ7025R3::setExposureTime(float val){
  float maxExposureTime = (float)_framePeriod*0.0001-2.7;
  if (val > maxExposureTime) val = maxExposureTime;
  if (val > 13) val = 13;
  if (val < 0.02) val = 0.02;
  
  _exposureTime = val*5000;
  switchBank(0x0C);
  writeRegister(B_expo,(uint16_t)_exposureTime,2);
  switchBank(0x01);
  writeRegister(Bank1_Sync_Update_Flag,0x01);
}

float PAJ7025R3::getGain(){
  switchBank(0x01);
  _gain = (1 + 0.0625*readRegister(B_global_R));
  uint8_t H = readRegister(B_ggh_r);
  if (H == 0x02) _gain *= 2;
  else if (H == 0x03) _gain *= 4;
  return _gain;
}

void PAJ7025R3::setGain(float gainTemp){
  if (gainTemp < 1) gainTemp = 1;
  if (gainTemp > 8) gainTemp = 8;
  uint8_t _B_global, _B_ggh;
  if (gainTemp < 2) {
    _B_global = (gainTemp - 1)/0.0625;
    _B_ggh = 0x00;
  }
  else if (gainTemp < 4) {
    _B_global = (gainTemp - 2)/0.125;
    _B_ggh = 0x02;
  }
  else {
    _B_global = (gainTemp - 4)/0.25;
    _B_ggh = 0x03;
  }
  
  _gain = (1 + 0.0625*readRegister(B_global_R));
  if (_B_ggh == 0x02) _gain *= 2;
  else if (_B_ggh == 0x03) _gain *= 4;
 
  switchBank(0x0C);
  writeRegister(B_global, _B_global);
  writeRegister(B_ggh, _B_ggh);
  switchBank(0x01);
  writeRegister(0x01,0x01);
}

uint8_t PAJ7025R3::getPixelBrightnessThreshold(){
  switchBank(0x0C);
  return readRegister(Cmd_thd);
}

void PAJ7025R3::setPixelBrightnessThreshold(uint8_t value){
  _pixelBrightnessThreshold = value;
  switchBank(0x0C);
  writeRegister(Cmd_thd,value);
}

uint8_t PAJ7025R3::getPixelNoiseTreshold(){
  switchBank(0x00);
  return readRegister(Cmd_nthd);
}

void PAJ7025R3::setPixelNoiseTreshold(uint8_t value){
  _pixelNoiseThreshold = value;
  switchBank(0x00);
  writeRegister(Cmd_nthd,value);
}

uint16_t PAJ7025R3::getMaxAreaThreshold(){
  switchBank(0x00);
  return readRegister(Cmd_oahb, 2);
}

void PAJ7025R3::setMaxAreaThreshold(uint16_t val){
  _maxAreaThreshold = val;
  switchBank(0x00);
  writeRegister(Cmd_oahb,val, 2);
}

uint8_t PAJ7025R3::getMinAreaThreshold(){
  switchBank(0x0C);
  return readRegister(Cmd_oalb);
}

void PAJ7025R3::setMinAreaThreshold(uint8_t val){
  _minAreaThreshold = val;
  switchBank(0x0C);
  writeRegister(Cmd_oalb,val);
}

uint16_t PAJ7025R3::getXResolutionScale(){
  switchBank(0x0C);
  return readRegister(Cmd_scale_resolution_x, 2);
}

uint16_t PAJ7025R3::getYResolutionScale(){
  switchBank(0x0C);
  return readRegister(Cmd_scale_resolution_y, 2);
}

void PAJ7025R3::setResolutionScale(uint16_t x, uint16_t y){
  if (x>4095) x = 4095;
  if (y>4095) y = 4095;
  switchBank(0x0C);
  writeRegister(Cmd_scale_resolution_x, x, 2);
  writeRegister(Cmd_scale_resolution_y, y, 2);
}

bool PAJ7025R3::getObjectLabelingMode(){
  switchBank(0x00);
  return readRegister(Cmd_dsp_operation_mode);
}

void PAJ7025R3::setObjectLabelingMode(bool val){
  switchBank(0x00);
  writeRegister(Cmd_dsp_operation_mode,val);
}

uint8_t PAJ7025R3::getObjectNumberSetting(){
  switchBank(0x00);
  _objectNumber = readRegister(Cmd_max_objects_num);
  return _objectNumber;
}

void PAJ7025R3::setObjectNumberSetting(uint8_t val){
  if (val > 16) val = 16;
  switchBank(0x00);
  writeRegister(Cmd_max_objects_num,val);
  _objectNumber = val;
}

uint8_t PAJ7025R3::getBarOrientationRatio(){
  switchBank(0x00);
  return readRegister(Cmd_orientation_ratio);
}

void PAJ7025R3::setBarOrientationRatio(uint8_t val){
  switchBank(0x00);
  writeRegister(Cmd_orientation_ratio,val);
}

bool PAJ7025R3::getVsync(){
  switchBank(0x0C);
  uint8_t Vsync = readRegister(Cmd_IOMode_GPIO_08);
  if (Vsync == 0x0B) return true;
  else return false;
}

void PAJ7025R3::setVsync(bool enable){
  switchBank(0x0C);
  if (enable) writeRegister(Cmd_IOMode_GPIO_08,0x0B);
  else writeRegister(Cmd_IOMode_GPIO_08,0x02);
}

bool PAJ7025R3::getExposureSignal(){
  return false;
}

void PAJ7025R3::setExposureSignal(bool enable){
  switchBank(0x0C);
  if (enable) writeRegister(Cmd_IOMode_GPIO_13,0x08); //assign LED_SIDE to G13
  else writeRegister(Cmd_IOMode_GPIO_13,0x00);
  switchBank(0x00);
  if (enable) writeRegister(Cmd_OtherGPO_13,0x2C); //G13 output Exposure Signal
  else writeRegister(Cmd_OtherGPO_13,0xDC);
  writeRegister(Bank0_Sync_Updated_Flag,0x01); //Bank0 sync update flag
}

void PAJ7025R3::setDebugMode(uint8_t mode) {
  switchBank(0x01);
  writeRegister(B_tg_outgen_DebugMode,mode);
}

bool PAJ7025R3::checkProductId(){
  switchBank(0x00); //switch to register bank 0x00
  uint8_t val1 = readRegister(0x03);
  uint8_t val2 = readRegister(0x02);
  if (val1 == 0x70 && val2 == 0x25) return true;
  else return false;
}

void PAJ7025R3::interruptHandler() {
  _interruptTriggered = true;
}

static void PAJ7025R3outsideInterruptHandler() {
  pointerToPAJ7025R3Class->interruptHandler();
}

void PAJ7025R3::registerInterrupt() {
  pinMode(_ledSide, INPUT_PULLUP);
  attachInterrupt(_ledSide, ::PAJ7025R3outsideInterruptHandler, FALLING);
}

bool PAJ7025R3::getInterruptState() {
  bool temp = _interruptTriggered;
  _interruptTriggered = false;
  return temp;
}

bool PAJ7025R3::getOutput(IrPoint *irPoints){
  uint16_t validPoints = 0;
  detectedPoints = 0;

  switchBank(0x0A);

  for (int i=0; i<_objectNumber; i++) {
    //Check if point is valid by checking the brightness
    digitalWrite(_cs,LOW);
    SPI.transfer(0x81);
    SPI.transfer(i*9+6);
    bool valid = SPI.transfer(0x00) > 0 ? true : false;
    digitalWrite(_cs,HIGH);

    //If point is valid, get all data
    if (valid) {
      uint8_t _outputBuffer[9];
      detectedPoints++;
      validPoints |= 1<<i;
      digitalWrite(_cs,LOW);
      SPI.transfer(0x81);
      SPI.transfer(i*9);

      for (int j=0; j<9; j++)
        _outputBuffer[j] = SPI.transfer(0x00);
      digitalWrite(_cs,HIGH);

      irPoints[i].invalidCount = 0;
      uint16_t x = _outputBuffer[2] | _outputBuffer[3]<<8;
      uint16_t y = _outputBuffer[4] | _outputBuffer[5]<<8;
      irPoints[i].setArea(_outputBuffer[0] | _outputBuffer[1]<<8);
      irPoints[i].setXRaw(_outputBuffer[2] | _outputBuffer[3]<<8);
      irPoints[i].setYRaw(_outputBuffer[4] | _outputBuffer[5]<<8);
      irPoints[i].setAvgBrightness(_outputBuffer[6]);
      irPoints[i].setMaxBrightness(_outputBuffer[7]);
    }
    else {
      irPoints[i].invalidCount++;
    }
    irPoints[i].updateData();
  }
  return detectedPoints;
}