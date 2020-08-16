#ifndef __SENSOR_H__
#define __SENSOR_H__

#include "config.h"
#include "task_scheduler.h"
#include "mqtt.h"

// Base class (interface) for "sensor port" wrapper classes;
//   only need to support a single get/update value pair of methods
class SensorPort {
public:
  virtual ~SensorPort() { }

  virtual void update() = 0;
  inline double value() const {
    return _value;
  }

protected:
  double _value;
};

// Analog sensor with periodic update and MQTT publishing
class Sensor {
public:
  Sensor(
    SensorPort& port, unsigned long update_interval_millis,
    MQTT& mqtt, const char* mqtt_topic
  ) :
    _port(port), _updateIntervalMillis(update_interval_millis),
    _mqtt(mqtt), _mqttTopic(mqtt_topic),
    _updateTask(update_interval_millis, TASK_FOREVER, std::bind(&Sensor::update, this))
  { }
  ~Sensor() { }

  void begin(Scheduler* sched);

  void update();

  inline double value() {
    return _port.value();
  }

private:
  SensorPort& _port;
  unsigned long _updateIntervalMillis;

  MQTT& _mqtt;
  const String _mqttTopic;

  Task _updateTask;
};

#endif  /* __SENSOR_H__ */