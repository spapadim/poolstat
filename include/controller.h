#ifndef __CONTROLLER_H__
#define __CONTROLLER_H__

#include "config.h"
#include "task_scheduler.h"
#include "mqtt.h"
#include "switch.h"
#include "sensor.h"

#include <functional>

// Trivial bang-bang controller (more than enough for super-slow-changing sensor values that we deal with)
class Controller {
public:
  static constexpr unsigned long DEFAULT_CONTROL_INTERVAL_MILLIS = 5000;

  // Default undershoot/overshoot
  //  NB. Only computed using initial_setpoint (i.e., *no* update to under/over-shoot when setpoint changes)
  static constexpr double DEFAULT_INITIAL_UNDERSHOOT_PCT = 0.006;  // 0.6%
  static constexpr double DEFAULT_INITIAL_OVERSHOOT_PCT = 0.004;   // 0.4%

  Controller(
    MQTT& mqtt,
    const char* const mqtt_setpoint_topic,
    double initial_setpoint,
    const char* const mqtt_state_topic, const char* const mqtt_control_topic,
    Switch& switch_, Sensor& sensor,
    double setpoint_undershoot = -1.0, double setpoint_overshoot = -1.0,
    unsigned long control_interval_millis = DEFAULT_CONTROL_INTERVAL_MILLIS) :
    _mqtt(mqtt),
    _mqttSetpointTopic(mqtt_setpoint_topic),
    _mqttStateTopic(mqtt_state_topic), _mqttControlTopic(mqtt_control_topic),
    _switch(switch_), _sensor(sensor),
    _setpoint(initial_setpoint), 
    _setpointUndershoot(setpoint_undershoot >= 0.0 ? setpoint_undershoot : DEFAULT_INITIAL_UNDERSHOOT_PCT * initial_setpoint), 
    _setpointOvershoot(setpoint_overshoot >= 0.0 ? setpoint_overshoot : DEFAULT_INITIAL_OVERSHOOT_PCT * initial_setpoint),
    _controlEnabled(false),
    _controlIntervalMillis(control_interval_millis),
    _controlLoopTask(control_interval_millis, TASK_FOREVER, std::bind(&Controller::_controlLoopCb, this))
  { }
  virtual ~Controller() { }

  void begin(Scheduler* sched);

  // Sensor-based control
  inline bool is_control_enabled()  {
    return _controlEnabled;
  }
  void set_control_enabled(bool enabled);
  inline void enable_control()  {
    set_control_enabled(true);
  }
  inline void disable_control() {
    set_control_enabled(false);
  }

  inline double get_setpoint()  {
    return _setpoint; 
  }
  void set_setpoint(double value);

private:
  MQTT& _mqtt;
  const String _mqttSetpointTopic;
  const String _mqttStateTopic;
  const String _mqttControlTopic;

  Switch& _switch;
  Sensor& _sensor;

  double _setpoint;
  const double _setpointUndershoot, _setpointOvershoot;
  bool _controlEnabled;

  const unsigned long _controlIntervalMillis;
  Task _controlLoopTask;

  void _controlLoopCb();

  void _mqttCb(const char* topic, String payload);
};

#endif  /* __CONTROLLER_H__ */