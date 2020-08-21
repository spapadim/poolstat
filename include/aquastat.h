#ifndef __AQUASTAT_H__
#define __AQUASTAT_H__

#include "config.h"
#include "controller.h"

#include <WaterLevel.h>

// Trivial wrapper class for WaterLevelSensor (so it can be used with Sensor)
class WaterSensorPort : public SensorPort {
public:
  WaterSensorPort(WaterLevelSensor& sensor) : _sensor(sensor) { }

  virtual void update() override {
    _value = _sensor.level();
  }

private:
  WaterLevelSensor& _sensor;
};

// Trivial subclass that hides the WaterSensorPort wrapper class
using WaterSensor = Sensor<WaterSensorPort, WaterLevelSensor& >;

// Trivial subclass with convenience constructor (no overrides or extensions)
class Aquastat : public Controller {
public:
  Aquastat(MQTT& mqtt, WaterSensor& waterLevel, Switch& waterValve) :
    Controller(
      mqtt,
      MQTT_TOPIC_AQUASTAT_SETPOINT,
      AQUASTAT_SETPOINT_DEFAULT,
      MQTT_TOPIC_AQUASTAT_STATE, MQTT_TOPIC_AQUASTAT_CONTROL,
      waterValve, waterLevel,
      AQUASTAT_UPDATE_INTERVAL_SEC * 1000)
  { }
};

#endif  /* __AQUASTAT_H__ */