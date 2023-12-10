#include "RunningAverage.h"
#include "Arduino.h"

RunningAverage::RunningAverage() {}

uint16_t RunningAverage::getAverage(uint16_t reading) {
  //subtract last readings
  _total -= _values[_index];
  //add new readings
  _values[_index] = reading;
  //add reading to total
  _total += _values[_index];
  //increment index to go to next element
  _index++;
  //if last element has been reached, set index to 0
  if (_index >= _nrOfReadings) _index = 0;
  //increment total readings count
  _totalReadings++;
  _clearCounter++;

  //if total reading count is smaller than _nrOfReadings only divide _total by _totalReadings
  if (_totalReadings <= _nrOfReadings) return _total / _totalReadings;
  //if total reading count is big enough, recalculate _total to remove any errors
  if (_clearCounter >= _clearAt) {
    //Serial.println("Reading: " + (String)_index + '\t' + (String)_totalReadings + '\t' + (String)_nrOfReadings + '\t' + (String)_total);
    //for (int i=0; i<_nrOfReadings; i++) Serial.print((String)(_values[i]) + '\t');
    //Serial.println("Total: " + (String)(_total / _nrOfReadings));
    recount();
  }

  //return the average
  return _total / _nrOfReadings;
}

void RunningAverage::setNrOfReadings(uint8_t nrOfReadings) {
  if (nrOfReadings == 0) nrOfReadings = 1;
  else if (nrOfReadings > MAX_READINGS) nrOfReadings = MAX_READINGS;
  _nrOfReadings = nrOfReadings;
  _clearAt = _nrOfReadings*10;
  reset();
  _firstReset = true;
}

void RunningAverage::recount() {
  _total = 0;
  _clearCounter = 0;
  for (int i=0; i<_nrOfReadings; i++) _total += _values[i];
}

void RunningAverage::reset() {
  _total = 0;
  _totalReadings = 0;
  _index = 0;
  _clearCounter = 0;
  for (int i=0; i<_nrOfReadings; i++) _values[i] = 0;
}