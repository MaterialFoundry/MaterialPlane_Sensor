#ifndef MAX17260_H
#define MAX17260_H

#include "Arduino.h"

/**
 * I2C data
 */
#define I2C_ADDRESS 0xB6

/**
 * Registers
 */
#define MAX17260_DESIGNCAP_ADDR     0x18
#define MAX17260_VEMPTY_ADDR        0x3A

#define MAX17260_SOFTWAKE_ADDR      0x60
#define MAX17260_STATUS_ADDR        0x00
#define MAX17260_FSTAT_ADDR         0x3D
#define MAX17260_HIBCFG_ADDR        0xBA

#define MAX17260_VCELL_ADDR         0x09
#define MAX17260_AVGVCELL_ADDR      0x19
#define MAX17260_MAXMINVOLT_ADDR    0x1B
#define MAX17260_CURRENT_ADDR       0x0A
#define MAX17260_AVGCURRENT_ADDR    0x0B
#define MAX17260_POWER_ADDR         0xB1
#define MAX17260_AVGPOWER_ADDR      0xB3
#define MAX17260_MAXMINCURR_ADDR    0x1B
#define MAX17260_MODELCFG_ADDR      0xDB
#define MAX17260_ICHGTERM_ADDR      0x1E
#define MAX17260_REPCAP_ADDR        0x05
#define MAX17260_REPSOC_ADDR        0x06
#define MAX17260_FULLCAPREP_ADDR    0x10
#define MAX17260_TTE_ADDR           0x11
#define MAX17260_TTF_ADDR           0x20
#define MAX17260_CYCLES_ADDR        0x17
#define MAX17260_FULLSOCTHR_ADDR    0x13
#define MAX17260_VFOCV_ADDR         0xFB
#define MAX17260_CONFIG_ADDR        0x1D
#define MAX17260_CONFIG2_ADDR       0xBB
#define MAX17260_IALRTH_ADDR        0xB4
#define MAX17260_SALRTH_ADDR        0x03
#define MAX17260_TALRTH_ADDR        0x02
#define MAX17260_VALRTH_ADDR        0x01

#define MAX17260_RCOMP0_ADDR        0x38
#define MAX17260_TEMPCO_ADDR        0x39
#define MAX17260_FULLCAPNOM_ADDR    0x23

/**
 * Bitmasks
 */
#define MAX17260_REFRESHMODEL_bm    0x8000
#define MAX17260_NTC_bm             0x2000
#define MAX17260_VCHG_bm            0x0400
#define MAX17260_MODELID_bm         0x00F0
#define MAX17260_CSEL_bm            0x0004
#define MAX17260_STICKYALRT_bm      0x7800
#define MAX17260_AEN_bm             0x0004
#define MAX17260_BEI_bm             0x0002
#define MAX17260_BER_bm             0x0001
#define MAX17260_DSOCEN_bm          0x0080
#define MAX17260_TALRTEN_bm         0x0040
#define MAX17260_MINCURRFLAG_bm     0x0004
#define MAX17260_MAXCURRFLAG_bm     0x0040
#define MAX17260_MINVOLTFLAG_bm     0x0100
#define MAX17260_MAXVOLTFLAG_bm     0x1000
#define MAX17260_MINTEMPFLAG_bm     0x0200
#define MAX17260_MAXTEMPFLAG_bm     0x2000
#define MAX17260_MINSOCFLAG_bm      0x0400
#define MAX17260_MAXSOCFLAG_bm      0x4000
#define MAX17260_SOCFLAG_bm         0x0080
#define MAX17260_BATINFLAG_bm       0x0800
#define MAX17260_BATREMFLAG_bm      0x8000
#define MAX17260_ALERTFLAGS_bm      0xFFC4


/**
 * Enums
 */
enum MAX17260_ERR
{
    ERR_OKAY,
    ERR_DNR,
    ERR_MODEL
};

enum MAX17260_MODELID
{
    MODEL_LICOO = 0,
    MODEL_NCRNCA = 2,
    MODEL_LIFEPO4 = 6
};

enum MAX17260_NTC
{
    NTC_10K,
    NTC_100K
};

enum MAX17260_TempSensor
{
    TS_INTERNAL,
    TS_THERMISTOR
};

const String alertFlagsString[] = {
    "minCurr",
    "maxCurr",
    "minVolt",
    "maxVolt",
    "minTemp",
    "maxTemp",
    "minSOC",
    "maxSOC",
    "dSOC",
    "battIn",
    "battRem"
};


/**
 * MAX17260 class
 */
class MAX17260
{
    public:
        MAX17260(int sda, int scl, int clk);
        void begin();
        MAX17260_ERR initializeModel();
        void setSenseResistor(uint16_t rSense);
        uint16_t getSenseResistor();
        void setDesignCap(uint16_t cap);
        uint16_t getDesignCap();
        void setEmptyVoltage(uint16_t voltage);
        uint16_t getEmptyVoltage();
        void setEmptyRecoveryVoltage(uint16_t voltage);
        uint16_t getEmptyRecoveryVoltage();

        //Soft wakeup register
        uint16_t getSoftWakeRegister();
        void clearSoftWakeup();
        void setSoftWakeup();

        //Status register
        uint16_t getStatusRegister();
        void setStatusRegister(uint16_t data);
        bool getPOR();
        void clearPOR();
        void clearAlertFlags();
        uint16_t getAlertFlags(bool clearFlags = false);
        String alertFlagsToString(uint8_t flag);

        //FStat register
        uint16_t getFStatRegister();
        void setFStatRegister(uint16_t data);
        bool getDataNotReady();

        //HibCfg register
        uint16_t getHibCfgRegister();
        void setHibCfgRegister(uint16_t data);

        //Model Config Register Functions
        void setModelConfig(uint16_t data);
        uint16_t getModelConfig();
        void refreshModel();
        bool getModelRefreshed();
        MAX17260_NTC getNtcType();
        void setNtcType(MAX17260_NTC type);
        MAX17260_MODELID getBatteryModel();
        void setBatteryModel(MAX17260_MODELID model);
        bool getChargingVoltage();
        void setChargingVoltage(bool val);
        void setRComp0(uint16_t val);
        uint16_t getRComp0();
        void setTempCo(uint16_t val);
        uint16_t getTempCo();
        void setFullCapNom(uint16_t cap);
        uint16_t getFullCapNom();
        

        uint16_t getChargeTerminationCurrent();
        void setChargeTerminationCurrent(uint16_t val);
        uint16_t getCapacity();
        float getPercentage();
        uint8_t getPercentageInt();
        uint16_t getFullCapacity();
        void setFullCapacity(uint16_t cap);
        uint16_t getTimeToEmpty();
        uint16_t getTimeToFull();
        uint16_t getCycles();
        void setCycles(uint16_t cycles);

        uint16_t getFullSOCThreshold();
        void setFullSOCThreshold(uint16_t thr);

        uint16_t getVoltage();
        uint16_t getAverageVoltage();
        float getCurrent();
        float getAverageCurrent();
        int16_t getPower();
        int16_t getAveragePower();
        uint16_t getOpenCircuitVoltage();
        float getPercentagePerHour();

        //Config registers
        void setConfigRegister(uint16_t config);
        uint16_t getConfigRegister();
        void setConfig2Register(uint16_t config);
        uint16_t getConfig2Register();
        void setTemperatureSensor(MAX17260_TempSensor);
        MAX17260_TempSensor getTemperatureSensor();
        void setStickyAlerts(bool val);
        bool getStickyAlerts();
        void setThresholdAlert(bool val);
        bool getThresholdAlert();
        void setBatteryInsertAlert(bool val);
        bool getBatteryInsertAlert();
        void setBatteryRemoveAlert(bool val);
        bool getBatteryRemoveAlert();
        void setSocAlert(bool val);
        bool getSocAlert();
        void setTemperatureAlert(bool val);
        bool getTemperatureAlert();

        //Threshold registers
        void setCurrentThreshold(int16_t min, int16_t max);
        void setVoltageThreshold(uint16_t min, uint16_t max);
        void setTemperatureThreshold(int8_t min, int8_t max);
        void setSocThreshold(uint8_t min, uint8_t max);

        //void registerCustomInterrupt(int intPin, void (*callback)(void));
        void registerInterrupt(int intPin);
        void interruptHandler();
        bool getInterruptState();

    private:
        void printBIN(uint16_t d, bool e);
        uint16_t calcVoltage(uint16_t val);
        float calcCurrent(int16_t val);
        int16_t calcFromCurrent(float current);
        int16_t calcPower(int val);
        uint16_t calcCapacity(uint16_t val);
        uint16_t calcFromCapacity(uint16_t cap);
        
        float calcPercentage(uint16_t val);
        uint16_t calcTime(uint16_t val);
        void writeRegister(uint8_t addr, uint16_t data);
        int16_t readRegister(uint8_t addr);
        bool _started = false;
        uint8_t _sda;
        uint8_t _scl;
        uint8_t _alrt;
        int _i2cClk;
        uint16_t _rSense = 20;
        uint16_t _designCap = 1200;
        uint16_t _IchgTerm = 150;
        uint16_t _VEmpty = 3100;
        uint16_t _VEmptyRec = 3500;
        uint16_t _FullSOCThr = 95;
        bool _VChg = false;
        MAX17260_NTC _NTC = NTC_10K;
        MAX17260_MODELID _modelID = MODEL_LICOO;
        uint32_t _interruptTimer = 0;
        volatile bool _interruptTriggered = false;
        uint16_t _configRegister = 0xFFFF;
        uint16_t _config2Register = 0xFFFF;
        float _prevPercentage = 0;
        unsigned long percTimer = 99999;
};

#endif /* MAX17260 */