#ifndef __THERMISTOR_H__
#define __THERMISTOR_H__

#include <Arduino.h>
#include <ADCLib.h>


class Thermistor {
public:
  Thermistor(
    ADCPort& adcPort, double referenceResistance, 
    double nominalResistance, double nominalTemperatureCelsius,
    double beta,
    double lambda = 1.0  // No smoothing
  ) : 
    _adcPort(adcPort), _refR(referenceResistance), 
    _nomR(nominalResistance), _nomT(nominalTemperatureCelsius + 273.15), _beta(beta),
    _lambda(lambda),
    _lastKelvin(-1.0),
    _invNomT(1.0/(nominalTemperatureCelsius + 273.15)),
    _logConst(log(referenceResistance/nominalResistance)/beta)
  {  }
  
  inline double celsius() { return kelvin() - 273.15; }
  inline double fahrenheit() { return 32.0 + 1.8 * celsius(); };
  double kelvin();

private:
  ADCPort& _adcPort;

  double _refR;  // Reference resistance (Ohm)
  double _nomR;  // Nominal thermistor resistance (Ohm)
  double _nomT;  // Nominal thermistor temperature (Kelvin)
  double _beta;  // Beta coefficient (unitless)

  // Used for exponential smoothing
  double _lambda;
  double _lastKelvin;

  // Derived values, to slighly speed up calculations
  double _invNomT;  // 1/T0
  double _logConst; // ln(Rref/R0)/B
};

#endif /* __THERMISTOR_H__ */