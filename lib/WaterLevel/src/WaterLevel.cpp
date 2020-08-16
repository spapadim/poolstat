#include <Arduino.h>

#include "WaterLevel.h"

double WaterLevelSensor::level() {
  const double adcValue = _adcPort.acquire();
  // Search ADC thresholds (linear search is more than sufficient)
  int i;
  for (i = 1;  i < _nLevels && adcValue <= _levelAdcThresholds[i-1];  i++)  // TODO FIXME!!
    ;
  return _levelValues[i];
}

void WaterLevelSensor::_initAdcThresholds() {
  // If a level is immersed in water then, effectively,
  // a resistance equal to WATER_R is added in parallel to the level resistance
  // Therefore, the effective resistance between those level wires is now:
  double immersedLevelR = (_levelR * _waterR) / (_levelR + _waterR);
  // The ADC is connected to the reference resistance (which is between the resistive ladder output and ground)
  // Thus, for each level wire (from 1 to _nLevels-1), we just have to calcu
  for (int i = 0;  i < _nLevels - 1;  i++) {
    // When the bottom i levels are immersed, the total resistance of the ladder is simply
    //   ladderR(i) := i * immersedLevelR + (_nLevels - 1 - i) * _levelR
    // Hence, the ADC reading should be
    //   adc(i) := _refR / (_refR + ladderR(i)) == 1 / (1 + ladderR(i)/_refR)
    // However, as threshold, we would shall take the midpoint between those two values, i.e.,
    //   thresh(i) := (adc(i) + adc(i+1))/2
    //
    // Some simple properties that might be useful (noted for future use, to save a couple min):
    //   1. ladderR(i+1) - ladderR(i) == immersedLevelR - _levelR
    //   2. For f(x) := 1/(1+ax) (note, for adc(i), a == 1/_refR), it follows that the mean
    //      (f(x-d) + f(x+d)) / 2 == f(x) / (1 + (a^2 * d^2) / f^2(x))
    //
    // Note: Alternatively, we could choose the threshold based on the midpoint wrt resistance, but
    //   that seems less desirable.
    //   TODO -- or is it?
    double ladderR_i  = i * immersedLevelR + (_nLevels - 1 - i) * _levelR;
    double ladderR_i1 = ladderR_i + immersedLevelR - _levelR;
    double adc_i  = _refR / (_refR + ladderR_i);
    double adc_i1 = _refR / (_refR + ladderR_i1);
    _levelAdcThresholds[i] = (adc_i + adc_i1) / 2.0;
  }
}