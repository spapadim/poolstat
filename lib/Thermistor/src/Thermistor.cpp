#include <Arduino.h>

#include "Thermistor.h"

/*

R = Rref / (res/adc - 1)

1/T = 1/T0 + (1/B)*ln(R/R0)

----

R/R0 = (Rref/R0) / (res/adc - 1)

ln(R/R0) = ln(Rref/R0) - ln(res/adc - 1)

(1/B)*ln(R/R0) = (ln(Rref/R0)/B) - ln(res/adc - 1)/B =: C

1/T = 1/T0 + C => T = 1/(1/T0 + C) = T0/(1 + T0*C)

*/

double Thermistor::kelvin() {
  double C = _logConst - log(1.0/_adcPort.measure() - 1.0)/_beta;
  double tempK = 1.0/(_invNomT + C);
  if (_lastKelvin >= 0.0) {
    tempK = _lambda*tempK + (1.0-_lambda)*_lastKelvin;
  }
  _lastKelvin = tempK;
  return tempK;
}