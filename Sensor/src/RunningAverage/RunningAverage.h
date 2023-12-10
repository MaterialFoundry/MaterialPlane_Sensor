#ifndef RUNNINGAVERAGE_H
#define RUNNINGAVERAGE_H

#include "Arduino.h"

#define MAX_READINGS  100

class RunningAverage {
  public:
    RunningAverage();
    uint16_t getAverage(uint16_t reading);
    void setNrOfReadings(uint8_t nrOfReadings);
    void recount();
    void reset();
  private:
    uint8_t _nrOfReadings = 10;
    uint32_t _total = 0;
    uint16_t _values[MAX_READINGS];
    uint8_t _index = 0;
    uint32_t _totalReadings = 0;
    uint16_t _clearCounter = 0;
    uint16_t _clearAt = MAX_READINGS;
    bool _firstReset = false; 
};

#endif /* RUNNINGAVERAGE_H */