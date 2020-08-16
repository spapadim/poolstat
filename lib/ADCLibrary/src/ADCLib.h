#ifndef __ADCLIB_H__
#define __ADCLIB_H__

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

// ESP32-specific (for now?)
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


#endif  /* __ADCLIB_H__ */