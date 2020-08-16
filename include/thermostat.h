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

// Trivial subclass that only defines a convenience constructor (and hides the *SensorPort wrapper class)
class ThermistorSensor : public Sensor {
public:
  ThermistorSensor(
    Thermistor& thermistor, unsigned long update_interval_millis,
    MQTT& mqtt, const char* mqtt_topic
  ) :
    Sensor(_port, update_interval_millis, mqtt, mqtt_topic),
    _port(thermistor)
  { }

private:
  ThermistorSensorPort _port;
};

// Trivial subclass with convenience constructor (no overrides or extensions)
class Thermostat : public Controller {
public:
  Thermostat(MQTT& mqtt, ThermistorSensor& temperatureSensor, Switch& heaterRelay) :
    Controller(
      mqtt,
      THERMOSTAT_SETPOINT_POOL_DEFAULT,
      MQTT_TOPIC_THERMOSTAT_STATE, MQTT_TOPIC_THERMOSTAT_CONTROL,
      heaterRelay, temperatureSensor,
      THERMOSTAT_UPDATE_INTERVAL_SEC * 1000)
  { }
};

#endif  /* __THERMOSTAT_H__ */