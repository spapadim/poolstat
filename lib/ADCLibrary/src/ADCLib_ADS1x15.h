#ifndef __ADCLIB_ADS1X15_H__
#define __ADCLIB_ADS1X15_H__

#include "ADCLib.h"
#include <Adafruit_ADS1015.h>


// TODO - Add support for ADS1015 -- just a different resolution constant divisor in acquire(), no?
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

#endif  /* __ADCLIB_ADS1X15_H__ */
