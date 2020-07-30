#ifndef __THERMISTOR_H__
#define __THERMISTOR_H__

#include <Arduino.h>

#ifdef THERMISTOR_CONFIG_ADS1115
#include <Adafruit_ADS1015.h>
#endif

/***************************************************************************
 * 
 ***************************************************************************/

// Abstract base class
class ADCPort {
public:
  static const uint8_t DEFAULT_SAMPLES = 5;
  static const uint32_t DEFAULT_INTERVAL = 30;

  ADCPort(double attenuation = 1.0, uint8_t nSamples = DEFAULT_SAMPLES, uint32_t interval = DEFAULT_INTERVAL)
  : _attenuation(attenuation), _nSamples(nSamples), _interval(interval) { }
  virtual ~ADCPort() { }
  virtual double acquire() = 0;  // Should return adc_value/resolution (i.e., in 0.0..1.0 range)

  double measure() {
    double sum = 0.0;
    for (uint8_t i = 0;  i < _nSamples;  i++) {
      sum += acquire();
      delay(_interval);
    }
    double val = sum / _nSamples * _attenuation;
    Serial.printf("****  Measurement %f\n", val);
    return val;
  }

private:
  double _attenuation;
  uint8_t _nSamples;
  uint32_t _interval;
};

class MCUPort : public ADCPort {
public:
  MCUPort(int pin, double attenuation = 1.0, uint8_t nSamples = DEFAULT_SAMPLES, uint32_t interval = DEFAULT_INTERVAL)
  : ADCPort(attenuation, nSamples, interval), _pin(pin) { }
  virtual ~MCUPort() { }

  virtual double acquire() override { 
    double val = ((double)analogRead(_pin)) / 4095.0;
    Serial.printf("****  ADC reading %f\n", val);
    return val;
  }

private:
  int _pin;
};

#ifdef THERMISTOR_CONFIG_ADS1115
class ADS1115Port : public ADCPort {
public:
  ADS1115Port(
    Adafruit_ADS1015& ads, uint8_t channel,
    double attenuation = 1.0, uint8_t nSamples = DEFAULT_SAMPLES, uint32_t interval = DEFAULT_INTERVAL
  ) 
  : ADCPort(attenuation, nSamples, interval), _ads(ads), _channel(channel) { }
  virtual ~ADS1115Port() { }

  virtual double acquire() override { 
    double val = ((double)_ads.readADC_SingleEnded(_channel)) / 32767.0;
    Serial.printf("****  ADC reading %f\n", val);
    return val;
  }

private:
  Adafruit_ADS1015& _ads;
  uint8_t _channel;
};
#endif

/***************************************************************************
 * 
 ***************************************************************************/



class Thermistor {
public:
  Thermistor(
    ADCPort& adcPort, double referenceResistance, 
    double nominalResistance, double nominalTemperatureCelsius,
    double beta,
    double lambda = 1.0  // No smoothing
  )
  : _adcPort(adcPort), _refR(referenceResistance), 
    _nomR(nominalResistance), _nomT(nominalTemperatureCelsius + 273.15), _beta(beta),
    _lambda(lambda),
    _lastKelvin(-1.0),
    _invNomT(1.0/(nominalTemperatureCelsius + 273.15)),
    _logConst(log(referenceResistance/nominalResistance)/beta)
  { }
  
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