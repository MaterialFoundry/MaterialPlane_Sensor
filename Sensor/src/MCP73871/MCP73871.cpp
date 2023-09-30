#include "MCP73871.h"
#include "Arduino.h"

MCP73871 *pointerToMCP73871Class;

MCP73871::MCP73871(int usbSel, int prog, int te, int ce, int pg, int stat1, int stat2) {
    _usbSelPin = usbSel;    //input, select between USB (low) or AC-DC (high)
    _progPin = prog;        //input, USB current select, 100mA (low) or 500mA (high) => ext pullup
    _tePin = te;            //input, safety time enable (en == low)
    _cePin = ce;            //input, charge enable (en == high) => ext pullup
    _pgPin = pg;            //output, power good (open-drain)
    _stat1Pin = stat1;      //output, stat1 (open-drain)
    _stat2Pin = stat2;      //output, stat2 (open-drain)
}

void MCP73871::begin() {
    pointerToMCP73871Class = this;
    _started = true;
    pinMode(_usbSelPin, OUTPUT);
    pinMode(_progPin, OUTPUT);
    pinMode(_tePin, OUTPUT);
    pinMode(_cePin, OUTPUT);
    pinMode(_pgPin, INPUT_PULLUP);
    pinMode(_stat1Pin, INPUT_PULLUP);
    pinMode(_stat2Pin, INPUT_PULLUP);

    //Set input to 500mA to limit current for safety reasons
    setCurrentMode(CURR_500MA);

    //Enable safety timer
    disableTimer();

    //enable charging
    enableCharging();
}

void MCP73871::enableCharging() {
    _chargeEnable = ENABLE;
    if (_started) digitalWrite(_cePin, _chargeEnable);
}

void MCP73871::disableCharging() {
    _chargeEnable = DISABLE;
    if (_started) digitalWrite(_cePin, _chargeEnable);
}

void MCP73871::enableTimer() {
    _timerEnable = ENABLE;
    if (_started) digitalWrite(_tePin, !_timerEnable);
}

void MCP73871::disableTimer() {
    _timerEnable = DISABLE;
    if (_started) digitalWrite(_tePin, !_timerEnable);
}

void MCP73871::setCurrentMode(MCP73871_CURR current) {
    _currentMode = current;
    if (!_started) return;
    if (current == CURR_100MA) {
        digitalWrite(_usbSelPin, LOW);
        digitalWrite(_progPin, LOW);
    }
    else if (current == CURR_500MA) {
        digitalWrite(_usbSelPin, LOW);
        digitalWrite(_progPin, HIGH);
    }
    else if (current == CURR_MAX) {
        digitalWrite(_usbSelPin, HIGH);
    }
}

MCP73871_CURR MCP73871::getCurrentMode() {
    return _currentMode;
}

String MCP73871::getCurrentModeString() {
    return currentModeString[_currentMode];
}

uint8_t MCP73871::getStatusRaw() {
    uint8_t stat = digitalRead(_cePin) << 3 | digitalRead(_stat1Pin) << 2 | digitalRead(_stat2Pin) << 1 | digitalRead(_pgPin);
    return stat;
}

MCP73871_STAT MCP73871::getStatus() {
    uint8_t stat = getStatusRaw();
    if (stat == 0b0111 || stat == 0b1111) return STAT_SHUTDOWN;
    else if (stat == 0b1110 || stat == 0b1100) return STAT_CHARGED;
    else if (stat == 0b0110) return STAT_DISABLED;
    else if (stat == 0b1010) return STAT_CHARGING;
    //else if (stat == 12) return STAT_CHARGED;
    else if (stat == 0b0000 || stat == 0b1000) return STAT_FAULT;
    else if (stat == 0b0011|| stat == 0b1011) return STAT_LOW_BAT;
    else if (stat == 0b0110 || stat == 0b1110) return STAT_NO_BAT;
    return STAT_ERR;
}

String MCP73871::getStatusString() {
    return statusString[getStatus()];
}

void MCP73871::interruptHandlerStat1() {
    _interruptTriggeredStat1 = true;
}
void MCP73871::interruptHandlerStat2() {
    _interruptTriggeredStat2 = true;
}
void MCP73871::interruptHandlerPG() {
    _interruptTriggeredPG = true;
}

static void MCP73871outsideInterruptHandlerStat1() {
    pointerToMCP73871Class->interruptHandlerStat1();
}

static void MCP73871outsideInterruptHandlerStat2() {
    pointerToMCP73871Class->interruptHandlerStat2();
}

static void MCP73871outsideInterruptHandlerPG() {
    pointerToMCP73871Class->interruptHandlerPG();
}

void MCP73871::registerInterrupts() {
    attachInterrupt(_stat1Pin, ::MCP73871outsideInterruptHandlerStat1, CHANGE);
    attachInterrupt(_stat2Pin, ::MCP73871outsideInterruptHandlerStat2, CHANGE);
    attachInterrupt(_pgPin, ::MCP73871outsideInterruptHandlerPG, CHANGE);
    _interruptTriggeredStat1 = false;
    _interruptTriggeredStat2 = false;
    _interruptTriggeredPG = false;
}

uint8_t MCP73871::getInterruptState() {
    uint8_t temp = _interruptTriggeredStat1 << 2 | _interruptTriggeredStat2 << 1 | _interruptTriggeredPG;
    _interruptTriggeredStat1 = false;
    _interruptTriggeredStat2 = false;
    _interruptTriggeredPG = false;
   return temp;
}