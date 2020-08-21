#include <Arduino.h>
#include "config.h"

#include "sensor.h"

void SensorBase::begin(Scheduler* sched, bool auto_update) {
  sched->addTask(_updateTask);
  set_auto_update_enabled(auto_update);
}

void SensorBase::update() {
  double oldValue = value();
  _port.update();

  double newValue = value();
  if (fabs(newValue - oldValue) >= UPDATE_TRIGGER_THRESHOLD) {
    _mqtt.publish(_mqttTopic.c_str(), String(_port.value(), 2).c_str(), true);  // Retained message
    updateSignal.emit(newValue);
  }

  DEBUG_MSG("Sensor::update() - value is %.2f", newValue);
}

void SensorBase::set_auto_update_enabled(bool enabled) {
  if (enabled) {
    _updateTask.enable();
  } else {
    _updateTask.disable();
  }
}
