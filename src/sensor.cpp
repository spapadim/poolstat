#include <Arduino.h>
#include "config.h"

#include "sensor.h"

void Sensor::begin(Scheduler* sched) {
  sched->addTask(_updateTask);
  _updateTask.enable();
}

void Sensor::update() {
  _port.update();
  _mqtt.publish(_mqttTopic.c_str(), String(_port.value(), 2).c_str(), true);  // Retained message
}
