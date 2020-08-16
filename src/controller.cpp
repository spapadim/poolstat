#include <Arduino.h>
#include "config.h"

#include "controller.h"


void Controller::begin(Scheduler* sched) {
  using namespace std::placeholders;

  // Add tasks, but do not enable
  sched->addTask(_controlLoopTask);

  // Register MQTT callbacks for ".../control" topics
  _mqtt.add_subscription(_mqttControlTopic, std::bind(&Controller::_mqttCb, this, _1, _2), 1);
  _mqtt.add_subscription(_mqttSetpointTopic, std::bind(&Controller::_mqttCb, this, _1, _2), 1);

  // Publish MQTT messages (by setting control/switch states to their current value)
  set_control_enabled(is_control_enabled());
  set_setpoint(get_setpoint());
}

void Controller::set_control_enabled(bool enabled) {
  if (enabled) {
    _controlLoopTask.enable();
  } else {
    _controlLoopTask.disable();
  }
  _controlEnabled = enabled;

  // If disabling, turn off switch as well
  if (!enabled) {
    _switch.off();
  }

  _mqtt.publish(_mqttStateTopic.c_str(), enabled ? "on" : "off", true);

  DEBUG_MSG("Controller::set_control_enabled(%d)", int(enabled));
}

void Controller::set_setpoint(double value)  {
  _setpoint = value; 
  _mqtt.publish(_mqttSetpointTopic.c_str(), String(value, 2).c_str(), true);  // Retained message

  DEBUG_MSG("Controller::set_setpoint(%f)", value);
}

// private
void Controller::_controlLoopCb() {
  DEBUG_MSG("Controller::_controlLoopCb()");

  if (!_controlEnabled)  // Doesn't hurt to double-check ...
    return;

  // Trivial bang-bang update
  if (_sensor.value() > _setpoint + _setpointOvershoot && _switch.is_on()) {
    _switch.off();
  } else if (_sensor.value() < _setpoint - _setpointUndershoot && !_switch.is_on()) {
    _switch.on();
  }
}

// private
void Controller::_mqttCb(const char* topic, String payload) {
  if (_mqttControlTopic == topic) {
    bool enable = (payload == "on");
    DEBUG_MSG("MQTT Controller::set_control_enabled to %d", int(enable));
    set_control_enabled(enable);
  } else if (_mqttSetpointTopic == topic) {
    double value = atof(payload.c_str());
    DEBUG_MSG("MQTT Controller::set_setpoint to %f", value);
    set_setpoint(value);
  } else {
    DEBUG_MSG("WHOOPS: Controller got unexpected MQTT topic %s", topic);
  }
}
