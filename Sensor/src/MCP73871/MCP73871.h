#ifndef MCP73871_H
#define MCP73871_H

#include "Arduino.h"

/**
 * Enums
 */
enum MCP73871_EN
{
    DISABLE,
    ENABLE
};

enum MCP73871_CURR
{
    CURR_100MA,
    CURR_500MA,
    CURR_MAX
};

const String currentModeString[] =
{
    "100ma",
    "500ma",
    "max"
};

enum MCP73871_STAT
{
    STAT_SHUTDOWN,  //0
    STAT_LOW_POWER, //1
    STAT_DISABLED,  //2
    STAT_CHARGING,  //3
    STAT_CHARGED,   //4
    STAT_FAULT,     //5
    STAT_LOW_BAT,   //6
    STAT_NO_BAT,    //7
    STAT_ERR        //8
};

const String statusString[] =
{
    "Not Charging",
    "Low Power",
    "Disabled",
    "Charging",
    "Charged",
    "Fault",
    "Low Battery",
    "No Battery",
    "Error"
};

/**
 * @brief MCP73871 class
 * 
 */

class MCP73871
{
    public:
        MCP73871(int usbSel, int prog, int te, int ce, int pg, int stat1, int stat2);
        void begin();
        void enableCharging();
        void disableCharging();
        void enableTimer();
        void disableTimer();
        void setCurrentMode(MCP73871_CURR current);
        MCP73871_CURR getCurrentMode();
        String getCurrentModeString();
        uint8_t getStatusRaw();
        MCP73871_STAT getStatus();
        String getStatusString();

        void registerInterrupts();
        void interruptHandlerStat1();
        void interruptHandlerStat2();
        void interruptHandlerPG();
        uint8_t getInterruptState();

    private:

        //Variables
        bool _started = false;
        bool _chargeEnable = ENABLE; 
        MCP73871_CURR _currentMode = CURR_500MA;
        bool _timerEnable = ENABLE; 
        bool _interruptTriggeredStat1 = false;
        bool _interruptTriggeredStat2 = false;
        bool _interruptTriggeredPG = false;

        //Pins
        uint8_t _usbSelPin;
        uint8_t _progPin;
        uint8_t _tePin;
        uint8_t _cePin;
        uint8_t _pgPin;
        uint8_t _stat1Pin;
        uint8_t _stat2Pin;
};

#endif /* MCP73871 */