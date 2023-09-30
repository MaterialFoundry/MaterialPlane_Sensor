#include "MAX17260.h"
#include "Arduino.h"
#include <Wire.h>
#include <math.h>

MAX17260 *pointerToMAX17260Class;

/**
 * @brief Construct a new MAX17260::MAX17260 object
 * 
 * @param sda int - I2C SDA pin
 * @param scl int - I2C SCL pin
 */
MAX17260::MAX17260(int sda, int scl, int clk) {
    _sda = sda;
    _scl = scl;
    _i2cClk = clk;
}

/**
 * @brief Initialize the I2C port
 * 
 */
void MAX17260::begin() {
    pointerToMAX17260Class = this;
    Wire.begin(_sda, _scl);
    Wire.setClock(_i2cClk);
}

MAX17260_ERR MAX17260::initializeModel() {
    _started = true;

    //Wait until device is ready
    uint8_t counter = 0;
    while(getDataNotReady()) {
        delay(10);
        counter++;
        if (counter >= 100) return ERR_DNR; 
    }

    //Initialize configuration
    uint16_t HibCFG = getHibCfgRegister();                              //Store HibCFG

    setSoftWakeup();                                                    //Enable soft-wakeup
    setHibCfgRegister(0);                                               //Clear HibCFG
    setSoftWakeup();                                                    //Clear soft-wakeup

    setDesignCap(_designCap);                                           //Configure designCap register
    setChargeTerminationCurrent(_IchgTerm);                             //Configure charge termination current register
    writeRegister(MAX17260_VEMPTY_ADDR, _VEmpty << 7 | _VEmptyRec);     //Configure VEmpty register
    setFullSOCThreshold(_FullSOCThr);                                   //Configure FullSOCThreshold register
    setModelConfig(1 << 15 | _NTC << 13 | _VChg << 10 | _modelID << 4); //Configure model register

    if (_configRegister != 0xFFFF) setConfigRegister(_configRegister);
    if (_config2Register != 0xFFFF) setConfig2Register(_config2Register);

    //Wait until model is refreshed
    counter = 0;
    while(getModelRefreshed()) {
        delay(10);
        counter++;
        if (counter >= 100) return ERR_MODEL; 
    }
    setHibCfgRegister(HibCFG);                                          //Reset HibCFG register to original values
    clearPOR();                                                         //Clear POR bit to indicate model and parameters are loaded
    
    percTimer = millis();
    _prevPercentage = getPercentage();
    return ERR_OKAY;
}

/**
 * @brief Sets the current sense resistor
 * 
 * @param rSense uint16_t - Resistance value in mOhm
 */
void MAX17260::setSenseResistor(uint16_t rSense) {
    _rSense = rSense;
}

/**
 * @brief Returns the configured sense resistor
 * 
 * @return uint16_t - Resitance value in mOhm
 */
uint16_t MAX17260::getSenseResistor() {
    return _rSense;
}

/**
 * @brief Sets the design capacity of the battery
 * 
 * @param cap uint16_t - Capacity in mAh
 */
void MAX17260::setDesignCap(uint16_t cap) {
    _designCap = cap;
    if (_started) writeRegister(MAX17260_DESIGNCAP_ADDR, calcFromCapacity(_designCap));
}

/**
 * @brief Returns the configured design capacity of the battery
 * 
 * @return uint16_t - Capacity in mAh
 */
uint16_t MAX17260::getDesignCap() {
    return calcCapacity(readRegister(MAX17260_DESIGNCAP_ADDR));
}

/**
 * @brief Sets the configured empty voltage of the battery
 * 
 * @param voltage uint16_t - Voltage in mV
 */
void MAX17260::setEmptyVoltage(uint16_t voltage) {
    _VEmpty = voltage / 10;
    if (!_started) return;
    uint16_t v = _VEmpty << 7 | readRegister(MAX17260_VEMPTY_ADDR) & 0x7F;
    writeRegister(MAX17260_VEMPTY_ADDR, v);
}

/**
 * @brief Returns the configured empty voltage of the battery
 * 
 * @return uint16_t - Voltage in mV
 */
uint16_t MAX17260::getEmptyVoltage() {
    return 10 * ((readRegister(MAX17260_VEMPTY_ADDR) >> 7) & 0x1FF);
}

/**
 * @brief Sets the configured empty recovery voltage of the battery
 * 
 * @param voltage uint16_t - Voltage in mV
 */
void MAX17260::setEmptyRecoveryVoltage(uint16_t voltage) {
    _VEmptyRec = voltage / 40;
    if (!_started) return;
    uint16_t v = _VEmptyRec & 0x7F | readRegister(MAX17260_VEMPTY_ADDR) & 0xFF80;
    writeRegister(MAX17260_VEMPTY_ADDR, v);
}

/**
 * @brief Returns the configured empty voltage of the battery
 * 
 * @return uint16_t - Voltage in mV
 */
uint16_t MAX17260::getEmptyRecoveryVoltage() {
    return 40 * (readRegister(MAX17260_VEMPTY_ADDR) & 0x7F);
}

/***********************************************************
 * Soft-Wakeup Register Functions
 ***********************************************************/
uint16_t MAX17260::getSoftWakeRegister() {
    return readRegister(MAX17260_SOFTWAKE_ADDR);
}

void MAX17260::clearSoftWakeup() {
    writeRegister(MAX17260_SOFTWAKE_ADDR, 0x00);
}

void MAX17260::setSoftWakeup() {
    writeRegister(MAX17260_SOFTWAKE_ADDR, 0x90);
}

/***********************************************************
 * Status Register Functions
 ***********************************************************/
uint16_t MAX17260::getStatusRegister() {
    return readRegister(MAX17260_STATUS_ADDR);
}

void MAX17260::setStatusRegister(uint16_t data) {
    writeRegister(MAX17260_STATUS_ADDR, data);
}

bool MAX17260::getPOR() {
    bool POR = (getStatusRegister() >> 1) & 1;
    if (POR) _started = true;
    return POR;
}

void MAX17260::clearPOR() {
    setStatusRegister(getStatusRegister() & 0xFFFD);
}

void MAX17260::clearAlertFlags() {
    uint16_t status = getStatusRegister() & ~MAX17260_ALERTFLAGS_bm;
    setStatusRegister(status);
}

uint16_t MAX17260::getAlertFlags(bool clearFlags) {
    uint16_t status = getStatusRegister();
    bool minCurr = (status & MAX17260_MINCURRFLAG_bm) >>  2;
    bool maxCurr = (status & MAX17260_MAXCURRFLAG_bm) >>  6;
    bool minVolt = (status & MAX17260_MINVOLTFLAG_bm) >>  8;
    bool maxVolt = (status & MAX17260_MAXVOLTFLAG_bm) >> 12;
    bool minTemp = (status & MAX17260_MINTEMPFLAG_bm) >>  9;
    bool maxTemp = (status & MAX17260_MAXTEMPFLAG_bm) >> 13;
    bool minSoc  = (status & MAX17260_MINSOCFLAG_bm)  >> 10;
    bool maxSoc  = (status & MAX17260_MAXSOCFLAG_bm)  >> 14;
    bool soc     = (status & MAX17260_SOCFLAG_bm)     >>  7;
    bool batIn   = (status & MAX17260_BATINFLAG_bm)   >>  3;
    bool batRem  = (status & MAX17260_BATREMFLAG_bm)  >> 15;
    if (clearFlags) clearAlertFlags();
    return minCurr | maxCurr << 1 | minVolt << 2 | maxVolt << 3 | minTemp << 4 | maxTemp << 5 | minSoc << 6 | maxSoc << 7 | soc << 8 | batIn << 9 | batRem << 10;
}

String MAX17260::alertFlagsToString(uint8_t flag) {
    return alertFlagsString[flag];
}

/***********************************************************
 * FStat Register Functions
 ***********************************************************/
uint16_t MAX17260::getFStatRegister() {
    return readRegister(MAX17260_FSTAT_ADDR);
}

void MAX17260::setFStatRegister(uint16_t data) {
    writeRegister(MAX17260_FSTAT_ADDR, data);
}

bool MAX17260::getDataNotReady() {
    return getFStatRegister() & 1;
}

/***********************************************************
 * HibCfg Register Functions
 ***********************************************************/
uint16_t MAX17260::getHibCfgRegister() {
    return readRegister(MAX17260_HIBCFG_ADDR);
}

void MAX17260::setHibCfgRegister(uint16_t data) {
    writeRegister(MAX17260_HIBCFG_ADDR, data);
}

/***********************************************************
 * Model Config Register Functions
 ***********************************************************/
/**
 * @brief Sets the model config register
 * 
 * @param data uint16_t - data to set
 */
void MAX17260::setModelConfig(uint16_t data) {
    writeRegister(MAX17260_MODELCFG_ADDR, data);
}

/**
 * @brief Returns the model config register
 * 
 * @return uint16_t - data in register
 */
uint16_t MAX17260::getModelConfig() {
    return readRegister(MAX17260_MODELCFG_ADDR);
}


void MAX17260::refreshModel() {
    setModelConfig(MAX17260_REFRESHMODEL_bm);
}

bool MAX17260::getModelRefreshed() {
    return readRegister(MAX17260_MODELCFG_ADDR) >> 15;
}

/**
 * @brief Get configured NTC type
 * 
 * @return NTC_100K - 100kOhm
 * @return NTC_10K - 10kOhm
 */
MAX17260_NTC MAX17260::getNtcType() {
    return (MAX17260_NTC)((getModelConfig() & MAX17260_NTC_bm) >> 13);
}

/**
 * @brief Set NTC Type
 * 
 * @param type MAX17260_NTC - Set NTC_10K for 10kOhm or NTC_100K for 100kOhm
 */
void MAX17260::setNtcType(MAX17260_NTC type) {
    _NTC = type;
    if (!_started) return;
    uint16_t reg = getModelConfig();
    if (_NTC == NTC_100K) reg |= MAX17260_NTC_bm;
    else    reg &= ~MAX17260_NTC_bm;
    setModelConfig(reg);
}

/**
 * @brief Returns the configured battery model
 * 
 * @return uint8_t - 0 for lithium cobalt-oxide, 2 for lithium NCR or NCA, 6 for lithium iron-phosphate
 */
MAX17260_MODELID MAX17260::getBatteryModel() {
    return (MAX17260_MODELID)((getModelConfig() & MAX17260_MODELID_bm) >> 4);
}

/**
 * @brief Sets the battery model
 * 
 * @param model uint8_t - 0 for lithium cobalt-oxide, 2 for lithium NCR or NCA, 6 for lithium iron-phosphate
 */
void MAX17260::setBatteryModel(MAX17260_MODELID model) {
    _modelID = model;
    if (_started) setModelConfig((_modelID << 4) | (getModelConfig() & ~MAX17260_MODELID_bm));
}

/**
 * @brief Returns the configured charging voltage
 * 
 * @return true - Charging voltages lower than 4.25V
 * @return false - Charging voltages higher than 4.25V
 */
bool MAX17260::getChargingVoltage() {
    return (getModelConfig() & MAX17260_VCHG_bm) >> 10;
}

/**
 * @brief Sets the charging voltage
 * 
 * @param val bool - false for voltages < 4.25V, true for voltages >= 4.25V
 */
void MAX17260::setChargingVoltage(bool val) {
    setModelConfig((val << 10) | (getModelConfig() & ~MAX17260_VCHG_bm));
}


void MAX17260::setRComp0(uint16_t val) {
    writeRegister(MAX17260_RCOMP0_ADDR, val);
}

uint16_t MAX17260::getRComp0() {
    return readRegister(MAX17260_RCOMP0_ADDR);
}

void MAX17260::setTempCo(uint16_t val) {
    writeRegister(MAX17260_TEMPCO_ADDR, val);
}

uint16_t MAX17260::getTempCo() {
    return readRegister(MAX17260_TEMPCO_ADDR);
}

void MAX17260::setFullCapNom(uint16_t cap) {
    return writeRegister(MAX17260_FULLCAPNOM_ADDR, calcFromCapacity(cap));
}

uint16_t MAX17260::getFullCapNom() {
    return readRegister(MAX17260_FULLCAPNOM_ADDR);
}

/***********************************************************
 * End Model Config Register Functions
 ***********************************************************/

uint16_t MAX17260::getChargeTerminationCurrent() {
    return calcCurrent(readRegister(MAX17260_ICHGTERM_ADDR));
}

void MAX17260::setChargeTerminationCurrent(uint16_t val) {
    _IchgTerm = val;
    if (!_started) return;
    _IchgTerm = calcFromCurrent(val);
    writeRegister(MAX17260_ICHGTERM_ADDR, _IchgTerm);
}



uint16_t MAX17260::getCapacity() {
    return calcCapacity(readRegister(MAX17260_REPCAP_ADDR));
}

float MAX17260::getPercentage() {
    return calcPercentage(readRegister(MAX17260_REPSOC_ADDR));
}

uint8_t MAX17260::getPercentageInt() {
    return round(getPercentage());
}

float MAX17260::getPercentagePerHour() {
    float currentPercentage = getPercentage();
    if (percTimer == 99999) {
        percTimer = millis();
        _prevPercentage = currentPercentage;
        return 0;
    }
    float diff = currentPercentage - _prevPercentage;
    float timeDiff = (millis() - percTimer);
    percTimer = millis();
    _prevPercentage = currentPercentage;
    return diff * 3600000 / timeDiff;
}

uint16_t MAX17260::getFullCapacity() {
    return calcCapacity(readRegister(MAX17260_FULLCAPREP_ADDR));
}

void MAX17260::setFullCapacity(uint16_t cap) {
    return writeRegister(MAX17260_FULLCAPREP_ADDR, calcFromCapacity(cap));
}

uint16_t MAX17260::getTimeToEmpty() {
    return calcTime(readRegister(MAX17260_TTE_ADDR));
}   

uint16_t MAX17260::getTimeToFull() {
    return calcTime(readRegister(MAX17260_TTF_ADDR));
}

uint16_t MAX17260::getCycles() {
    return readRegister(MAX17260_CYCLES_ADDR);
}

void MAX17260::setCycles(uint16_t cycles) {
    writeRegister(MAX17260_CYCLES_ADDR, cycles);
}





uint16_t MAX17260::getFullSOCThreshold() {
    return calcPercentage(readRegister(MAX17260_FULLSOCTHR_ADDR));
}

void MAX17260::setFullSOCThreshold(uint16_t thr) {
    _FullSOCThr = thr;
    if (!_started) return;
    uint16_t val = (_FullSOCThr*256 & 0xFFF8) | 0x05;
    writeRegister(MAX17260_FULLSOCTHR_ADDR, val);
}









/**
 * @brief Returns the current battery voltage
 * 
 * @return uint16_t - Voltage in mV
 */
uint16_t MAX17260::getVoltage() {
    return calcVoltage(readRegister(MAX17260_VCELL_ADDR));
}

/**
 * @brief Returns the average battery voltage
 * 
 * @return uint16_t - Voltage in mV
 */
uint16_t MAX17260::getAverageVoltage() {
    return calcVoltage(readRegister(MAX17260_AVGVCELL_ADDR));
}

/*
MAXMIN
*/

/**
 * @brief Returns the current into the battery
 * 
 * @return float - Current in mA
 */
float MAX17260::getCurrent() {
    return calcCurrent(readRegister(MAX17260_CURRENT_ADDR));
}

/**
 * @brief Returns the average current into the battery
 * 
 * @return float - Current in mA
 */
float MAX17260::getAverageCurrent() {
    return calcCurrent(readRegister(MAX17260_AVGCURRENT_ADDR));
}

/**
 * @brief Returns the power in mW
 * 
 * @return int16_t - Power in mW
 */
int16_t MAX17260::getPower() {
    return calcPower(readRegister(MAX17260_POWER_ADDR));
}

/**
 * @brief Returns the average power in mW
 * 
 * @return int16_t - Power in mW
 */
int16_t MAX17260::getAveragePower() {
    return calcPower(readRegister(MAX17260_AVGPOWER_ADDR));
}

uint16_t MAX17260::getOpenCircuitVoltage() {
    return calcVoltage(readRegister(MAX17260_VFOCV_ADDR));
}




/**
 * @brief Calculates the voltage from the register value
 * 
 * @param val uint16_t - Register value
 * @return uint16_t - Calculated voltage
 */
uint16_t MAX17260::calcVoltage(uint16_t val) {
    return 1.25*val/16;
}

//Calculates the current in mA
float MAX17260::calcCurrent(int16_t val) {
    return (float)val*1.5625/_rSense;
}

int16_t MAX17260::calcFromCurrent(float val) {
    return (int16_t)(val*_rSense/1.5625);
}

int16_t MAX17260::calcPower(int val) {
    uint32_t temp = 8*val/_rSense;
    return (uint16_t)temp;
}

uint16_t MAX17260::calcCapacity(uint16_t val) {
    uint32_t temp = 5*val/_rSense;
    return (uint16_t)temp;
}

uint16_t MAX17260::calcFromCapacity(uint16_t cap) {
    uint32_t temp = cap*_rSense/5;
    return (uint16_t)temp;
}

float MAX17260::calcPercentage(uint16_t val) {
    return (float)val/256;
}

uint16_t MAX17260::calcTime(uint16_t val) {
    return 5.625 * val;
}

////////////////////////////////////////////
//Config Registers
////////////////////////////////////////////
void MAX17260::setConfigRegister(uint16_t config) {
    _configRegister = config;
    writeRegister(MAX17260_CONFIG_ADDR, config);
}

uint16_t MAX17260::getConfigRegister() {
    _configRegister = readRegister(MAX17260_CONFIG_ADDR);
    return _configRegister;
}

void MAX17260::setConfig2Register(uint16_t config) {
    _config2Register = config;
    writeRegister(MAX17260_CONFIG2_ADDR, config);
}

uint16_t MAX17260::getConfig2Register() {
    _config2Register = readRegister(MAX17260_CONFIG2_ADDR);
    return _config2Register;
}

void MAX17260::setStickyAlerts(bool val) {
    if (_configRegister == 0xFFFF) getConfigRegister();
    if (val) _configRegister |= MAX17260_STICKYALRT_bm;
    else    _configRegister &= ~MAX17260_STICKYALRT_bm;
    if (_started) setConfigRegister(_configRegister);
}

bool MAX17260::getStickyAlerts() {
    uint16_t conf = getConfigRegister() & MAX17260_STICKYALRT_bm;
    return conf == MAX17260_STICKYALRT_bm;
}

void MAX17260::setThresholdAlert(bool val) {
    if (_configRegister == 0xFFFF) getConfigRegister();
    if (val) _configRegister |= MAX17260_AEN_bm;
    else    _configRegister &= ~MAX17260_AEN_bm;
    if (_started) setConfigRegister(_configRegister);
}

bool MAX17260::getThresholdAlert() {
    return (getConfigRegister() & MAX17260_AEN_bm) >> 2;
}

void MAX17260::setBatteryInsertAlert(bool val) {
    if (_configRegister == 0xFFFF) getConfigRegister();
    if (val) _configRegister |= MAX17260_BEI_bm;
    else    _configRegister &= ~MAX17260_BEI_bm;
    if (_started) setConfigRegister(_configRegister);
}

bool MAX17260::getBatteryInsertAlert() {
    return (getConfigRegister() & MAX17260_BEI_bm) >> 1;
}

void MAX17260::setBatteryRemoveAlert(bool val) {
    if (_configRegister == 0xFFFF) getConfigRegister();
    if (val) _configRegister |= MAX17260_BER_bm;
    else    _configRegister &= ~MAX17260_BER_bm;
    if (_started) setConfigRegister(_configRegister);
}

bool MAX17260::getBatteryRemoveAlert() {
    return getConfigRegister() & MAX17260_BER_bm;
}

void MAX17260::setSocAlert(bool val) {
    if (_config2Register == 0xFFFF) getConfig2Register();
    if (val) _config2Register |= MAX17260_DSOCEN_bm;
    else    _config2Register &= ~MAX17260_DSOCEN_bm;
    if (_started) setConfig2Register(_config2Register);
}

bool MAX17260::getSocAlert() {
    return (getConfigRegister() & MAX17260_DSOCEN_bm) >> 7;
}

void MAX17260::setTemperatureAlert(bool val) {
    if (_config2Register == 0xFFFF) getConfig2Register();
    if (val) _config2Register |= MAX17260_TALRTEN_bm;
    else    _config2Register &= ~MAX17260_TALRTEN_bm;
    if (_started) setConfigRegister(_config2Register);
}

bool MAX17260::getTemperatureAlert() {
    return (getConfigRegister() & MAX17260_TALRTEN_bm) >> 6;
}

////////////////////////////////////////////
//Threshold registers
////////////////////////////////////////////
void MAX17260::setCurrentThreshold(int16_t min, int16_t max) {
    if (min == 0 && max == 0) {
        writeRegister(MAX17260_VALRTH_ADDR, 0xFF00);
        return;
    }
    int8_t minCurr = 4*min/_rSense;
    int8_t maxCurr = 4*max/_rSense;
    writeRegister(MAX17260_IALRTH_ADDR, maxCurr<<8 | minCurr);
}

void MAX17260::setVoltageThreshold(uint16_t min, uint16_t max) {
    if (min == 0 && max == 0) {
        writeRegister(MAX17260_VALRTH_ADDR, 0xFF00);
        return;
    }
    int8_t minVolt = min/20;
    int8_t maxVolt = max/20;
    writeRegister(MAX17260_VALRTH_ADDR, maxVolt<<8 | minVolt);
}

void MAX17260::setTemperatureThreshold(int8_t min, int8_t max) {
    if (min == 0 && max == 0) {
        writeRegister(MAX17260_TALRTH_ADDR, 0x7F80);
        return;
    }
    writeRegister(MAX17260_TALRTH_ADDR, max<<8 | min);
}

void MAX17260::setSocThreshold(uint8_t min, uint8_t max) {
    if (min == 0 && max == 0) {
        writeRegister(MAX17260_SALRTH_ADDR, 0xFF00);
        return;
    }
    writeRegister(MAX17260_SALRTH_ADDR, max<<8 | min);
}

////////////////////////////////////////////
//Interrupts
////////////////////////////////////////////
/*
void MAX17260::registerCustomInterrupt(int intPin, void (*callback)(void)) {
    _alrt = intPin;
    pinMode(_alrt, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(_alrt), callback, CHANGE);
}
*/

void MAX17260::interruptHandler() {
    _interruptTriggered = true;
}

static void MAX17260outsideInterruptHandler() {
    pointerToMAX17260Class->interruptHandler();
}

void MAX17260::registerInterrupt(int intPin) {
    _alrt = intPin;
    pinMode(_alrt, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(_alrt), ::MAX17260outsideInterruptHandler, CHANGE);
}

bool MAX17260::getInterruptState() {
    bool temp = _interruptTriggered;
    _interruptTriggered = false;
   return temp;
}




////////////////////////////////////////////
//Write/Read Registers
////////////////////////////////////////////
void MAX17260::writeRegister(uint8_t addr, uint16_t data) {
    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(addr);
    Wire.write(data & 0xFF);
    Wire.write(data >> 8);
    Wire.endTransmission();
}

int16_t MAX17260::readRegister(uint8_t addr) {
    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(addr);
    Wire.endTransmission();
    Wire.requestFrom(I2C_ADDRESS, 2);
    uint8_t rec1 = Wire.read();
    uint8_t rec2 = Wire.read();
    Wire.endTransmission();
    return (int16_t)((rec2 << 8) | rec1);
}