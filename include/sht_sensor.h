#ifndef __SHT_SENSOR__
#define __SHT_SENSOR__

#include "config.h"

#include <uFire_SHT20.h>

#include "sensor.h"

class SHTTemperaturePort : public SensorPort {
public:
  SHTTemperaturePort(uFire_SHT20& sht) : _sht(sht) { }

  virtual void update() override {
    _value = (double)_sht.temperature_f();
  }

private:
  uFire_SHT20& _sht;
};

class SHTHumidityPort : public SensorPort {
public:
  SHTHumidityPort(uFire_SHT20& sht) : _sht(sht) { }

  virtual void update() override {
    _value = (double)_sht.humidity();
  }

private:
  uFire_SHT20& _sht;
};

// Trivial subclasses that hides the SHT*Port wrapper classes
using SHTTemperatureSensor = Sensor<SHTTemperaturePort, uFire_SHT20& >;
using SHTHumiditySensor = Sensor<SHTHumidityPort, uFire_SHT20& >;


#endif  /* __SHT_SENSOR__ */