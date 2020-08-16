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

// Trivial subclass that only defines a convenience constructor (and hides the *SensorPort wrapper class)
class WaterSensor : public Sensor {
public:
  WaterSensor(
    WaterLevelSensor& waterLevelSensor, unsigned long update_interval_millis,
    MQTT& mqtt, const char* mqtt_topic
  ) :
    Sensor(_port, update_interval_millis, mqtt, mqtt_topic),
    _port(waterLevelSensor)
  { }

private:
  WaterSensorPort _port;
};

// Trivial subclass with convenience constructor (no overrides or extensions)
class Aquastat : public Controller {
public:
  Aquastat(MQTT& mqtt, WaterSensor& waterLevel, Switch& waterValve) :
    Controller(
      mqtt,
      AQUASTAT_SETPOINT_DEFAULT,
      MQTT_TOPIC_AQUASTAT_STATE, MQTT_TOPIC_AQUASTAT_CONTROL,
      waterValve, waterLevel,
      AQUASTAT_UPDATE_INTERVAL_SEC * 1000)
  { }
};

#endif  /* __AQUASTAT_H__ */