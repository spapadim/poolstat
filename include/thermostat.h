#ifndef __THERMOSTAT_H__
#define __THERMOSTAT_H__

#include "config.h"
#include "controller.h"

#include <Thermistor.h>

// Trivial wrapper class for Thermistor
class ThermistorSensorPort : public SensorPort {
public:
  ThermistorSensorPort(Thermistor& thermistor) : _thermistor(thermistor) { }

  virtual void update() override {
    _value = _thermistor.fahrenheit();
  }

private:
  Thermistor& _thermistor;
};

// Trivial subclass that hides the ThermistorSensorPort wrapper class
using ThermistorSensor = Sensor<ThermistorSensorPort, Thermistor& >;

// Trivial subclass with convenience constructor (no overrides or extensions)
class Thermostat : public Controller {
public:
  Thermostat(MQTT& mqtt, ThermistorSensor& temperatureSensor, Switch& heaterRelay) :
    Controller(
      mqtt,
      MQTT_TOPIC_THERMOSTAT_SETPOINT,
      THERMOSTAT_SETPOINT_DEFAULT,
      MQTT_TOPIC_THERMOSTAT_STATE, MQTT_TOPIC_THERMOSTAT_CONTROL,
      heaterRelay, temperatureSensor,
      THERMOSTAT_UPDATE_INTERVAL_SEC * 1000)
  { }
};

#endif  /* __THERMOSTAT_H__ */